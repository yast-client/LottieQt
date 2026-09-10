//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickItem>
#include <QTimer>
#include <QNetworkAccessManager>
#include "tgsplugin.h"

class LottieItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool autoLoad MEMBER autoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(QUrl source MEMBER source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool error MEMBER error NOTIFY errorChanged)
    Q_PROPERTY(bool paused MEMBER paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    Q_PROPERTY(QSize scaledSize READ scaledSize WRITE setScaledSize NOTIFY scaledSizeChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY frameCountChanged)
    Q_PROPERTY(bool loop MEMBER loop WRITE setLoop NOTIFY loopChanged)

public:
    LottieItem();
    ~LottieItem();

    void setSource(QUrl source);
    void setAutoLoad(bool value);
    bool loaded() const;
    Q_INVOKABLE void begin();

    void setPaused(bool value);

    int currentFrame() const;
    void setCurrentFrame(int frame);

    QSize sourceSize() const;
    QSize scaledSize() const;
    void setScaledSize(QSize size);

    int frameCount() const;

    void setLoop(bool value);

signals:
    void sourceChanged();
    void autoLoadChanged();
    void loadedChanged();
    void errorChanged();
    void stoppedChanged();
    void pausedChanged();
    void currentFrameChanged();
    void sourceSizeChanged();
    void scaledSizeChanged();
    void frameCountChanged();
    void loopChanged();
    void loopFinished();

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData*) override;

private:
    void setupHandler();
    void updateSize();
    void setError();
    void reset();
    void loadNextFrame();
    bool isLoopFinished();

private slots:
    void handleNetworkRequestFinished();

private:
    QUrl source;
    QIODevice *device;
    TgsIOHandler *handler; // TODO/TBD: should we remove TgsIOHandler?
    QNetworkAccessManager *networkManager;
    QImage currentImage;
    QTimer nextImageTimer;
    QMap<QImageIOHandler::ImageOption, QVariant> pendingOptions;
    int pendingFrameJump;
    bool jumpedToFrame;
    bool autoLoad;
    bool error;
    bool paused;
    bool loop;
};
