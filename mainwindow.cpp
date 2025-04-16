#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <nlohmann/json.hpp>
#include <QtCore/QDateTime>

// 常量定义统一管理
namespace
{
    constexpr int LH08_BIT_COUNT = 8;
    const QString TIMESTAMP_FORMAT = "yyyy-MM-dd HH:mm:ss.zzz";
    const QMap<QString, QString> BUTTON_STYLES = {
        {"green", "background-color: rgb(0,255,37);"},
        {"red", "background-color: rgb(238,27,37);"},
        {"purple", "background-color: rgb(140, 0, 255);"},
        {"blue", "background-color: rgb(2, 160, 229);"},
        {"yellow", "background-color: rgb(255, 238, 0);"},
        {"cyan", "background-color: rgb(2, 195, 229);"}};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      isPlaying(false),
      isDown(true),
      isFanOpen(false),
      lh08(0x00),
      lh08_buffer(0x00),
      m_canMotorspeedbuffer(0.f),
      m_motor_mode(0),
      show_dev(false)
{
    ui->setupUi(this);

    initializeUI();
    initializeMQTT();
    initializeVideo();
    setupConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeUI()
{
    ui->operatingFlickArea->hide();
    ui->operatingFlickArea->set_window(this);
    setWindowTitle("监控");
    m_videoWidget = ui->videoArea;
}

void MainWindow::initializeMQTT()
{
    m_mqttClient = std::make_unique<MQTTClient>(config);
    m_mqttClient->subscribe(config.subTOPIC, 1);
}

void MainWindow::initializeVideo()
{
    m_decoder = std::make_unique<VideoDecoder>(config);
}

void MainWindow::setupConnections()
{
    connect(m_decoder.get(), &VideoDecoder::frameReady, this, &MainWindow::updateVideoFrame);
    connect(m_decoder.get(), &VideoDecoder::errorOccurred, this, &MainWindow::handleError);

    connect(m_mqttClient.get(), &MQTTClient::errorOccurred,
            this, &MainWindow::handleMQTTError);

    connect(m_mqttClient.get(), &MQTTClient::messageReceived,
            this, &MainWindow::handleMessageReceived);

    setupButtonConnections();
    setupCheckBoxConnections();
}

void MainWindow::setupButtonConnections()
{
    connect(ui->operatingArea->getSwitchMotorModeButton(), &QPushButton::clicked, this, &MainWindow::onSwitchMotorModeClicked);
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

// 统一处理CheckBox连接
void MainWindow::setupCheckBoxConnections()
{
    const QVector<QCheckBox *> checkBoxes = {
        ui->operatingArea->getCheckBox1(),
        ui->operatingArea->getCheckBox2(),
        ui->operatingArea->getCheckBox3(),
        ui->operatingArea->getCheckBox4(),
        ui->operatingArea->getCheckBox5(),
        ui->operatingArea->getCheckBox6(),
        ui->operatingArea->getCheckBox7(),
        ui->operatingArea->getCheckBox8(),
    };

    for (int i = 0; i < LH08_BIT_COUNT; ++i)
    {
        connect(checkBoxes[i], &QCheckBox::stateChanged,
                this, [this, i](int state)
                { handleCheckBoxChanged(i, state); });
    }
}

// 统一处理CheckBox状态变化
void MainWindow::handleCheckBoxChanged(int bitIndex, int state)
{
    const bool checked = (state == Qt::Checked);
    lh08 = (lh08 & ~(1 << bitIndex)) | (checked << bitIndex);
    updateCheckBoxStyle(ui->operatingArea->getCheckBoxes()[bitIndex], checked);
    qDebug() << "lh08 changed: 0x" << QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::updateCheckBoxStyle(QCheckBox *box, bool checked)
{
    box->setStyleSheet(QString("QCheckBox::indicator {   width: 100%;"
                               "height: 100%;"
                               "border: none;"
                               "background: transparent;}"
                               "QCheckBox {%1}")
                           .arg(checked ? BUTTON_STYLES["green"] : BUTTON_STYLES["red"]));
}

// 消息处理逻辑重构
void MainWindow::handleMessageReceived(const QString &topic, const QByteArray &payload)
{
    qDebug() << "[MAIN] Message received on" << topic;

    try
    {
        const auto data = nlohmann::json::parse(payload.toStdString());
        const std::string id = data.value("id", "");

        if (id.empty())
        {
            qWarning() << "Invalid message: missing ID";
            return;
        }

        const auto &ack = data.at("params").at("ack").at("value");
        const auto timestamp = parseTimestamp(ack.get<std::string>());

        qDebug() << QString("[%1] %2 received").arg(timestamp.toString(TIMESTAMP_FORMAT), QString::fromStdString(id));

        // 使用策略模式处理不同消息类型
        static const QHash<QString, std::function<void()>> handlers = {
            {CAN_POS_ID, [&]
             { handleCanPosMessage(data); }},
            {CAN_SPEED_ID, [&]
             { handleCanSpeedMessage(data); }},
            {MODBUS_MOROTR_ID, [&]
             { handleModbusMotorMessage(data); }},
            {LH08_ID, [&]
             { handleLH08Message(data); }},
            {PWM_MOTOR_ID, [&]
             { handlePwmMotorMessage(data); }},
            {CAMERA_ID, [&]
             { handleCameraMessage(data); }},
        };

        const QString qid = QString::fromStdString(id);
        if (handlers.contains(qid))
        {
            handlers[qid]();
        }
        else
        {
            qWarning() << "Unhandled message type:" << qid;
        }
    }
    catch (const nlohmann::json::exception &e)
    {
        qCritical() << "JSON error:" << e.what();
    }
    catch (const std::exception &e)
    {
        qCritical() << "Error processing message:" << e.what();
    }
}

// 时间戳解析封装
QDateTime MainWindow::parseTimestamp(const std::string &timestampStr) const
{
    const qint64 ms = std::stoll(timestampStr);
    return QDateTime::fromMSecsSinceEpoch(ms, Qt::UTC);
}

// 各类型消息处理函数
void MainWindow::handleCanPosMessage(const nlohmann::json &data)
{
    float angle = data["params"]["angle"]["value"];
    int speed = data["params"]["speed"]["value"];
    int cur_max = data["params"]["current"]["value"];
    int mode = data["params"]["mode"]["value"];
    qDebug() << "[MAINWINDOW] 已接收：\nangle: " << angle << "\nspeed: " << speed << "\ncur_max: " << cur_max << "\nmode: " << mode;

    const auto buttons = {
        ui->operatingArea->getForwardButton(),
        ui->operatingArea->getBackwardButton(),
        ui->operatingArea->getStopMovingButton(),
        ui->operatingArea->getSwitchMotorModeButton()};
    for (auto btn : buttons)
    {
        btn->setEnabled(true);
    }
}

void MainWindow::handleCanSpeedMessage(const nlohmann::json &data)
{
    m_canMotorspeedbuffer = data["params"]["speed2"]["value"];
    qDebug() << "[MAINWINDOW] m_canMotorspeedbuffer 已接收：" << m_canMotorspeedbuffer;

    const auto buttons = {
        ui->operatingArea->getForwardButton(),
        ui->operatingArea->getBackwardButton(),
        ui->operatingArea->getStopMovingButton(),
        ui->operatingArea->getSwitchMotorModeButton()};

    for (auto btn : buttons)
    {
        btn->setEnabled(true);
    }
}

void MainWindow::handleModbusMotorMessage(const nlohmann::json &data)
{
    bool isDown_r = false;
    if (data["params"]["angle485_1"]["value"] == config.MOTOR485_1_DOWN_POS && data["params"]["angle485_2"]["value"] == config.MOTOR485_2_DOWN_POS)
    {
        isDown_r = true;
    }
    else if (data["params"]["angle485_1"]["value"] == config.MOTOR485_1_UP_POS && data["params"]["angle485_2"]["value"] == config.MOTOR485_2_UP_POS)
    {
        isDown_r = false;
    }
    else
    {
        qDebug() << "[MAINWINDOW] MODBUS_MOROTR_ID ack is error!!!! ";
        ui->operatingArea->getUp8DownButton()->setEnabled(true);
        return;
    }
    if (isDown_r != isDown)
    {
        toggleButtonState(isDown,
                          ui->operatingArea->getUp8DownButton(),
                          "毛刷降",
                          "毛刷升",
                          BUTTON_STYLES["purple"],
                          BUTTON_STYLES["blue"]);
    }
    else
    {
        qDebug() << "[MAINWINDOW] angle not change. isDown: " << isDown << "\t isDown_r: " << isDown_r;
        ui->operatingArea->getUp8DownButton()->setEnabled(true);
    }
}

void MainWindow::handleLH08Message(const nlohmann::json &data)
{
    lh08_buffer = std::stoi(data["params"]["status"]["value"].get<std::string>(), nullptr, 16);
    qDebug() << "LH08 buffer updated: 0x" << QString::number(lh08_buffer, 16);

    const auto buttons = {
        ui->operatingArea->getOpenSelectedButton(),
        ui->operatingArea->getCloseSelectedButton(),
        ui->operatingArea->getOpen8CloseButton()};

    for (auto btn : buttons)
    {
        btn->setEnabled(true);
    }
}

void MainWindow::handlePwmMotorMessage(const nlohmann::json &data)
{
    bool isFanOpen_r = false;
    if (data["params"]["duty"]["value"] > 5.2f)
    {
        isFanOpen_r = true;
    }
    else if (data["params"]["duty"]["value"] < 0.02f)
    {
        isFanOpen_r = false;
    }
    else
    {
        return;
    }

    if (isFanOpen_r != isFanOpen)
    {
        toggleButtonState(isFanOpen,
                          ui->operatingArea->getFanCtrlButton(),
                          "推进器开",
                          "推进器关",
                          BUTTON_STYLES["yellow"],
                          BUTTON_STYLES["cyan"]);
    }
}

void MainWindow::handleCameraMessage(const nlohmann::json &data)
{
    const bool isPlaying_r = data["params"]["Camera_state"]["value"] == 1;
    if (isPlaying_r != isPlaying)
    {
        toggleButtonState(isPlaying,
                          ui->operatingArea->getPlay8StopButton(),
                          "摄像头开",
                          "摄像头关",
                          BUTTON_STYLES["green"],
                          BUTTON_STYLES["red"]);
    }
}

// 按钮状态切换
void MainWindow::toggleButtonState(bool &state, QPushButton *btn,
                                   const QString &textOn, const QString &textOff,
                                   const QString &styleOn, const QString &styleOff)
{
    state = !state;
    btn->setText(state ? textOff : textOn);
    btn->setStyleSheet(state ? styleOff : styleOn);
    btn->setEnabled(true);
    qDebug() << "Button" << btn->text() << "toggled to" << (state ? "OFF" : "ON");
}

// 统一处理电机控制消息
void MainWindow::sendMotorControl(float value, MotorControlType_e type)
{
    std::map<QString, nlohmann::json> fields;

    switch (type)
    {
    case SpeedControl:
        fields["speed2"] = value;
        fields["current"] = config.MOTOR_CURRENT;
        fields["mode"] = 1;
        break;

    case PositionControl:
        fields["angle"] = value;
        fields["current"] = config.MOTOR_CURRENT;
        fields["speed"] = config.ANGLE_SPEED;
        fields["mode"] = 1;
        break;
    }

    m_mqttClient->publishMqttMessage(fields, type == SpeedControl ? CAN_SPEED_ID : CAN_POS_ID);
}

void MainWindow::onForwardClicked()
{
    ui->operatingArea->getForwardButton()->setEnabled(false);
    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(false);

    const float value = m_motor_mode ? (m_canMotorspeedbuffer + config.SPEED_DELTA) : config.ANGLE_DELTA;

    sendMotorControl(value, m_motor_mode ? SpeedControl : PositionControl);
}

void MainWindow::onBackwardClicked()
{
    ui->operatingArea->getBackwardButton()->setEnabled(false);
    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(false);

    const float value = m_motor_mode ? (m_canMotorspeedbuffer - config.SPEED_DELTA) : -config.ANGLE_DELTA;

    sendMotorControl(value, m_motor_mode ? SpeedControl : PositionControl);
}

void MainWindow::onStopMovingClicked()
{
    ui->operatingArea->getStopMovingButton()->setEnabled(false);
    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(false);

    sendMotorControl(0, m_motor_mode ? SpeedControl : PositionControl);
}

void MainWindow::onFanCtrlClicked()
{
    ui->operatingArea->getFanCtrlButton()->setEnabled(false);
    std::map<QString, nlohmann::json> fields;
    fields["duty"] = !isFanOpen ? 5.3f : 0.01f;

    m_mqttClient->publishMqttMessage(fields, PWM_MOTOR_ID);
}

void MainWindow::onGPSClicked()
{
    qDebug() << "[MAINWINDOW] Clicked [GPS] button.";
}

void MainWindow::onOpenSelectedClicked()
{
    ui->operatingArea->getOpenSelectedButton()->setEnabled(false);
    qDebug() << "[MAINWINDOW] lh08:" << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0')
             << ", lh08_buffer:" << "0x" + QString::number(lh08_buffer, 16).toUpper().rightJustified(2, '0');
    std::map<QString, nlohmann::json> fields;
    char str[4];
    sprintf(str, "%02x", (lh08 | lh08_buffer));
    fields["status"] = str;

    m_mqttClient->publishMqttMessage(fields, LH08_ID);
}

void MainWindow::onCloseSelectedClicked()
{
    ui->operatingArea->getCloseSelectedButton()->setEnabled(false);
    qDebug() << "[MAINWINDOW] lh08:" << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0')
             << ", lh08_buffer:" << "0x" + QString::number(lh08_buffer, 16).toUpper().rightJustified(2, '0');
    std::map<QString, nlohmann::json> fields;
    char str[4];
    sprintf(str, "%02x", (~lh08 & lh08_buffer));
    fields["status"] = str;

    m_mqttClient->publishMqttMessage(fields, LH08_ID);
}

void MainWindow::onOpen8CloseClicked()
{
    ui->operatingArea->getOpen8CloseButton()->setEnabled(false);
    qDebug() << "[MAINWINDOW] lh08:" << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0')
             << ", lh08_buffer:" << "0x" + QString::number(lh08_buffer, 16).toUpper().rightJustified(2, '0');
    std::map<QString, nlohmann::json> fields;
    char str[4];
    sprintf(str, "%02x", (lh08));
    fields["status"] = str;

    m_mqttClient->publishMqttMessage(fields, LH08_ID);
}

// 设备切换按钮优化
void MainWindow::on_devBtn_clicked()
{
    show_dev = !show_dev;
    ui->operatingArea->setVisible(!show_dev);
    ui->operatingFlickArea->setVisible(show_dev);
}

// 错误处理统一封装
void MainWindow::handleError(const QString &message)
{
    QMessageBox::critical(this, "Error", message);
    if (m_decoder)
    {
        m_decoder->stop();
    }
}

void MainWindow::handleMQTTError(const QString &msg, bool maxPublishFlag)
{
    if (maxPublishFlag)
    {
        const auto buttons = {
            ui->operatingArea->getForwardButton(),
            ui->operatingArea->getBackwardButton(),
            ui->operatingArea->getStopMovingButton(),
            ui->operatingArea->getSwitchMotorModeButton(),
            ui->operatingArea->getUp8DownButton(),
            ui->operatingArea->getPlay8StopButton(),
            ui->operatingArea->getFanCtrlButton(),
            ui->operatingArea->getOpenSelectedButton(),
            ui->operatingArea->getCloseSelectedButton(),
            ui->operatingArea->getOpen8CloseButton()};

        for (auto btn : buttons)
        {
            btn->setEnabled(true);
        }
    }
    QMessageBox::critical(this, "MQTT Error", msg);
}

void MainWindow::updateVideoFrame(const QImage &frame)
{
    m_videoWidget->setFrame(frame);
}

void MainWindow::onSwitchMotorModeClicked()
{
    m_motor_mode = m_motor_mode ? 0 : 1;

    QString btn1 = m_motor_mode ? "加速" : "前进";
    QString btn2 = m_motor_mode ? "减速" : "后退";
    ui->operatingArea->getForwardButton()->setText(btn1);
    ui->operatingArea->getBackwardButton()->setText(btn2);
}

void MainWindow::onPlay8StopClicked()
{
    ui->operatingArea->getPlay8StopButton()->setEnabled(false);
    std::map<QString, nlohmann::json> fields;
    fields["Camera_state"] = !isPlaying ? 1 : 0;

    m_mqttClient->publishMqttMessage(fields, CAMERA_ID);

    // 启动视频解码线程
    m_decoder->start();
}

void MainWindow::onUp8DownClicked()
{
    ui->operatingArea->getUp8DownButton()->setEnabled(false);
    std::map<QString, nlohmann::json> fields;
    fields["angle485_1"] = isDown ? config.MOTOR485_1_UP_POS : config.MOTOR485_1_DOWN_POS;
    fields["angle485_2"] = isDown ? config.MOTOR485_2_UP_POS : config.MOTOR485_2_DOWN_POS;
    fields["type"] = 1;

    m_mqttClient->publishMqttMessage(fields, MODBUS_MOROTR_ID);
}

void MainWindow::handleInputsAccepted(const QStringList &inputs, dialog_type_e type)
{
    qDebug() << "type: " << type;
    std::map<QString, nlohmann::json> fields;
    switch (type)
    {
    case MOTOR_CAN_ANGLE_DIALOG:
        fields["angle"] = inputs.at(0).toFloat();
        fields["current"] = inputs.at(1).toInt();
        fields["speed"] = inputs.at(2).toInt();
        fields["mode"] = inputs.at(3).toInt();

        qDebug() << "Angle: " << fields["angle"].dump().c_str();
        qDebug() << "Current: " << fields["current"].dump().c_str();
        qDebug() << "Speed: " << fields["speed"].dump().c_str();
        qDebug() << "Mode: " << fields["mode"].dump().c_str();

        m_mqttClient->publishMqttMessage(fields, CAN_POS_ID);
        break;

    case MOTOR_CAN_SPEED_DIALOG:
        fields["speed2"] = inputs.at(0).toFloat();
        fields["current"] = inputs.at(1).toInt();
        fields["mode"] = inputs.at(2).toInt();

        qDebug() << "Speed: " << fields["speed2"].dump().c_str();
        qDebug() << "Current: " << fields["current"].dump().c_str();
        qDebug() << "Mode: " << fields["mode"].dump().c_str();

        m_mqttClient->publishMqttMessage(fields, CAN_SPEED_ID);
        break;

    case MOTOR_485_DIALOG:
        fields["angle485_1"] = inputs.at(0).toFloat();
        fields["angle485_2"] = inputs.at(1).toInt();
        fields["type"] = inputs.at(2).toInt();

        qDebug() << "Angle_1: " << fields["angle485_1"].dump().c_str();
        qDebug() << "Angle_2: " << fields["angle485_2"].dump().c_str();
        qDebug() << "Type: " << fields["type"].dump().c_str();

        m_mqttClient->publishMqttMessage(fields, MODBUS_MOROTR_ID);
        break;

    case LH08_DIALOG:
        char str[4];
        sprintf(str, "%02x", inputs.at(0).toUInt(nullptr, 2));
        fields["status"] = str;

        qDebug() << "Status: " << fields["status"].dump().c_str();

        m_mqttClient->publishMqttMessage(fields, LH08_ID);
        break;

    case PWM_DIALOG:
        fields["duty"] = inputs.at(0).toFloat();

        qDebug() << "Duty: " << fields["duty"].dump().c_str();

        m_mqttClient->publishMqttMessage(fields, PWM_MOTOR_ID);
        break;

    default:
        break;
    }
}
