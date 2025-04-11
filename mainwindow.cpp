#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <nlohmann/json.hpp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      isPlaying(false),
      isDown(true),
      isFanOpen(false)
{
    ui->setupUi(this);

    // 初始化 MQTT 客户端和视频解码器（使用智能指针管理）
    m_mqttClient = std::make_unique<MQTTClient>(config);
    m_mqttClient->subscribe(config.subTOPIC, 1);

    m_decoder = std::make_unique<VideoDecoder>(config);

    // 获取 UI 中的视频显示控件
    m_videoWidget = ui->videoArea;

    setWindowTitle("RTMP Player");

    connect(m_decoder.get(), &VideoDecoder::frameReady, this, &MainWindow::updateVideoFrame);
    connect(m_decoder.get(), &VideoDecoder::errorOccurred, this, &MainWindow::handleError);
    connect(m_mqttClient.get(), &MQTTClient::errorOccurred, this, [this](const QString &msg)
    { QMessageBox::critical(this, "MQTT Error", msg); });
    connect(m_mqttClient.get(), &MQTTClient::messageReceived, this, &MainWindow::handleMessageReceived);

    connectButtons();
}

MainWindow::~MainWindow()
{
    if (m_decoder)
    {
        m_decoder->stop();
    }
    delete ui;
}

void MainWindow::updateVideoFrame(const QImage &frame)
{
    m_videoWidget->setFrame(frame);
}

void MainWindow::handleError(const QString &message)
{
    QMessageBox::critical(this, "Playback Error", message);
    m_decoder->stop();
}

void MainWindow::handleMessageReceived(const QString &topic, const QByteArray &payload)
{
    qDebug() << "[MAINWINDOW] Received message on" << topic;
    nlohmann::json data = nlohmann::json::parse(payload.toStdString());
    std::string id;
    if (data.contains("id") && !data["id"].is_null()) {
        id = data["id"].get<std::string>();
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
        long long timestamp_ms = std::stoll(timestampStr);
        time_t seconds = timestamp_ms / 1000;
        int milliseconds = timestamp_ms % 1000;
        std::tm* ptm = std::gmtime(&seconds);
        char buffer[32] = {0};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);

        QString formattedTimestamp = QString("时间戳为：%1.%2 UTC")
            .arg(QString::fromUtf8(buffer))
            .arg(milliseconds, 3, 10, QChar('0'));

        qDebug() << "[MAINWINDOW] id:" << QString::fromStdString(id) << "已接收" << formattedTimestamp;
    } else {
        qDebug() << "[MAINWINDOW] ack 不存在或者为 null";
    }
}

void MainWindow::onPlay8StopClicked()
{
    toggleButtonState(isPlaying, ui->operatingArea->getPlay8StopButton(), "摄像头开", "摄像头关", "background-color: rgb(0,255,0);", "background-color: rgb(255,0,0);");
    std::map<QString, nlohmann::json> fields;
    fields["Camera_state"] = isPlaying ? 1 : 0;

    m_mqttClient->publishMqttMessage(fields, "006");

    // 启动视频解码线程
    m_decoder->start();
}

void MainWindow::onUp8DownClicked()
{
    toggleButtonState(isDown, ui->operatingArea->getUp8DownButton(), "毛刷降", "毛刷升", "background-color: rgb(140, 0, 255);", "background-color: rgb(2, 160, 229);");
    std::map<QString, nlohmann::json> fields;
    fields["angle485"] = isDown ? 90 : -90;
    fields["type"]     = 1;

    m_mqttClient->publishMqttMessage(fields, "003");
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
    std::map<QString, nlohmann::json> fields;
    fields["duty"] = isFanOpen ? 5.3f : 0.01f;

    m_mqttClient->publishMqttMessage(fields, "005");
}

void MainWindow::onGPSClicked()
{
    qDebug() << "[MAINWINDOW] Clicked [GPS] button.";
}

void MainWindow::onOpenSelectedClicked()
{
    std::map<QString, nlohmann::json> fields;
    fields["status"] = 0x01;

    m_mqttClient->publishMqttMessage(fields, "004");
}

void MainWindow::onCloseSelectedClicked()
{
    std::map<QString, nlohmann::json> fields;
    fields["status"] = 0x00;

    m_mqttClient->publishMqttMessage(fields, "004");
}

void MainWindow::onOpen8CloseClicked()
{
    std::map<QString, nlohmann::json> fields;
    fields["status"] = 0x02;

    m_mqttClient->publishMqttMessage(fields, "004");
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
