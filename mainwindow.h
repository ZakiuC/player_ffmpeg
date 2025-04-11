#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "video_decoder.h"
#include "videowidget.h"
#include "mqtt_client.h"
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateVideoFrame(const QImage &frame);
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
    bool isPlaying;             // 视频播放状态
    bool isDown;                // 毛刷位置状态
    bool isFanOpen;             // 推进器开关状态
    std::unique_ptr<VideoDecoder> m_decoder;  // 视频解码器
    VideoWidget *m_videoWidget;               // 视频显示控件
    std::unique_ptr<MQTTClient> m_mqttClient;   // MQTT 通信客户端

    float m_canMotorspeedbuffer;
};

#endif // MAINWINDOW_H
