#include "video_decoder.h"
#include <QDebug>

extern "C"
{
#include <libavutil/imgutils.h>
}

VideoDecoder::VideoDecoder(QObject *parent, const QString &url)
    : QThread(parent), m_url(url)
{
    avformat_network_init();
}

VideoDecoder::~VideoDecoder()
{
    stop();
    cleanup();
    avformat_network_deinit();
}

void VideoDecoder::stop()
{
    m_running = false;
    if (isRunning())
    {
        wait(500);
        terminate();
    }
}

void VideoDecoder::run()
{
    m_running = true;

    // 初始化FFmpeg参数
    AVDictionary *options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "flags", "low_delay", 0);
    av_dict_set(&options, "tune", "zerolatency", 0);
    av_dict_set(&options, "framedrop", "1", 0);
    av_dict_set(&options, "probesize", "32", 0);
    


    // 打开输入流
    if (avformat_open_input(&m_formatCtx, m_url.toUtf8().constData(), nullptr, &options) < 0)
    {
        emit errorOccurred("Failed to open stream");
        return;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        emit errorOccurred("Failed to get stream info");
        return;
    }

    // 查找视频流
    for (unsigned i = 0; i < m_formatCtx->nb_streams; i++)
    {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
            break;
        }
    }

    if (m_videoStream == -1 || !initDecoder())
    {
        emit errorOccurred("No video stream found");
        return;
    }

    if (!initSwsContext())
    {
        emit errorOccurred("Failed to initialize sws context");
        return;
    }

    // 准备解码资源
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgbFrame = av_frame_alloc();
    uint8_t *buffer = nullptr;

    // 分配RGB缓冲区
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStream]->codecpar;
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                                              codecPar->width,
                                              codecPar->height, 1);
    buffer = static_cast<uint8_t *>(av_malloc(bufferSize));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer,
                         AV_PIX_FMT_RGB24, codecPar->width,
                         codecPar->height, 1);

    // 主解码循环
    while (m_running)
    {
        int ret = av_read_frame(m_formatCtx, packet);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF)
                break;
            if (ret != AVERROR(EAGAIN))
            {
                qWarning() << "Read frame error:" << ret;
                break;
            }
            continue;
        }

        if (packet->stream_index == m_videoStream)
        {
            ret = avcodec_send_packet(m_codecCtx, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN))
            {
                qWarning() << "Error sending packet:" << ret;
                av_packet_unref(packet);
                continue;
            }

            while (true)
            {
                ret = avcodec_receive_frame(m_codecCtx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0)
                {
                    qWarning() << "Error receiving frame:" << ret;
                    break;
                }

                // 转换颜色空间
                sws_scale(m_swsCtx,
                          frame->data, frame->linesize,
                          0, codecPar->height,
                          rgbFrame->data, rgbFrame->linesize);

                // 创建QImage并发送
                QImage image(
                    rgbFrame->data[0],
                    codecPar->width,
                    codecPar->height,
                    rgbFrame->linesize[0],
                    QImage::Format_RGB888);

//                emit frameReady(image.copy());
                emit frameReady(QImage(buffer, codecPar->width, codecPar->height, QImage::Format_RGB888));
            }
        }
        av_packet_unref(packet);
    }

    // 清理资源
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&rgbFrame);
    av_packet_free(&packet);
}

bool VideoDecoder::initDecoder()
{
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStream]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        qWarning() << "Unsupported codec:" << codecPar->codec_id;
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, codecPar);

    // 设置低延迟参数
    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
    m_codecCtx->thread_count = 4; // 适当设置解码线程数
    m_codecCtx->thread_type = FF_THREAD_FRAME;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0)
    {
        qWarning() << "Failed to open codec";
        return false;
    }
    return true;
}

bool VideoDecoder::initSwsContext()
{
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStream]->codecpar;
    m_swsCtx = sws_getContext(
        codecPar->width, codecPar->height,
        m_codecCtx->pix_fmt,
        codecPar->width, codecPar->height,
        AV_PIX_FMT_RGB24,
//        SWS_BILINEAR,
                SWS_FAST_BILINEAR,
                nullptr, nullptr, nullptr);
    return m_swsCtx != nullptr;
}

void VideoDecoder::cleanup()
{
    if (m_codecCtx)
    {
        avcodec_close(m_codecCtx);
        avcodec_free_context(&m_codecCtx);
    }
    if (m_formatCtx)
    {
        avformat_close_input(&m_formatCtx);
    }
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
}
