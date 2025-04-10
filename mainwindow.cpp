#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>

#include <nlohmann/json.hpp>
#include <mqtt/async_client.h>

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    isPlaying = false;
    isDown = true;
    isFanOpen = false;
    // 初始化MQTT客户端
    m_mqttClient = new MQTTClient(config);
    // 订阅主题
    m_mqttClient->subscribe(config.subTOPIC, 1);

    // 初始化视频解码器
    m_decoder = new VideoDecoder(config);

    // 从 UI 中获取视频显示区域组件，用于显示解码后的视频帧
    m_videoWidget = ui->videoArea;

    // 设置窗口标题
    setWindowTitle("RTMP Player");

    // 连接解码器的 frameReady 信号到 updateVideoFrame 槽函数，
    // 当解码器解码出一帧图像时更新视频显示
    connect(m_decoder, &VideoDecoder::frameReady, this, &MainWindow::updateVideoFrame);

    // 连接解码器的 errorOccurred 信号到 handleError 槽函数，
    // 当解码过程中发生错误时提示用户
    connect(m_decoder, &VideoDecoder::errorOccurred, this, &MainWindow::handleError);

    connect(m_mqttClient, &MQTTClient::errorOccurred, this, [this](const QString &msg)
            { QMessageBox::critical(this, "MQTT Error", msg); });

    connect(m_mqttClient, &MQTTClient::messageReceived, this, &MainWindow::handleMessageReceived);

    // 将按钮点击事件与槽函数连接
    connectButtons();
}

MainWindow::~MainWindow()
{
    // 如果解码器存在，则先停止解码线程，再释放内存
    if (m_decoder)
    {
        m_decoder->stop();
        delete m_decoder;
    }

    delete ui;
}

void MainWindow::updateVideoFrame(const QImage &frame)
{
    // 将新的视频帧传递给视频显示组件，触发界面重绘
    m_videoWidget->setFrame(frame);
}

void MainWindow::handleError(const QString &message)
{
    // 弹出错误消息对话框，通知用户播放错误信息
    QMessageBox::critical(this, "Playback Error", message);
    // 出现错误后停止视频解码线程
    m_decoder->stop();
}
void MainWindow::handleMessageReceived(const QString &topic, const QByteArray &payload)
{
    qDebug() << "[MAINWINDOW] Received message on" << topic;
    nlohmann::json data = nlohmann::json::parse(payload.toStdString());
//    qDebug() << QString::fromStdString(data.dump(4));
    std::string id;
    if (data.contains("id") && !data["id"].is_null()) {
        id = data["id"].get<std::string>();
//        qDebug() << "id: " << QString::fromStdString(id);
    } else {
        qDebug() << "[MAINWINDOW] id 不存在或者为 null";
        return;
    }
    std::string timestampStr;
    if (data.contains("params") &&
        data["params"].contains("ack") &&
        data["params"]["ack"].contains("value") &&
        !data["params"]["ack"]["value"].is_null())
    {
        timestampStr = data["params"]["ack"]["value"].get<std::string>();
//        qDebug() << "ack: " << QString::fromStdString(timestampStr);
        // 将字符串转换为 long long 类型（毫秒）
        long long timestamp_ms = std::stoll(timestampStr);

        // 计算秒和毫秒部分
        time_t seconds = timestamp_ms / 1000;         // 秒数
        int milliseconds = timestamp_ms % 1000;         // 毫秒部分

        // 将秒数转换为 UTC 时间结构体
        std::tm* ptm = std::gmtime(&seconds);

        // 格式化时间字符串，格式为 "YYYY-MM-DD HH:MM:SS"
        char buffer[32] = {0};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);

        QString formattedTimestamp = QString("时间戳为：%1.%2 UTC")
            .arg(QString::fromUtf8(buffer))
            .arg(milliseconds, 3, 10, QChar('0'));

        qDebug() << "[MAINWINDOW] id: " << QString::fromStdString(id) << "已接收 " << formattedTimestamp;
    } else {
        qDebug() << "[MAINWINDOW] ack 不存在或者为 null";
    }
}

void MainWindow::onPlay8StopClicked()
{
    toggleButtonState(isPlaying, ui->operatingArea->getPlay8StopButton(), "摄像头开", "摄像头关", "background-color: rgb(0,255,0);", "background-color: rgb(255,0,0);");
    m_mqttClient->publishMqttMessage("Camera_state", isPlaying ? 1 : 0, "005");

    // 启动视频解码线程，开始实时解码视频流
    m_decoder->start();
}

void MainWindow::onUp8DownClicked()
{
    toggleButtonState(isDown, ui->operatingArea->getUp8DownButton(), "毛刷降", "毛刷升", "background-color: rgb(140, 0, 255);", "background-color: rgb(2, 160, 229);");
    m_mqttClient->publishMqttMessage("angle485", isDown ? 90 : -90, "002");
}

void MainWindow::onForwardClicked()
{
    m_mqttClient->publishMovementParams(config.TOPIC, "thing.event.property.post", "001", 90, 10, 20, 1);
}

void MainWindow::onBackwardClicked()
{
    m_mqttClient->publishMovementParams(config.TOPIC, "thing.event.property.post", "001", -90, 10, 20, 1);
}

void MainWindow::onStopMovingClicked()
{
    qDebug() << "[MAINWINDOW] Clicked [停止] button.";
}

void MainWindow::onFanCtrlClicked()
{
    toggleButtonState(isFanOpen, ui->operatingArea->getFanCtrlButton(), "推进器开", "推进器关", "background-color: rgb(255, 238, 0);", "background-color: rgb(2, 195, 229);");
    m_mqttClient->publishMqttMessage("duty", isFanOpen ? 5.3f : 0.01f, "004");
}

void MainWindow::onGPSClicked()
{
    qDebug() << "[MAINWINDOW] Clicked [GPS] button.";
}

void MainWindow::onOpenSelectedClicked()
{
    m_mqttClient->publishMqttMessage("status", "0x01", "003");
}

void MainWindow::onCloseSelectedClicked()
{
    m_mqttClient->publishMqttMessage("status", "0x00", "003");
}

void MainWindow::onOpen8CloseClicked()
{
    m_mqttClient->publishMqttMessage("status", "0x02", "003");
}

void MainWindow::toggleButtonState(bool &state, QPushButton *btn, const QString &textOn, const QString &textOff, const QString &styleOn, const QString &styleOff)
{
    state = !state;
    qDebug() << "[MAINWINDOW] Clicked [" << btn->text() << "] button.";

    if (state)
    {
        btn->setText(textOff);
        btn->setStyleSheet(styleOff);
    }
    else
    {
        btn->setText(textOn);
        btn->setStyleSheet(styleOn);
    }
}

void MainWindow::connectButtons()
{
    connect(ui->operatingArea->getPlay8StopButton(), &QPushButton::clicked, this, &MainWindow::onPlay8StopClicked);
    connect(ui->operatingArea->getUp8DownButton(), &QPushButton::clicked, this, &MainWindow::onUp8DownClicked);
    connect(ui->operatingArea->getForwardButton(), &QPushButton::clicked, this, &MainWindow::onForwardClicked);
    connect(ui->operatingArea->getBackwardButton(), &QPushButton::clicked, this, &MainWindow::onBackwardClicked);
    connect(ui->operatingArea->getStopMovingButton(), &QPushButton::clicked, this, &MainWindow::onStopMovingClicked);
    connect(ui->operatingArea->getFanCtrlButton(), &QPushButton::clicked, this, &MainWindow::onFanCtrlClicked);
    connect(ui->operatingArea->getGPSButton(), &QPushButton::clicked, this, &MainWindow::onGPSClicked);
    connect(ui->operatingArea->getOpenSelectedButton(), &QPushButton::clicked, this, &MainWindow::onOpenSelectedClicked);
    connect(ui->operatingArea->getCloseSelectedButton(), &QPushButton::clicked, this, &MainWindow::onCloseSelectedClicked);
    connect(ui->operatingArea->getOpen8CloseButton(), &QPushButton::clicked, this, &MainWindow::onOpen8CloseClicked);
}
