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
    void setFrame(const QImage &frame);
    QSize videoSize() const { return m_videoSize; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_currentFrame; // 当前视频帧
    QSize m_videoSize;     // 视频原始尺寸
    QMutex m_mutex;        // 互斥锁用于线程安全
};

#endif // VIDEOWIDGET_H
