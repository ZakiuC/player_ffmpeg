#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QThread>
#include <QImage>
#include <QString>
#include <QWaitCondition>
#include <QMutex>
#include <atomic>  // 用于原子类型

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "config.h"

/**
 * @brief 负责从视频流中解码视频帧，并通过信号传递给界面显示
 */
class VideoDecoder : public QThread
{
    Q_OBJECT
public:
    explicit VideoDecoder(const AppConfig &cfg, QObject *parent = nullptr);
    ~VideoDecoder();

    /**
     * @brief 停止解码线程，并安全退出（不使用强制终止）
     */
    void stop();

signals:
    /**
     * @brief 当视频帧解码完成后发出信号
     * @param frame 解码后的 QImage 对象
     */
    void frameReady(const QImage &frame);

    /**
     * @brief 当解码发生错误时发出信号
     * @param message 错误描述
     */
    void errorOccurred(const QString &message);

protected:
    void run() override;

private:
    void cleanup();
    bool initDecoder();
    bool initSwsContext();

    AppConfig m_config;
    std::atomic<bool> m_running{false};  // 用于控制线程运行状态
    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    int m_videoStream = -1;

    // 暂停/唤醒功能预留
    std::atomic<bool> m_paused{false};
    QWaitCondition m_pauseCondition;
    QMutex m_pauseMutex;
};

#endif // VIDEODECODER_H
