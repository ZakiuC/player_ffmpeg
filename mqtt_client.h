#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <string>
#include <nlohmann/json.hpp>
#include "config.h"
#include <mqtt/async_client.h>
#include <mqtt/callback.h>
#include <memory>
#include <vector>
#include <utility>
#include <QMutex>
#include <QDateTime>
#include <QQueue>

/**
 * @brief MQTT通信类
 */
class MQTTClient : public QObject
{
    Q_OBJECT
public:
    explicit MQTTClient(const AppConfig &cfg, QObject *parent = nullptr);
    ~MQTTClient();

    void publish(const QString &topic,
                 const nlohmann::json &params,
                 const QString &method,
                 const QString &id = "123",
                 const QString &version = "1.0");

    void publishMovementParams(const QString &topic, const QString &method, const QString &id, int angle, float speed, int current, int mode, const QString &version = "1.0");
    void publishMqttMessage(const std::map<QString, nlohmann::json>& fieldValues, const QString &id);

    void subscribe(const QString &topic, int qos = 1);
    void unsubscribe(const QString &topic);

signals:
    void errorOccurred(const QString &message, bool maxPublish = false);
    void messageReceived(const QString &topic, const QByteArray &payload);

private slots:
    void onConnected();
    void onMessageReceived(const QString &topic, const QByteArray &payload);

private:
    void connectToBroker();
    void handleMessageArrived(mqtt::const_message_ptr msg);

    // 内部回调类
    class MQTTClientCallback : public mqtt::callback
    {
    public:
        MQTTClientCallback(MQTTClient *client) : m_client(client) {}
        void connected(const std::string& cause) override;
        void connection_lost(const std::string& cause) override;
        void message_arrived(mqtt::const_message_ptr msg) override;
    private:
        MQTTClient *m_client;
    };

    AppConfig m_config;
    std::unique_ptr<mqtt::async_client> m_client;
    mqtt::connect_options m_connOpts;
    std::unique_ptr<MQTTClientCallback> m_callback;
    std::vector<std::pair<QString, int>> m_subscribedTopics;
    QMutex m_publishMutex;         // 保护速率限制检查的互斥锁
    QQueue<QDateTime> m_publishTimestamps;  // 存储每次 publish 的时间戳
    int maxPublishPerSecond;         // 时间窗口内允许的最大发布数
    int timeWindowMs;                // 时间窗口，单位为毫秒，例如 1000 表示 1 秒
};

#endif // MQTTCLIENT_H
