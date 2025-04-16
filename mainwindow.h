#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "video_decoder.h"
#include "videowidget.h"
#include "mqtt_client.h"
#include <memory>
#include "numberpaddialog.h"
#include <QCheckBox>

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
     * @brief 更新视频帧
     * @param frame 视频帧
     */
    void updateVideoFrame(const QImage &frame);
    
    /**
     * @brief 处理MQTT错误
     * @param message 错误信息
     */
    void handleError(const QString &message);

    /**
     * @brief 处理MQTT错误
     * @param message 错误信息
     */
    void handleMQTTError(const QString &msg, bool maxPublishFlag);

    /**
     * @brief 处理订阅的MQTT消息
     * @param topic 消息主题
     * @param payload 消息负载
     */
    void handleMessageReceived(const QString &topic, const QByteArray &payload);

    /**
     * @brief 处理切换电机控制模式按钮的点击事件
     */
    void onSwitchMotorModeClicked();

    /**
     * @brief 处理切换摄像头启停的点击事件
     */
    void onPlay8StopClicked();

    /**
     * @brief 处理毛刷位置按钮的点击事件
     */
    void onUp8DownClicked();

    /**
     * @brief 处理电机向前/加速按钮的点击事件
     */
    void onForwardClicked();

    /**
     * @brief 处理电机向后/减速按钮的点击事件
     */
    void onBackwardClicked();
    
    /**
     * @brief 处理停止移动按钮的点击事件
     */
    void onStopMovingClicked();
    
    /**
     * @brief 处理风扇开关按钮的点击事件
     */
    void onFanCtrlClicked();

    /**
     * @brief 处理GPS开关按钮的点击事件
     */
    void onGPSClicked();

    /**
     * @brief 处理继电器开关按钮的开启的点击事件
     */
    void onOpenSelectedClicked();

    /**
     * @brief 处理继电器开关按钮的关闭的点击事件
     */
    void onCloseSelectedClicked();

    /**
     * @brief 处理继电器同步按钮的点击事件
     */
    void onOpen8CloseClicked();

    /**
     * @brief 处理切换"开发者模式"的点击事件
     */
    void on_devBtn_clicked();

public slots:
    /**
     * @brief 处理开发者模式参数控制的输入字段
     * @param inputs 输入字段列表
     * @param type 对话框类型
     */
    void handleInputsAccepted(const QStringList &inputs, dialog_type_e type);

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 初始化MQTT客户端
     */
    void initializeMQTT();
    
    /**
     * @brief 初始化视频显示控件
     */
    void initializeVideo();
    
    /**
     * @brief 设置控件的连接
     */
    void setupConnections();
    
    /**
     * @brief 设置按钮的连接
     */
    void setupButtonConnections();
    
    /**
     * @brief 设置复选框的连接
     */
    void setupCheckBoxConnections();
    
    /**
     * @brief 处理复选框的切换事件
     * @param bitIndex 复选框的位索引
     * @param state 复选框的新状态
     */
    void handleCheckBoxChanged(int bitIndex, int state);

    /**
     * @brief 更新复选框的样式
     * @param box 复选框
     * @param checked 复选框的新状态
     */
    void updateCheckBoxStyle(QCheckBox *box, bool checked);

    /**
     * @brief 解析时间戳字符串
     * @param timestampStr 时间戳字符串
     * @return 解析后的时间戳
     */
    QDateTime parseTimestamp(const std::string &timestampStr) const;

    /**
     * @brief 处理CAN电机位置模式消息
     * @param data 消息数据
     */
    void handleCanPosMessage(const nlohmann::json &data);

    /**
     * @brief 处理CAN电机速度模式消息
     * @param data 消息数据
     */
    void handleCanSpeedMessage(const nlohmann::json &data);

    /**
     * @brief 处理MODBUS电机消息
     * @param data 消息数据
     */
    void handleModbusMotorMessage(const nlohmann::json &data);

    /**
     * @brief 处理LH08继电器消息
     * @param data 消息数据
     */
    void handleLH08Message(const nlohmann::json &data);

    /**
     * @brief 处理PWM电机消息
     * @param data 消息数据
     */
    void handlePwmMotorMessage(const nlohmann::json &data);

    /**
     * @brief 处理摄像头消息
     * @param data 消息数据
     */
    void handleCameraMessage(const nlohmann::json &data);

    /**
     * @brief 切换按钮的状态
     * @param state 当前按钮状态
     * @param btn 要切换状态的按钮
     * @param textOn 按钮文本切换到开启状态时的文本
     * @param textOff 按钮文本切换到关闭状态时的文本
     * @param styleOn 按钮样式切换到开启状态时的样式
     * @param styleOff 按钮样式切换到关闭状态时的样式
     */
    void toggleButtonState(bool &state, QPushButton *btn,
                           const QString &textOn, const QString &textOff,
                           const QString &styleOn, const QString &styleOff);

    /**
     * @brief 发送电机控制消息
     * @param value 电机控制值
     * @param type 电机控制类型
     */
    void sendMotorControl(float value, MotorControlType_e type);

    Ui::MainWindow *ui;
    AppConfig config; // 应用配置
    bool isPlaying;   // 视频播放状态
    bool isDown;      // 毛刷位置状态
    bool isFanOpen;   // 推进器开关状态
    uint8_t lh08;     // LH08继电器状态
    uint8_t lh08_buffer;    // LH08继电器状态缓存
    std::unique_ptr<VideoDecoder> m_decoder;  // 视频解码器
    VideoWidget *m_videoWidget;               // 视频显示控件
    std::unique_ptr<MQTTClient> m_mqttClient; // MQTT 通信客户端

    float m_canMotorspeedbuffer;    // CAN电机速度模式缓存
    uint8_t m_motor_mode;   // 电机控制模式

    bool show_dev;  // 开发者模式显示状态
};

#endif // MAINWINDOW_H
