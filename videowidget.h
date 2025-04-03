#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMutex>

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
    QImage m_currentFrame;
    QSize m_videoSize;
    QMutex m_mutex;
};

#endif // VIDEOWIDGET_H
