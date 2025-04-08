QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 设置 FFmpeg 头文件路径
INCLUDEPATH += /opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/local/ffmpeg_build/include

LIBS += -L/opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/local/ffmpeg_build/lib \
        -lavformat -lavcodec -lavutil -lswscale -lswresample -lavfilter -lpostproc \

QMAKE_LFLAGS += -Wl,-rpath-link,/opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/lib

## 添加 Paho MQTT C/C++ 库的头文件目录
#INCLUDEPATH += /opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/local/include
LIBS += -L/opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/lib/gcc/aarch64-linux-gnu/11 \
        -L/opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/local/lib \
        -lpaho-mqttpp3 -lpaho-mqtt3a

message($$LIBS)
message($$QMAKE_LFLAGS)

## 添加 nlohmann/json 的头文件目录
#INCLUDEPATH += /opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/include

QMAKE_LFLAGS += -Wl,-rpath,/opt/EmbedSky/TQ3588/toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/lib


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mqtt_client.cpp \
    video_decoder.cpp \
    videowidget.cpp

HEADERS += \
    config.h \
    mainwindow.h \
    mqtt_client.h \
    video_decoder.h \
    videowidget.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
