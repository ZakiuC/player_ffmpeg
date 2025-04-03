#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QThread>
#include <QImage>
#include <QString>
#include <QWaitCondition>
#include <QMutex>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QThread
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr, const QString &url = "");
    ~VideoDecoder();

    void stop();

signals:
    void frameReady(const QImage &frame);
    void errorOccurred(const QString &message);

protected:
    void run() override;

private:
    void cleanup();
    bool initDecoder();
    bool initSwsContext();

    QString m_url;
    bool m_running = false;

    // FFmpeg相关成员
    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    int m_videoStream = -1;
    std::atomic<bool> m_paused{false};
        QWaitCondition m_pauseCondition;
        QMutex m_pauseMutex;
};

#endif // VIDEODECODER_H
