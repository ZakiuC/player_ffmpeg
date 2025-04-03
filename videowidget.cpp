#include "videowidget.h"
#include <QPainter>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoWidget::setFrame(const QImage &frame)
{
    if(!frame.isNull()) {
        QMutexLocker locker(&m_mutex);
        m_currentFrame = frame;
        m_videoSize = frame.size();
        update();
    }
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if(!m_currentFrame.isNull()) {
        QMutexLocker locker(&m_mutex);

        // 计算保持宽高比的缩放
        QSize widgetSize = size();
        QSize scaledSize = m_videoSize.scaled(widgetSize, Qt::KeepAspectRatio);

        QRect drawRect(QPoint(0,0), scaledSize);
        drawRect.moveCenter(rect().center());

//        painter.drawImage(drawRect, m_currentFrame);
        painter.drawPixmap(drawRect, QPixmap::fromImage(m_currentFrame));
    } else {
        painter.fillRect(rect(), Qt::black);
    }
}
