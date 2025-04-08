#include "mqtt_client.h"
#include <mqtt/async_client.h>
#include <mqtt/exception.h>
#include <QDebug>

MQTTClient::MQTTClient(const AppConfig &cfg, QObject *parent)
    : QObject(parent),
      m_config(cfg)
{
}

void MQTTClient::publish(const QString &topic,
                         const nlohmann::json &params,
                         const QString &method,
                         const QString &id,
                         const QString &version)
{
    try
    {
        // 1. 创建MQTT客户端
        mqtt::async_client client(m_config.SERVER_ADDRESS.toStdString(), m_config.CLIENT_ID.toStdString());

        // 2. 配置连接选项
        mqtt::connect_options connOpts;
        connOpts.set_clean_session(true);
        connOpts.set_automatic_reconnect(true);
        connOpts.set_keep_alive_interval(60);
        connOpts.set_user_name(m_config.USERNAME.toStdString());
        connOpts.set_password(m_config.PASSWORD.toStdString());

        // 3. 连接服务器
        qDebug() << "[MQTT] Connecting to:" << m_config.SERVER_ADDRESS.toStdString().c_str();
        client.connect(connOpts)->wait();
        qDebug() << "[MQTT] Connected successfully";

        // 4. 构造payload
        nlohmann::json payload;
        payload["id"] = id.toStdString();
        payload["version"] = version.toStdString();
        payload["params"] = params;
        payload["method"] = method.toStdString();

        // 5. 创建消息并设置QoS
        auto pubmsg = mqtt::make_message(topic.toStdString(), payload.dump());
        pubmsg->set_qos(1);

        // 6. 发布消息
        client.publish(pubmsg)->wait_for(std::chrono::seconds(10));
        qDebug() << "[MQTT] Message published to topic:" << topic.toStdString().c_str();

        // 7. 断开连接
        client.disconnect()->wait();
        qDebug() << "[MQTT] Disconnected";
    }
    catch (const mqtt::exception &exc)
    {
        QString errorMsg = QString("MQTT Error: %1").arg(exc.what());
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
    }
}
