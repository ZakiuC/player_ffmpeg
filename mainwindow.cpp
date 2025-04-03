#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 从 UI 中获取视频显示区域组件，用于显示解码后的视频帧
    m_videoWidget = ui->showArea;

    // 设置窗口标题
    setWindowTitle("RTMP Player");

    // 初始化视频解码器
    m_decoder = new VideoDecoder(this, "rtmp://111.231.8.200:9090/live/test");

    // 连接解码器的 frameReady 信号到 updateVideoFrame 槽函数，
    // 当解码器解码出一帧图像时更新视频显示
    connect(m_decoder, &VideoDecoder::frameReady, this, &MainWindow::updateVideoFrame);

    // 连接解码器的 errorOccurred 信号到 handleError 槽函数，
    // 当解码过程中发生错误时提示用户
    connect(m_decoder, &VideoDecoder::errorOccurred, this, &MainWindow::handleError);

    // 启动视频解码线程，开始实时解码视频流
    m_decoder->start();
}

MainWindow::~MainWindow()
{
    // 如果解码器存在，则先停止解码线程，再释放内存
    if (m_decoder)
    {
        m_decoder->stop();
        delete m_decoder;
    }

    delete ui;
}

void MainWindow::updateVideoFrame(const QImage &frame)
{
    // 将新的视频帧传递给视频显示组件，触发界面重绘
    m_videoWidget->setFrame(frame);
}

void MainWindow::handleError(const QString &message)
{
    // 弹出错误消息对话框，通知用户播放错误信息
    QMessageBox::critical(this, "Playback Error", message);
    // 出现错误后停止视频解码线程
    m_decoder->stop();
}
