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
      isFanOpen(false),
      lh08(0x00),
      lh08_buffer(0x00),
      m_canMotorspeedbuffer(0.f),
      m_motor_mode(0),
      show_dev(false)
{
    ui->setupUi(this);

    ui->operatingFlickArea->hide();
    ui->operatingFlickArea->set_window(this);

    // 初始化 MQTT 客户端和视频解码器（使用智能指针管理）
    m_mqttClient = std::make_unique<MQTTClient>(config);
    m_mqttClient->subscribe(config.subTOPIC, 1);

    m_decoder = std::make_unique<VideoDecoder>(config);

    // 获取 UI 中的视频显示控件
    m_videoWidget = ui->videoArea;

    setWindowTitle("RTMP Player");

    connect(m_decoder.get(), &VideoDecoder::frameReady, this, &MainWindow::updateVideoFrame);
    connect(m_decoder.get(), &VideoDecoder::errorOccurred, this, &MainWindow::handleError);
    connect(m_mqttClient.get(), &MQTTClient::errorOccurred, this, [this](const QString &msg, bool maxPublishFlag)
            {
                if(maxPublishFlag)
                {
                    ui->operatingArea->getForwardButton()->setEnabled(true);
                    ui->operatingArea->getBackwardButton()->setEnabled(true);
                    ui->operatingArea->getStopMovingButton()->setEnabled(true);
                    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(true);
                    ui->operatingArea->getUp8DownButton()->setEnabled(true);
                    ui->operatingArea->getPlay8StopButton()->setEnabled(true);
                    ui->operatingArea->getFanCtrlButton()->setEnabled(true);
                    ui->operatingArea->getOpenSelectedButton()->setEnabled(true);
                    ui->operatingArea->getCloseSelectedButton()->setEnabled(true);
                    ui->operatingArea->getOpen8CloseButton()->setEnabled(true);
                }
                QMessageBox::critical(this, "MQTT Error", msg); });
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

void MainWindow::handleMessageReceived(const QString &topic, const QByteArray &payload)
{
    qDebug() << "[MAINWINDOW] Received message on" << topic;
    nlohmann::json data = nlohmann::json::parse(payload.toStdString());
    std::string id;
    if (data.contains("id") && !data["id"].is_null())
    {
        id = data["id"].get<std::string>();
    }
    else
    {
        qDebug() << "[MAINWINDOW] id 不存在或者为 null";
        return;
    }
    std::string timestampStr;
    try
    {
        if (data.contains("params") &&
            data["params"].contains("ack") &&
            data["params"]["ack"].contains("value") &&
            !data["params"]["ack"]["value"].is_null())
        {
            timestampStr = data["params"]["ack"]["value"].get<std::string>();
            long long timestamp_ms = std::stoll(timestampStr);
            time_t seconds = timestamp_ms / 1000;
            int milliseconds = timestamp_ms % 1000;
            std::tm *ptm = std::gmtime(&seconds);
            char buffer[32] = {0};
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);

            QString formattedTimestamp = QString("时间戳为：%1.%2 UTC")
                                             .arg(QString::fromUtf8(buffer))
                                             .arg(milliseconds, 3, 10, QChar('0'));

            qDebug() << "[MAINWINDOW] id:" << QString::fromStdString(id) << "已接收" << formattedTimestamp;

            if (id == CAMERA_ID)
            {
                bool isPlaying_r = false;
                if (data["params"]["Camera_state"]["value"] == 1)
                {
                    isPlaying_r = true;
                }
                else if (data["params"]["Camera_state"]["value"] != 0)
                {
                    return;
                }
                if (isPlaying_r != isPlaying)
                {
                    toggleButtonState(isPlaying, ui->operatingArea->getPlay8StopButton(), "摄像头开", "摄像头关", "background-color: rgb(0,255,0);", "background-color: rgb(255,0,0);");
                }
            }
            else if (id == LH08_ID)
            {
                std::string lh08_r = data["params"]["status"]["value"];
                lh08_buffer = std::stoi(lh08_r, nullptr, 16);
                qDebug() << QString("[MAINWINDOW] lh08_buffer 已接收：0x%1")
                                .arg(lh08_buffer, 2, 16, QChar('0'));
                ui->operatingArea->getOpenSelectedButton()->setEnabled(true);
                ui->operatingArea->getCloseSelectedButton()->setEnabled(true);
                ui->operatingArea->getOpen8CloseButton()->setEnabled(true);
            }
            else if (id == MODBUS_MOROTR_ID)
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
                    toggleButtonState(isDown, ui->operatingArea->getUp8DownButton(), "毛刷降", "毛刷升", "background-color: rgb(140, 0, 255);", "background-color: rgb(2, 160, 229);");
                }
                else
                {
                    qDebug() << "[MAINWINDOW] angle not change. isDown: " << isDown << "\t isDown_r: " << isDown_r;
                    ui->operatingArea->getUp8DownButton()->setEnabled(true);
                }
            }
            else if (id == PWM_MOTOR_ID)
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
                    toggleButtonState(isFanOpen, ui->operatingArea->getFanCtrlButton(), "推进器开", "推进器关", "background-color: rgb(255, 238, 0);", "background-color: rgb(2, 195, 229);");
                }
            }
            else if (id == CAN_SPEED_ID)
            {
                m_canMotorspeedbuffer = data["params"]["speed2"]["value"];
                qDebug() << "[MAINWINDOW] m_canMotorspeedbuffer 已接收：" << m_canMotorspeedbuffer;
                ui->operatingArea->getForwardButton()->setEnabled(true);
                ui->operatingArea->getBackwardButton()->setEnabled(true);
                ui->operatingArea->getStopMovingButton()->setEnabled(true);
                ui->operatingArea->getSwitchMotorModeButton()->setEnabled(true);
            }
            else if (id == CAN_POS_ID)
            {
                float angle = data["params"]["angle"]["value"];
                int speed = data["params"]["speed"]["value"];
                int cur_max = data["params"]["current"]["value"];
                int mode = data["params"]["mode"]["value"];
                qDebug() << "[MAINWINDOW] 已接收：\nangle: " << angle << "\nspeed: " << speed << "\ncur_max: " << cur_max << "\nmode: " << mode;
                ui->operatingArea->getForwardButton()->setEnabled(true);
                ui->operatingArea->getBackwardButton()->setEnabled(true);
                ui->operatingArea->getStopMovingButton()->setEnabled(true);
                ui->operatingArea->getSwitchMotorModeButton()->setEnabled(true);
            }
        }
        else
        {
            qDebug() << "[MAINWINDOW] ack 不存在或者为 null";
            return;
        }
    }
    catch (const nlohmann::json::exception &e)
    {
        qDebug() << "[MAINWINDOW] 捕获到异常: " << e.what();
    }
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

void MainWindow::onForwardClicked()
{
    ui->operatingArea->getForwardButton()->setEnabled(false);
    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(false);
    std::map<QString, nlohmann::json> fields;

    if (m_motor_mode)
    {
        m_canMotorspeedbuffer += config.SPEED_DELTA;
        fields["speed2"] = m_canMotorspeedbuffer;
        fields["current"] = config.MOTOR_CURRENT;
        fields["mode"] = 1;
    }
    else
    {
        fields["angle"] = config.ANGLE_DELTA;
        fields["current"] = config.MOTOR_CURRENT;
        fields["speed"] = config.ANGLE_SPEED;
        fields["mode"] = 1;
    }
    qDebug() << "[MAINWINDOW] Clicked [" << ui->operatingArea->getForwardButton()->text() << "] button.";

    m_mqttClient->publishMqttMessage(fields, m_motor_mode ? CAN_SPEED_ID : CAN_POS_ID);
}

void MainWindow::onBackwardClicked()
{
    ui->operatingArea->getBackwardButton()->setEnabled(false);
    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(false);
    std::map<QString, nlohmann::json> fields;

    if (m_motor_mode)
    {
        m_canMotorspeedbuffer -= config.SPEED_DELTA;
        fields["speed2"] = m_canMotorspeedbuffer;
        fields["current"] = config.MOTOR_CURRENT;
        fields["mode"] = 1;
    }
    else
    {
        fields["angle"] = -config.ANGLE_DELTA;
        fields["current"] = config.MOTOR_CURRENT;
        fields["speed"] = config.ANGLE_SPEED;
        fields["mode"] = 1;
    }
    qDebug() << "[MAINWINDOW] Clicked [" << ui->operatingArea->getBackwardButton()->text() << "] button.";

    m_mqttClient->publishMqttMessage(fields, m_motor_mode ? CAN_SPEED_ID : CAN_POS_ID);
}

void MainWindow::onStopMovingClicked()
{
    ui->operatingArea->getStopMovingButton()->setEnabled(false);
    ui->operatingArea->getSwitchMotorModeButton()->setEnabled(false);
    qDebug() << "[MAINWINDOW] Clicked [停止] button.";
    m_canMotorspeedbuffer = 0.0f;
    std::map<QString, nlohmann::json> fields;
    fields["speed2"] = m_canMotorspeedbuffer;
    fields["current"] = config.MOTOR_CURRENT;
    fields["mode"] = 1;

    m_mqttClient->publishMqttMessage(fields, CAN_SPEED_ID);
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
    btn->setEnabled(true);
}

void switchLh08Chl(QCheckBox *checkBox)
{
    if (checkBox->isChecked())
    {
        checkBox->setStyleSheet(
            "QCheckBox::indicator {   width: 100%;"
            "height: 100%;"
            "border: none;"
            "background: transparent;}"
            "QCheckBox {background-color: rgb(0, 255, 37);"
            "}");
    }
    else
    {
        checkBox->setStyleSheet(
            "QCheckBox::indicator {   width: 100%;"
            "height: 100%;"
            "border: none;"
            "background: transparent;}"
            "QCheckBox {background-color: rgb(238, 27, 37);"
            "}");
    }
}
void MainWindow::onCheckBox1changed()
{
    bool read_bit = ui->operatingArea->getCheckBox1()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox1());
    lh08 = (lh08 & ~(1 << 0)) | ((uint8_t)read_bit << 0);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox2changed()
{
    bool read_bit = ui->operatingArea->getCheckBox2()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox2());
    lh08 = (lh08 & ~(1 << 1)) | ((uint8_t)read_bit << 1);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox3changed()
{
    bool read_bit = ui->operatingArea->getCheckBox3()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox3());
    lh08 = (lh08 & ~(1 << 2)) | ((uint8_t)read_bit << 2);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox4changed()
{
    bool read_bit = ui->operatingArea->getCheckBox4()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox4());
    lh08 = (lh08 & ~(1 << 3)) | ((uint8_t)read_bit << 3);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox5changed()
{
    bool read_bit = ui->operatingArea->getCheckBox5()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox5());
    lh08 = (lh08 & ~(1 << 4)) | ((uint8_t)read_bit << 4);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox6changed()
{
    bool read_bit = ui->operatingArea->getCheckBox6()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox6());
    lh08 = (lh08 & ~(1 << 5)) | ((uint8_t)read_bit << 5);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox7changed()
{
    bool read_bit = ui->operatingArea->getCheckBox7()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox7());
    lh08 = (lh08 & ~(1 << 6)) | ((uint8_t)read_bit << 6);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::onCheckBox8changed()
{
    bool read_bit = ui->operatingArea->getCheckBox8()->isChecked();
    switchLh08Chl(ui->operatingArea->getCheckBox8());
    lh08 = (lh08 & ~(1 << 7)) | ((uint8_t)read_bit << 7);
    qDebug() << "lh08 change -> " << "0x" + QString::number(lh08, 16).toUpper().rightJustified(2, '0');
}

void MainWindow::connectButtons()
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

    connect(ui->operatingArea->getCheckBox1(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox1changed);
    connect(ui->operatingArea->getCheckBox2(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox2changed);
    connect(ui->operatingArea->getCheckBox3(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox3changed);
    connect(ui->operatingArea->getCheckBox4(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox4changed);
    connect(ui->operatingArea->getCheckBox5(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox5changed);
    connect(ui->operatingArea->getCheckBox6(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox6changed);
    connect(ui->operatingArea->getCheckBox7(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox7changed);
    connect(ui->operatingArea->getCheckBox8(), &QCheckBox::stateChanged, this, &MainWindow::onCheckBox8changed);
}

void MainWindow::on_devBtn_clicked()
{
    if(show_dev)
    {
        ui->operatingArea->show();
        ui->operatingFlickArea->hide();
        show_dev = false;
    }else{
        ui->operatingArea->hide();
        ui->operatingFlickArea->show();
        show_dev = true;
    }
}
