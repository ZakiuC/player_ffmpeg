#include "mqtt_client.h"
#include <mqtt/async_client.h>
#include <mqtt/exception.h>
#include <QDebug>
#include <QMetaObject>

MQTTClient::MQTTClient(const AppConfig &cfg, QObject *parent)
    : QObject(parent),
      m_config(cfg),
      m_client(std::make_unique<mqtt::async_client>(m_config.SERVER_ADDRESS.toStdString(), m_config.CLIENT_ID.toStdString())),
      m_callback(std::make_unique<MQTTClientCallback>(this))
{
    m_client->set_callback(*m_callback);

    m_connOpts.set_clean_session(true);
    m_connOpts.set_automatic_reconnect(true);
    m_connOpts.set_keep_alive_interval(60);
    m_connOpts.set_user_name(m_config.USERNAME.toStdString());
    m_connOpts.set_password(m_config.PASSWORD.toStdString());

    connectToBroker();
}

MQTTClient::~MQTTClient()
{
    try
    {
        if (m_client->is_connected())
        {
            m_client->disconnect()->wait();
        }
    }
    catch (const mqtt::exception &e)
    {
        qCritical() << "Error in destructor:" << e.what();
    }
}

void MQTTClient::connectToBroker()
{
    try
    {
        m_client->connect(m_connOpts)->wait();
        qDebug() << "[MQTT] Connected to broker";
    }
    catch (const mqtt::exception &e)
    {
        qCritical() << "[MQTT] Connection failed:" << e.what();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void MQTTClient::publish(const QString &topic, const nlohmann::json &params, const QString &method, const QString &id, const QString &version)
{
    try
    {
        qDebug() << "[MQTT] id:" << id;
        qDebug() << "[MQTT] version:" << version;
        qDebug() << "[MQTT] params:" << QString::fromStdString(params.dump());
        qDebug() << "[MQTT] method:" << method;

        if (!m_client->is_connected())
        {
            connectToBroker();
        }

        nlohmann::json payload;
        payload["id"] = id.toStdString();
        payload["version"] = version.toStdString();
        payload["params"] = params;
        payload["method"] = method.toStdString();

        auto pubmsg = mqtt::make_message(topic.toStdString(), payload.dump());
        pubmsg->set_qos(1);

        m_client->publish(pubmsg)->wait_for(std::chrono::seconds(10));
        qDebug() << "[MQTT] Published to" << topic;
    }
    catch (const mqtt::exception &e)
    {
        qCritical() << "[MQTT] Publish error:" << e.what();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void MQTTClient::publishMovementParams(const QString &topic, const QString &method, const QString &id, int angle, float speed, int current, int mode, const QString &version)
{
    nlohmann::json params;
    params["angle"]["value"] = angle;
    params["speed"]["value"] = speed;
    params["current"]["value"] = current;
    params["mode"]["value"] = mode;
    publish(topic, params, method, id, version);
}

void MQTTClient::subscribe(const QString &topic, int qos)
{
    try
    {
        m_client->subscribe(topic.toStdString(), qos)->wait();
        m_subscribedTopics.emplace_back(topic, qos);
        qDebug() << "[MQTT] Subscribed to" << topic;
    }
    catch (const mqtt::exception &e)
    {
        qCritical() << "[MQTT] Subscribe error:" << e.what();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void MQTTClient::unsubscribe(const QString &topic)
{
    try
    {
        m_client->unsubscribe(topic.toStdString())->wait();
        auto it = std::remove_if(m_subscribedTopics.begin(), m_subscribedTopics.end(),
                                 [&topic](const auto &sub){ return sub.first == topic; });
        m_subscribedTopics.erase(it, m_subscribedTopics.end());
        qDebug() << "[MQTT] Unsubscribed from" << topic;
    }
    catch (const mqtt::exception &e)
    {
        qCritical() << "[MQTT] Unsubscribe error:" << e.what();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void MQTTClient::onConnected()
{
    for (const auto &sub : m_subscribedTopics)
    {
        try
        {
            m_client->subscribe(sub.first.toStdString(), sub.second)->wait();
            qDebug() << "[MQTT] Resubscribed to" << sub.first;
        }
        catch (const mqtt::exception &e)
        {
            qCritical() << "[MQTT] Resubscribe error:" << e.what();
            emit errorOccurred(QString::fromStdString(e.what()));
        }
    }
}

void MQTTClient::handleMessageArrived(mqtt::const_message_ptr msg)
{
    QString topic = QString::fromStdString(msg->get_topic());
    if(topic != m_config.subTOPIC)
    {
        return;
    }
    QByteArray payload(msg->get_payload().data(), msg->get_payload().size());
    QMetaObject::invokeMethod(this, "onMessageReceived", Qt::QueuedConnection,
                              Q_ARG(QString, topic),
                              Q_ARG(QByteArray, payload));
}

void MQTTClient::onMessageReceived(const QString &topic, const QByteArray &payload)
{
    emit messageReceived(topic, payload);
}

// MQTTClientCallback 的实现
void MQTTClient::MQTTClientCallback::connected(const std::string &cause)
{
    Q_UNUSED(cause);
    QMetaObject::invokeMethod(m_client, "onConnected", Qt::QueuedConnection);
}

void MQTTClient::MQTTClientCallback::connection_lost(const std::string &cause)
{
    Q_UNUSED(cause);
    qWarning() << "[MQTT] Connection lost";
}

void MQTTClient::MQTTClientCallback::message_arrived(mqtt::const_message_ptr msg)
{
    m_client->handleMessageArrived(msg);
}

void MQTTClient::publishMqttMessage(const std::map<QString, nlohmann::json>& fieldValues, const QString &id)
{
    nlohmann::json params;
    for (const auto &field : fieldValues) {
        // field.first 为字段名称，field.second 为对应的值，存入 JSON 时放到 "value" 下
        params[field.first.toStdString()]["value"] = field.second;
    }
    publish(m_config.TOPIC, params, "thing.event.property.post", id);
}
