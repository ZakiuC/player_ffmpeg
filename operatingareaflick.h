#ifndef OPERATINGAREAFLICK_H
#define OPERATINGAREAFLICK_H

#include <QWidget>
#include "mainwindow.h"

namespace Ui {
class OperatingAreaFlick;
}

class OperatingAreaFlick : public QWidget
{
    Q_OBJECT

public:
    explicit OperatingAreaFlick(QWidget *parent = nullptr);
    ~OperatingAreaFlick();

    void set_window(MainWindow *mainwindow);

private slots:
    void on_motor485Btn_clicked();

    void on_lh08Btn_clicked();

    void on_pwmBtn_clicked();

    void on_motorCanAngleBtn_clicked();

    void on_motorCanSpeedBtn_clicked();

private:
    Ui::OperatingAreaFlick *ui;
    MainWindow *m_mainWindow;
};

#endif // OPERATINGAREAFLICK_H
