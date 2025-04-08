#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <string>
#include <nlohmann/json.hpp>
#include "config.h"

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

signals:
    /**
     * @brief 错误发生
     * @param message 错误信息
     */
    void errorOccurred(const QString &message);

private:
    // 配置对象，存储编码所需的配置信息
    AppConfig m_config;
};

#endif // MQTTCLIENT_H
