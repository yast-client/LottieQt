//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "lottieitem.h"

#include <QQmlFile>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QFile>
#include <QNetworkRequest>
#include <QNetworkReply>

#define DEBUG_MODULE LottieItem
#include "debuglog.h"
#define LOG_(x) LOG(qPrintable(source.isValid() ? (source.isLocalFile() ? source.toLocalFile() : "<not a local file>") : "") << x)

LottieItem::LottieItem() :
    QQuickItem(),
    device(nullptr),
    handler(nullptr),
    networkManager(new QNetworkAccessManager(this)),
    pendingFrameJump(-1),
    jumpedToFrame(false),
    autoLoad(true),
    error(false),
    paused(false),
    loop(false)
{
    setFlag(ItemHasContents);

    nextImageTimer.setSingleShot(true);
    connect(&nextImageTimer, &QTimer::timeout, this, &LottieItem::loadNextFrame);
}

LottieItem::~LottieItem() {
    LOG_("Done");
    delete handler;
    if (device) {
        device->close();
        device->deleteLater();
    }
}

QSize LottieItem::sourceSize() const {
    if (handler)
        return handler->option(QImageIOHandler::Size).toSize();
    return QSize();
}

QSize LottieItem::scaledSize() const {
    if (handler)
        return handler->option(QImageIOHandler::ScaledSize).toSize();
    return pendingOptions.value(QImageIOHandler::ScaledSize).toSize();
}

void LottieItem::setScaledSize(QSize size) {
    if (scaledSize() != size) {
        if (handler) {
            handler->setOption(QImageIOHandler::ScaledSize, size);
            updateSize();
        } else
            pendingOptions.insert(QImageIOHandler::ScaledSize, size);
        emit scaledSizeChanged();
    }
}

void LottieItem::updateSize() {
    QSize size = scaledSize();
    if (size.isEmpty())
        size = sourceSize();
    if (!size.isEmpty()) {
        setImplicitSize(size.width(), size.height());
        emit sourceSizeChanged();
    }
    LOG_("Implicit size set to" << implicitWidth() << implicitHeight());
}

void LottieItem::reset() {
    delete handler;
    handler = nullptr;
    if (device) {
        device->close();
        device->deleteLater();
        device = nullptr;
    }
    emit loadedChanged();
}

void LottieItem::setSource(QUrl source) {
    if (this->source == source)
        return;
    reset();
    this->source = source;
    emit sourceChanged();
    LOG_("Source set");

    if (!source.isValid())
        return;

    const QString fileName = QQmlFile::urlToLocalFileOrQrc(source);
    if (!fileName.isEmpty()) {
        this->device = new QFile(fileName, this);
        if (!device->isOpen() && !device->open(QFile::ReadOnly)) {
            setError();
            return;
        }

        setupHandler();
    } else {
        QNetworkRequest request(source);
        QNetworkReply *reply = this->networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, &LottieItem::handleNetworkRequestFinished);
    }
}

void LottieItem::handleNetworkRequestFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply->error() != QNetworkReply::NoError) {
        setError();
        return;
    }

    this->device = reply;
    setupHandler();
}

void LottieItem::setupHandler() {
    delete handler;
    handler = new TgsIOHandler(this->device, TgsIOHandler::NAME);
    for (QImageIOHandler::ImageOption option : pendingOptions.keys())
        handler->setOption(option, pendingOptions.value(option));
    pendingOptions.clear();
    updateSize();
    LOG_(sourceSize() << scaledSize());
    emit loadedChanged();
    if (autoLoad)
        begin();
}

void LottieItem::setAutoLoad(bool value) {
    if (autoLoad != value) {
        LOG_("Set auto load" << value);
        autoLoad = value;
        emit autoLoadChanged();
    }
}

bool LottieItem::loaded() const {
    return handler;
}

void LottieItem::begin() {
    setAutoLoad(true);
    if (!handler)
        return;

    if (pendingFrameJump >= 0) {
        // already sets stopped to false
        setCurrentFrame(pendingFrameJump);
        pendingFrameJump = -1;
    } else {
        nextImageTimer.stop();
        loadNextFrame();
    }
}

void LottieItem::setPaused(bool value) {
    if (paused != value) {
        paused = value;
        LOG_((paused ? "Pausing" : "Unpausing"));
        if (paused && !jumpedToFrame)
            nextImageTimer.stop();
        else if (!isLoopFinished())
            loadNextFrame();
        emit pausedChanged();
    }
}

int LottieItem::currentFrame() const {
    if (handler)
        return handler->currentImageNumber();
    return 0;
}

void LottieItem::setCurrentFrame(int frame) {
    if (frame < 0)
        return;

    if (handler) {
        if (handler->jumpToImage(frame)) {
            jumpedToFrame = true;
            nextImageTimer.stop();
            loadNextFrame();
            LOG_("Jumped to frame" << frame);
        } else
            LOG_("Couldn't jump to frame" << frame);
    } else {
        LOG_("Pending frame jump" << frame);
        pendingFrameJump = frame;
    }
}

int LottieItem::frameCount() const {
    if (handler)
        return handler->imageCount();
    return 0;
}

void LottieItem::setLoop(bool value) {
    if (loop != value) {
        loop = value;
        emit loopChanged();
    }
}

void LottieItem::setError() {
    if (!error) {
        error = true;
        emit errorChanged();
    }
}

inline bool LottieItem::isLoopFinished() {
    return !loop && (handler->currentImageNumber() % handler->imageCount()) >= (handler->imageCount() - 1);
}

void LottieItem::loadNextFrame() {
    if (!handler) return;

    if (jumpedToFrame && paused) {
        if (!handler->currentRenderReady()) {
            // wait for the frame to be loaded, don't skip it
            nextImageTimer.start(handler->nextImageDelay());
            return;
        } else
            jumpedToFrame = false;
    }

    if (!handler->read(&currentImage)) {
        setError();
        return;
    }

    update();
    emit currentFrameChanged();

    if (!paused) {
        if (!isLoopFinished())
            nextImageTimer.start(handler->nextImageDelay());
        else
            emit loopFinished();
    }
}

QSGNode *LottieItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData*) {
    if (currentImage.isNull()) {
        delete oldNode;
        return nullptr;
    }

    QSGSimpleTextureNode *textureNode = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!textureNode) {
        textureNode = new QSGSimpleTextureNode();
        textureNode->setOwnsTexture(true);
    }

    QQuickWindow *window = this->window();
    if (window) {
        //QPixmap pixmap = QPixmap::fromImage(currentImage);
        QSGTexture *texture = window->createTextureFromImage(currentImage);
        textureNode->setTexture(texture);
        textureNode->setRect(boundingRect());
    }

    return textureNode;
}
