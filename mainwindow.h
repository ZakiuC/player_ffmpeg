#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "video_decoder.h"
#include "videowidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief 更新视频帧显示
     * @param frame 解码后的图像
     */
    void updateVideoFrame(const QImage &frame);

    /**
     * @brief 处理播放错误
     * @param message 错误描述信息
     */
    void handleError(const QString &message);

private:
    Ui::MainWindow *ui;
    VideoDecoder *m_decoder;    // 视频解码器对象，负责从流中解码视频帧
    VideoWidget *m_videoWidget; // 视频显示组件，用于在界面上显示视频
};

#endif // MAINWINDOW_H
