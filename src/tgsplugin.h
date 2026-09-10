//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>
#include <QImageIOPlugin>

#include "rlottie.h"

#include <QSize>
#include <QImage>
#include <QImageIOHandler>

class TgsIOHandler : public QImageIOHandler {
public:
    static const QByteArray NAME;
    static const QByteArray GZ_MAGIC;
    typedef std::string ByteArray;

    TgsIOHandler(QIODevice* device, const QByteArray& format);
    ~TgsIOHandler();

    ByteArray uncompress();
    bool load();
    void render(int frameIndex);
    void finishRendering();

    // QImageIOHandler
    bool canRead() const Q_DECL_OVERRIDE;
    bool read(QImage* image) Q_DECL_OVERRIDE;
    QVariant option(ImageOption option) const Q_DECL_OVERRIDE;
    void setOption(ImageOption option, const QVariant &value) Q_DECL_OVERRIDE;
    bool supportsOption(ImageOption option) const Q_DECL_OVERRIDE;
    bool jumpToNextImage() Q_DECL_OVERRIDE;
    bool jumpToImage(int imageNumber) Q_DECL_OVERRIDE;
    int loopCount() const Q_DECL_OVERRIDE;
    int imageCount() const Q_DECL_OVERRIDE;
    int nextImageDelay() const Q_DECL_OVERRIDE;
    int currentImageNumber() const Q_DECL_OVERRIDE;
    QRect currentImageRect() const Q_DECL_OVERRIDE;

    bool currentRenderReady() const;

public:
    QString fileName;
    QSize size;
    QSize scaledSize;
    qreal frameRate = 0.;
    int frameCount = 0;
    int currentFrame = 0;
    QImage firstImage;
    QImage prevImage;
    QImage currentImage;
    std::future<rlottie::Surface> currentRender;
    std::unique_ptr<rlottie::Animation> animation;
};

class TgsIOPlugin : public QImageIOPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "tgsplugin.json")
public:
    Capabilities capabilities(QIODevice* device, const QByteArray& format) const Q_DECL_OVERRIDE;
    QImageIOHandler* create(QIODevice* device, const QByteArray& format) const Q_DECL_OVERRIDE;
};
