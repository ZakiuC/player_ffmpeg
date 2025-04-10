#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

/**
 * 配置结构体，存储应用程序的所有配置信息
 */
struct AppConfig
{
    // 编码参数
    const QString RTMP_URL = "rtmp://111.231.8.200:9090/live/test";

    // MQTT 服务器的相关配置
    const QString SERVER_ADDRESS = "tcp://iot-06z00c19vf5ynvs.mqtt.iothub.aliyuncs.com:1883";                     // MQTT 服务器地址和端口
    const QString CLIENT_ID = "k1sbasnSsQz.test_aly|securemode=2,signmethod=hmacsha256,timestamp=1741594539567|"; // 客户端 ID
    const QString USERNAME = "test_aly&k1sbasnSsQz";                                                              // 用户名
    const QString PASSWORD = "fbe84b268f6a5ee565d4585768db801eb087f55fd703947f1ed6ad549bd31029";                  // 密码
    const QString TOPIC = "/sys/k1sbasnSsQz/test_aly/thing/event/property/post";                                  //
    const QString subTOPIC = "/k1sbasnSsQz/test_aly/user/ackcheck";
};

#endif // CONFIG_H
