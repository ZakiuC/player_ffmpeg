#include "video_decoder.h"
#include <QDebug>

extern "C"
{
#include <libavutil/imgutils.h>
}

VideoDecoder::VideoDecoder(const AppConfig &cfg, QObject *parent)
    : QThread(parent), m_config(cfg)
{
    // 初始化 FFmpeg 网络模块，支持网络协议
    avformat_network_init();
}

// 析构函数，停止解码线程并清理所有分配的资源
VideoDecoder::~VideoDecoder()
{
    stop();                    // 停止线程
    cleanup();                 // 清理解码器、格式上下文等资源
    avformat_network_deinit(); // 反初始化网络模块
}

void VideoDecoder::stop()
{
    // 将运行标志置为 false
    m_running = false;
    // 如果线程仍在运行，则等待一定时间后强制终止
    if (isRunning())
    {
        wait(500);   // 等待 500 毫秒
        terminate(); // 强制终止线程（在紧急情况使用）
    }
}

void VideoDecoder::run()
{
    m_running = true; // 设置线程运行标志为 true

    // 初始化 FFmpeg 解码参数，设置网络传输、延迟、丢帧等选项
    AVDictionary *options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0); // 使用 TCP 协议传输数据
    av_dict_set(&options, "fflags", "nobuffer", 0);    // 禁用缓冲，降低延迟
    av_dict_set(&options, "flags", "low_delay", 0);    // 设置低延迟模式
    av_dict_set(&options, "tune", "zerolatency", 0);   // 调整为零延迟模式
    av_dict_set(&options, "framedrop", "1", 0);        // 允许丢弃部分帧以保持实时性
    av_dict_set(&options, "probesize", "32", 0);       // 降低探测数据大小，加快启动速度

    // 打开视频流输入，使用指定的 URL 和参数
    if (avformat_open_input(&m_formatCtx, m_config.RTMP_URL.toUtf8().constData(), nullptr, &options) < 0)
    {
        // 打开视频流失败，发送错误信号并退出线程
        emit errorOccurred("Failed to open stream");
        return;
    }

    // 读取视频流信息，获取流内各种参数
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        // 读取流信息失败，发送错误信号
        emit errorOccurred("Failed to get stream info");
        return;
    }

    // 遍历所有流，查找视频流的索引
    for (unsigned i = 0; i < m_formatCtx->nb_streams; i++)
    {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
            break;
        }
    }

    // 如果未找到视频流或初始化解码器失败，则发送错误信号并退出
    if (m_videoStream == -1 || !initDecoder())
    {
        emit errorOccurred("No video stream found");
        return;
    }

    // 初始化颜色空间转换上下文，将解码帧转换为 RGB 格式
    if (!initSwsContext())
    {
        emit errorOccurred("Failed to initialize sws context");
        return;
    }

    // 分配解码过程中的必要资源
    AVPacket *packet = av_packet_alloc(); // 分配 AVPacket，用于存储压缩数据包
    AVFrame *frame = av_frame_alloc();    // 分配 AVFrame，用于存储解码后的原始帧
    AVFrame *rgbFrame = av_frame_alloc(); // 分配 AVFrame，用于存储转换后的 RGB 帧
    uint8_t *buffer = nullptr;            // 用于存储 RGB 数据的内存缓冲区

    // 根据视频流参数计算转换后 RGB 图像所需的缓冲区大小
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStream]->codecpar;
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                                              codecPar->width,
                                              codecPar->height, 1);
    // 分配内存缓冲区，并初始化 rgbFrame 的数据指针和行宽信息
    buffer = static_cast<uint8_t *>(av_malloc(bufferSize));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer,
                         AV_PIX_FMT_RGB24, codecPar->width,
                         codecPar->height, 1);

    // 主解码循环：不断读取数据包，进行解码和颜色空间转换，直到停止标志为 false
    while (m_running)
    {
        // 从视频流中读取一个数据包
        int ret = av_read_frame(m_formatCtx, packet);
        if (ret < 0)
        {
            // 如果读取到文件尾，则退出循环
            if (ret == AVERROR_EOF)
                break;
            // 若发生其他错误且不是临时错误，则记录错误并退出
            if (ret != AVERROR(EAGAIN))
            {
                qWarning() << "Read frame error:" << ret;
                break;
            }
            continue;
        }

        // 判断当前数据包是否属于视频流
        if (packet->stream_index == m_videoStream)
        {
            // 将数据包发送到解码器进行解码
            ret = avcodec_send_packet(m_codecCtx, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN))
            {
                qWarning() << "Error sending packet:" << ret;
                av_packet_unref(packet);
                continue;
            }

            // 循环接收解码后的帧
            while (true)
            {
                ret = avcodec_receive_frame(m_codecCtx, frame);
                // 如果没有更多帧可接收，则退出循环
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0)
                {
                    qWarning() << "Error receiving frame:" << ret;
                    break;
                }

                // 使用 sws_scale 将解码得到的帧转换为 RGB24 格式
                sws_scale(m_swsCtx,
                          frame->data, frame->linesize,
                          0, codecPar->height,
                          rgbFrame->data, rgbFrame->linesize);

                // 构造 QImage 对象，注意此处直接使用缓冲区数据构造，保证内存正确传递
                emit frameReady(QImage(buffer, codecPar->width, codecPar->height, QImage::Format_RGB888));
            }
        }
        // 释放数据包占用的内存，准备读取下一个数据包
        av_packet_unref(packet);
    }

    // 解码完成后，释放所有分配的内存和资源
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&rgbFrame);
    av_packet_free(&packet);
}

bool VideoDecoder::initDecoder()
{
    // 获取视频流的编解码参数
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStream]->codecpar;
    // 查找支持该编解码器 ID 的解码器
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        qWarning() << "Unsupported codec:" << codecPar->codec_id;
        return false;
    }

    // 分配编解码器上下文，并将参数复制到上下文中
    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, codecPar);

    // 设置低延迟相关参数，优化解码速度
    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
    m_codecCtx->thread_count = 4;              // 设置解码线程数，根据硬件情况可调整
    m_codecCtx->thread_type = FF_THREAD_FRAME; // 使用帧级并行解码

    // 打开解码器
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0)
    {
        qWarning() << "Failed to open codec";
        return false;
    }
    return true;
}

bool VideoDecoder::initSwsContext()
{
    // 获取视频流参数
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStream]->codecpar;
    // 创建颜色空间转换上下文，输入为解码器输出格式，输出为 RGB24 格式
    m_swsCtx = sws_getContext(
        codecPar->width, codecPar->height,
        m_codecCtx->pix_fmt,
        codecPar->width, codecPar->height,
        AV_PIX_FMT_RGB24,
        // SWS_BILINEAR,
        SWS_FAST_BILINEAR,
        nullptr, nullptr, nullptr);
    // 返回转换上下文是否创建成功
    return m_swsCtx != nullptr;
}

void VideoDecoder::cleanup()
{
    if (m_codecCtx)
    {
        avcodec_close(m_codecCtx);         // 关闭解码器
        avcodec_free_context(&m_codecCtx); // 释放解码器上下文
    }
    if (m_formatCtx)
    {
        avformat_close_input(&m_formatCtx); // 关闭视频流输入
    }
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx); // 释放颜色空间转换上下文
        m_swsCtx = nullptr;
    }
}
