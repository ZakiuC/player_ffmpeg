#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <string>
#include <nlohmann/json.hpp>
#include "config.h"
#include <mqtt/async_client.h>
#include <mqtt/callback.h>

/**
 * @brief MQTT通信类
 */
class MQTTClient : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param cfg 配置对象
     * @param parent 父对象
     */
    explicit MQTTClient(const AppConfig &cfg, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MQTTClient();

    /**
     * @brief 发布消息
     * @param topic 主题
     * @param params 参数
     * @param method 方法
     * @param id 消息ID
     * @param version 版本
     */
    void publish(const QString &topic,
                 const nlohmann::json &params,
                 const QString &method,
                 const QString &id = "123",
                 const QString &version = "1.0");

    /**
     * @brief 发布运动相关的参数
     * @param topic 主题
     * @param method 方法
     * @param id 消息ID
     * @param angle 角度
     * @param speed 速度
     * @param current 电流
     * @param mode 模式
     * @param version 版本
     */
    void publishMovementParams(const QString &topic, const QString &method, const QString &id, int angle, int speed, int current, int mode, const QString &version = "1.0");

    /**
     * @brief 发布通用的消息
     * @param key 键
     * @param value 值
     * @param eventCode 事件代码
     */
    template <typename T>
    void publishMqttMessage(const QString &key, const T &value, const QString &eventCode)
    {
        nlohmann::json params;
        params[key.toStdString()]["value"] = value;
        publish(m_config.TOPIC, params, "thing.event.property.post", eventCode);
    }

    /**
     * @brief 订阅主题
     * @param topic 主题
     * @param qos QoS等级
     */
    void subscribe(const QString &topic, int qos = 1);

    /**
     * @brief 取消订阅主题
     * @param topic 主题
     */
    void unsubscribe(const QString &topic);

signals:
    /**
     * @brief 错误发生
     * @param message 错误信息
     */
    void errorOccurred(const QString &message);

    /**
     * @brief 收到消息
     * @param topic 主题
     * @param payload 消息负载
     */
    void messageReceived(const QString &topic, const QByteArray &payload);

private slots:
    /**
     * @brief 连接成功
     */
    void onConnected();

    /**
     * @brief 收到消息
     * @param topic 主题
     * @param payload 消息负载
     */
    void onMessageReceived(const QString &topic, const QByteArray &payload);

private:
    /**
     * @brief 连接到MQTT服务器
     */
    void connectToBroker();

    /**
     * @brief 处理消息到达
     * @param msg 消息指针
     */
    void handleMessageArrived(mqtt::const_message_ptr msg);

    class MQTTClientCallback : public mqtt::callback
    {
    public:
        MQTTClientCallback(MQTTClient *client) : m_client(client) {}

        /**
         * @brief 连接成功
         * @param cause 连接原因
         */
        void connected(const std::string& cause) override;

        /**
         * @brief 连接丢失
         * @param cause 连接原因
         */
        void connection_lost(const std::string& cause) override;

        /**
         * @brief 消息到达
         * @param msg 消息指针
         */
        void message_arrived(mqtt::const_message_ptr msg) override;

    private:
        MQTTClient *m_client;   // MQTT客户端
    };

    AppConfig m_config; // 配置对象，存储编码所需的配置信息
    mqtt::async_client *m_client;   // MQTT客户端
    mqtt::connect_options m_connOpts;   // 连接选项
    MQTTClientCallback *m_callback; // 回调函数
    std::vector<std::pair<QString, int>> m_subscribedTopics;    // 订阅的主题
};

#endif // MQTTCLIENT_H
