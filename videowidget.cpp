#include "videowidget.h"
#include <QPainter>

VideoWidget::VideoWidget(QWidget *parent):QWidget(parent)
{
    // 设置控件属性，启用不透明绘制以提高显示性能
    setAttribute(Qt::WA_OpaquePaintEvent);
    // 设置控件大小策略为可扩展，确保视频显示区域能自适应窗口大小
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoWidget::setFrame(const QImage &frame)
{
    // 检查传入的视频帧是否有效
    if (!frame.isNull())
    {
        // 使用互斥锁保护数据，确保线程安全
        QMutexLocker locker(&m_mutex);
        m_currentFrame = frame;     // 更新当前视频帧
        m_videoSize = frame.size(); // 更新视频帧尺寸信息
        update();                   // 触发重绘事件，刷新界面显示
    }
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);        // 忽略未使用的事件参数
    QPainter painter(this); // 创建绘图对象，用于绘制当前控件
    // 设置平滑像素转换，保证缩放时图像质量更高
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 如果存在有效的视频帧，则绘制视频内容
    if (!m_currentFrame.isNull())
    {
        // 使用互斥锁确保线程安全，防止数据竞争
        QMutexLocker locker(&m_mutex);

        // 获取控件当前大小
        QSize widgetSize = size();
        // 计算等比例缩放后的尺寸，保持原视频宽高比
        QSize scaledSize = m_videoSize.scaled(widgetSize, Qt::KeepAspectRatio);

        // 根据缩放尺寸创建绘制矩形，并将其居中放置在控件中
        QRect drawRect(QPoint(0, 0), scaledSize);
        drawRect.moveCenter(rect().center());

        // 将当前视频帧转换为 QPixmap 后绘制到指定区域
        painter.drawPixmap(drawRect, QPixmap::fromImage(m_currentFrame));
    }
    else
    {
        // 如果没有视频帧，则填充整个控件为黑色背景
        painter.fillRect(rect(), Qt::black);
    }
}
