#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
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

    void handleMessageReceived(const QString &topic, const QByteArray &payload);

    void toggleButtonState(bool &state, QPushButton *btn, const QString &textOn, const QString &textOff, const QString &styleOn, const QString &styleOff);
    void connectButtons();

    void onPlay8StopClicked();
    void onUp8DownClicked();
    void onForwardClicked();
    void onBackwardClicked();
    void onStopMovingClicked();
    void onFanCtrlClicked();
    void onGPSClicked();
    void onOpenSelectedClicked();
    void onCloseSelectedClicked();
    void onOpen8CloseClicked();

private:
    Ui::MainWindow *ui;
    AppConfig config;           // 应用配置
    bool isPlaying;             // 视频是否正在播放
    bool isDown;                // 毛刷是否在下面
    bool isFanOpen;             // 推进器是否开启
    VideoDecoder *m_decoder;    // 视频解码器对象，负责从流中解码视频帧
    VideoWidget *m_videoWidget; // 视频显示组件，用于在界面上显示视频
    MQTTClient *m_mqttClient;   // mqtt
};

#endif // MAINWINDOW_H
