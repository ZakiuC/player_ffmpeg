#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "video_decoder.h"
#include "videowidget.h"
#include "mqtt_client.h"

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

    /**
     * @brief 播放/暂停按钮点击事件
     */
    void on_play8pauseBtn_clicked();

    /**
     * @brief 停止按钮点击事件
     */
    void on_stopBtn_clicked();

private:
    Ui::MainWindow *ui;
    AppConfig config;           // 应用配置
    VideoDecoder *m_decoder;    // 视频解码器对象，负责从流中解码视频帧
    VideoWidget *m_videoWidget; // 视频显示组件，用于在界面上显示视频
    MQTTClient *m_mqttClient;   // mqtt
};

#endif // MAINWINDOW_H
