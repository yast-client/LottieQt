//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>
#include <QImageIOPlugin>

#include "tlottie.h"

#include <QSize>
#include <QImage>
#include <QImageIOHandler>
#include <QtConcurrent/QtConcurrent>

class TgsIOHandler : public QImageIOHandler {
public:
    static const QByteArray NAME;
    static const QByteArray GZ_MAGIC;
    typedef std::string ByteArray;

    TgsIOHandler(QIODevice* device, const QByteArray& format);
    ~TgsIOHandler();

    // QImageIOHandler
    bool canRead() const override;
    bool read(QImage* image) override;
    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant &value) override;
    bool supportsOption(ImageOption option) const override;
    bool jumpToNextImage() override;
    bool jumpToImage(int imageNumber) override;
    int loopCount() const override;
    int imageCount() const override;
    int nextImageDelay() const override;
    int currentImageNumber() const override;
    QRect currentImageRect() const override;

    bool currentRenderReady() const;

private:
    ByteArray uncompress();
    bool load();
    void render(int frameIndex);
    void finishRendering();

    bool doRenderFrame(int frame, int width, int height);

private:
    QString fileName;
    QSize size;
    QSize scaledSize;
    qreal frameRate = 0.;
    int frameCount = 0;
    int currentFrame = 0;
    QImage firstImage;
    QImage prevImage;
    QImage currentImage;
    TLottieInstance *instance = nullptr;
    QFuture<bool> currentRender; // FIXME: should be QFuture<QImage> ideally
};

class TgsIOPlugin : public QImageIOPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "tgsplugin.json")
public:
    Capabilities capabilities(QIODevice* device, const QByteArray& format) const override;
    QImageIOHandler* create(QIODevice* device, const QByteArray& format) const override;
};
