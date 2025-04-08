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
#include "config.h"

/**
 * @brief 负责从视频流中解码视频帧，并通过信号传递给界面显示
 */
class VideoDecoder : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化成员变量，并设置默认的 URL
     * @param cfg 配置对象，存储编码所需的配置信息
     * @param parent 父对象
     */
    explicit VideoDecoder(const AppConfig &cfg, QObject *parent = nullptr);
    ~VideoDecoder();

    /**
     * @brief 停止解码线程，并清理所有资源
     */
    void stop();

signals:
    /**
     * @brief 当视频帧解码完成后，发出此信号传递解码后的视频帧
     * @param frame 解码后的视频帧
     */
    void frameReady(const QImage &frame);
    /**
     * @brief 当解码过程中出现错误时，发出此信号传递错误信息
     * @param message 错误信息
     */
    void errorOccurred(const QString &message);

protected:
    /**
     * @brief 重载 QThread 的 run() 函数，线程的主入口，解码视频帧并传递给界面显示
     */
    void run() override;

private:
    /**
     * @brief 清理所有分配的 FFmpeg 资源，释放解码器、格式上下文和 sws 转换上下文
     */
    void cleanup();

    /**
     * @brief 初始化视频解码器，配置并打开解码器
     */
    bool initDecoder();

    /**
     * @brief 初始化 sws 转换上下文，用于颜色空间转换
     * @return 成功初始化返回 true，否则返回 false
     */
    bool initSwsContext();

    // 配置对象，存储编码所需的配置信息
    AppConfig m_config;

    bool m_running = false; // 控制解码线程运行的标志

    // FFmpeg 相关成员变量，用于存储视频流、解码器及颜色转换上下文
    AVFormatContext *m_formatCtx = nullptr; // 格式上下文，管理视频输入流
    AVCodecContext *m_codecCtx = nullptr;   // 编解码器上下文，负责视频解码
    SwsContext *m_swsCtx = nullptr;         // sws 转换上下文，用于转换颜色空间
    int m_videoStream = -1;                 // 视频流索引

    // 线程暂停相关变量（当前未实现暂停功能，仅预留扩展接口）
    std::atomic<bool> m_paused{false};
    QWaitCondition m_pauseCondition; // 条件变量，用于线程暂停与唤醒
    QMutex m_pauseMutex;             // 互斥锁，保护暂停状态数据
};

#endif // VIDEODECODER_H
