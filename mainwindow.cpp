// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 创建视频显示组件
    m_videoWidget = ui->showArea;

    setWindowTitle("RTMP Player");
    // 初始化解码器
    m_decoder = new VideoDecoder(this, "rtmp://111.231.8.200:9090/live/test");
    connect(m_decoder, &VideoDecoder::frameReady, this, &MainWindow::updateVideoFrame);
    connect(m_decoder, &VideoDecoder::errorOccurred, this, &MainWindow::handleError);

    // 启动解码线程
    m_decoder->start();
}

MainWindow::~MainWindow()
{
    if (m_decoder)
    {
        m_decoder->stop();
        delete m_decoder;
    }
    delete ui;
}

void MainWindow::updateVideoFrame(const QImage &frame)
{
    // 直接传递图像到显示组件
    m_videoWidget->setFrame(frame);
}

void MainWindow::handleError(const QString &message)
{
    QMessageBox::critical(this, "Playback Error", message);
    m_decoder->stop();
}
