//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "tgsplugin.h"

#include <QFileDevice>
#include <QFileInfo>

#include <zlib.h>

#define DEBUG_MODULE TgsIOHandler
#include "debuglog.h"
#define LOG_(x) LOG(qPrintable(fileName) << x)

const QByteArray TgsIOHandler::NAME("tgs");
const QByteArray TgsIOHandler::GZ_MAGIC("\x1f\x8b");

TgsIOHandler::TgsIOHandler(QIODevice* device, const QByteArray& format) {
    QFileDevice* file = qobject_cast<QFileDevice*>(device);
    if (file)
        fileName = QFileInfo(file->fileName()).fileName();
    setDevice(device);
    setFormat(format);
}

TgsIOHandler::~TgsIOHandler() {
    if (currentRender.isRunning())
        currentRender.waitForFinished();
    if (instance)
        tlottie_drop(instance);
    LOG_("Done");
}

TgsIOHandler::ByteArray TgsIOHandler::uncompress() {
    const QByteArray zipped(device()->readAll());

    if (zipped.mid(0, 2) != GZ_MAGIC) {
        // Not compressed
        LOG_("Not compressed");
        return ByteArray(zipped.constData(), zipped.length());
    }

    LOG_("Uncompressing");
    std::string unzipped;
    if (!zipped.isEmpty()) {
        z_stream unzip;
        memset(&unzip, 0, sizeof(unzip));
        unzip.next_in = (Bytef*)zipped.constData();
        // Add 16 for decoding gzip header
        int zerr = inflateInit2(&unzip, MAX_WBITS + 16);
        if (zerr == Z_OK) {
            const uint chunk = 0x1000;
            unzipped.resize(chunk);
            unzip.next_out = (Bytef*)unzipped.data();
            unzip.avail_in = zipped.size();
            unzip.avail_out = chunk;
            LOG_("Compressed size" << zipped.size());
            while (unzip.avail_out > 0 && zerr == Z_OK) {
                zerr = inflate(&unzip, Z_NO_FLUSH);
                if (zerr == Z_OK && unzip.avail_out < chunk) {
                    // Data may get relocated, update next_out too
                    unzipped.resize(unzipped.size() + chunk);
                    unzip.next_out = (Bytef*)unzipped.data() + unzip.total_out;
                    unzip.avail_out += chunk;
                }
            }
            if (zerr == Z_STREAM_END) {
                unzipped.resize(unzip.next_out - (Bytef*)unzipped.data());
                LOG_("Uncompressed size" << unzipped.size());
            } else {
                unzipped.clear();
            }
            inflateEnd(&unzip);
        }
    }
    return unzipped;
}

bool TgsIOHandler::load() {
    if (!instance && device()) {
        ByteArray json = uncompress();
        if (json.size() > 0) {
            instance = tlottie_new_with_options(reinterpret_cast<const uint8_t*>(json.data()), json.size(),
                                                TLOTTIE_FITZ_NONE,
                                                nullptr, 0, nullptr, 0,
                                                TLOTTIE_CHANNEL_BGRA);
            if (instance) {
                size = QSize(tlottie_width(instance), tlottie_height(instance));
                frameRate = tlottie_frame_rate(instance);
                frameCount = tlottie_frame_count(instance);
                LOG_(size << frameCount << "frames," << frameRate << "fps");
                render(0); // Pre-render first frame
            }
        }
    }
    return instance;
}

void TgsIOHandler::finishRendering() {
    if (currentRender.isRunning()) {
        currentRender.waitForFinished();
        prevImage = currentImage;
        if (!currentFrame && !firstImage.isNull()) {
            LOG_("Rendered first frame");
            firstImage = currentImage;
        }
    } else // Must be the first frame
        prevImage = currentImage;
}

bool TgsIOHandler::doRenderFrame(int frame, int width, int height) {
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    TlottieStatus status = static_cast<TlottieStatus>(tlottie_render(
        instance, frame, width, height,
        reinterpret_cast<uint32_t*>(currentImage.bits()), pixelCount,
        1 // antialiasing
    ));

    if (status != TLOTTIE_OK) {
        LOG_("Couldn't render frame" << frame << "tlottie error" << status);
        return false;
    }
    return true;
}

void TgsIOHandler::render(int frameIndex) {
    currentFrame = frameIndex % frameCount;
    if (!currentFrame && !firstImage.isNull()) {
        // The first frame only gets rendered once
        currentImage = firstImage;
    } else {
        int width, height;
        if (!scaledSize.isEmpty()) {
            width = scaledSize.width();
            height = scaledSize.height();
        } else {
            width = size.width();
            height = size.height();
        }

        currentImage = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
        currentRender = QtConcurrent::run(this, &TgsIOHandler::doRenderFrame, currentFrame, width, height);
    }
}

bool TgsIOHandler::read(QImage* out) {
    if (load() && frameCount) {
        // We must have the first frame, will wait if necessary
        if (currentFrame && currentRender.isStarted() && (!currentRender.isFinished() || !currentRender.result())) {
            LOG_("Skipping frame" << currentFrame);
            currentFrame = (currentFrame + 1) % frameCount;
            *out = prevImage;
            return true;
        }
        finishRendering();
        *out = currentImage;
        render(currentFrame + 1);
        return true;
    }
    return false;
}

bool TgsIOHandler::canRead() const {
    return device();
}

QVariant TgsIOHandler::option(ImageOption option) const {
    switch (option) {
    case Size:
        ((TgsIOHandler*)this)->load(); // Cast off const
        return size;
    case Animation:
        return true;
    case ImageFormat:
        return QImage::Format_ARGB32_Premultiplied;
    default:
        break;
    }
    return QVariant();
}

bool TgsIOHandler::supportsOption(ImageOption option) const {
    switch (option) {
    case Size:
    case Animation:
    case ImageFormat:
    case ScaledSize:
        return true;
    default:
        break;
    }
    return false;
}

void TgsIOHandler::setOption(ImageOption option, const QVariant &value) {
    switch (option) {
    case ScaledSize:
        if (scaledSize != value.toSize()) {
            scaledSize = value.toSize();
            LOG_("Scaled to" << scaledSize);
        }
        break;
    default:
        break;
    }
}

bool TgsIOHandler::jumpToNextImage() {
    if (frameCount) {
        finishRendering();
        render(currentFrame + 1);
        return true;
    }
    return false;
}

bool TgsIOHandler::jumpToImage(int imageNumber) {
    if (frameCount) {
        if (imageNumber != currentFrame) {
            finishRendering();
            render(imageNumber);
        }
        return true;
    }
    return false;
}

int TgsIOHandler::loopCount() const {
    return -1;
}

int TgsIOHandler::imageCount() const {
    return frameCount;
}

int TgsIOHandler::currentImageNumber() const {
    return currentFrame;
}

QRect TgsIOHandler::currentImageRect() const {
    return QRect(QPoint(), size);
}

int TgsIOHandler::nextImageDelay() const {
    return frameRate > 0 ? static_cast<int>(1000/frameRate) : 33;
}

bool TgsIOHandler::currentRenderReady() const {
    return frameCount && currentFrame && currentRender.isStarted() && (!currentRender.isFinished() || !currentRender.result());
}

QImageIOPlugin::Capabilities TgsIOPlugin::capabilities(QIODevice*, const QByteArray& format) const {
    return Capabilities(format == TgsIOHandler::NAME ? CanRead : 0);
}

QImageIOHandler* TgsIOPlugin::create(QIODevice* device, const QByteArray& format) const {
    return new TgsIOHandler(device, format);
}
