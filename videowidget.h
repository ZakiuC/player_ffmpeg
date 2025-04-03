#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMutex>

/**
 * @brief 用于显示视频帧
 */
class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);

    /**
     * @brief 设置当前要显示的视频帧图像
     * @param frame 视频帧图像
     */
    void setFrame(const QImage &frame);

    /**
     * @brief 获取当前视频帧的尺寸
     * @return 视频帧尺寸
     */
    QSize videoSize() const { return m_videoSize; }

protected:
    /**
     * @brief 重写绘制事件，用于在窗口中绘制视频帧图像
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_currentFrame; // 存储当前要显示的视频帧图像
    QSize m_videoSize;     // 记录视频帧的原始尺寸
    QMutex m_mutex;        // 互斥锁，保护视频帧数据，确保多线程安全
};

#endif // VIDEOWIDGET_H
