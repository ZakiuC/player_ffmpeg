#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

#define  CLOSE_MQTT_ID      "000"
#define  CAN_POS_ID         "001"
#define  CAN_SPEED_ID       "002"
#define  MODBUS_MOROTR_ID   "003"
#define  LH08_ID            "004"
#define  PWM_MOTOR_ID       "005"
#define  CAMERA_ID          "006"

/**
 * 配置结构体，存储应用程序的所有配置信息
 */
struct AppConfig
{
    // 编码参数
    const QString RTMP_URL = "rtmp://111.231.8.200:9090/live/test";

    // MQTT 服务器的相关配置
    const QString SERVER_ADDRESS = "tcp://iot-06z00c19vf5ynvs.mqtt.iothub.aliyuncs.com:1883";
    const QString CLIENT_ID = "k1sbasnSsQz.test_aly|securemode=2,signmethod=hmacsha256,timestamp=1741594539567|";
    const QString USERNAME = "test_aly&k1sbasnSsQz";
    const QString PASSWORD = "fbe84b268f6a5ee565d4585768db801eb087f55fd703947f1ed6ad549bd31029";
    const QString TOPIC = "/sys/k1sbasnSsQz/test_aly/thing/event/property/post";
    const QString subTOPIC = "/k1sbasnSsQz/test_aly/user/ackcheck";

    // can电机配置
    const int MOTOR_CURRENT = 30;
    const float SPEED_DELTA = 5.f;
    const float ANGLE_DELTA = 90.f;
    const int ANGLE_SPEED = 10;

    // 485
    const double MOTOR485_1_DOWN_POS = -90;
    const double MOTOR485_1_UP_POS = 90;
    const double MOTOR485_2_DOWN_POS = -90;
    const double MOTOR485_2_UP_POS = 90;
};

#endif // CONFIG_H
