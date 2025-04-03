#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "video_decoder.h"
#include "videowidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateVideoFrame(const QImage &frame);
    void handleError(const QString &message);

private:
    Ui::MainWindow *ui;
    VideoDecoder *m_decoder;
    VideoWidget *m_videoWidget;
};
#endif // MAINWINDOW_H
