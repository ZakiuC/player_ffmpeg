#include "operatingareaflick.h"
#include "ui_operatingareaflick.h"
#include "numberpaddialog.h"
#include "mainwindow.h"
#include <QDebug>

OperatingAreaFlick::OperatingAreaFlick(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OperatingAreaFlick)
{
    ui->setupUi(this);
}

OperatingAreaFlick::~OperatingAreaFlick()
{
    delete ui;
}


void OperatingAreaFlick::on_motor485Btn_clicked()
{
    NumberPadDialog dialog(MOTOR_485_DIALOG, this);
    connect(&dialog, &NumberPadDialog::inputsAccepted, m_mainWindow, &MainWindow::handleInputsAccepted);

    // 自定义标签说明
    QStringList labels = {"角度1", "角度2", "模式"};

    // 设置多个输入框和说明（传入标签列表）
    dialog.setInputFields(labels.size(), labels);
    dialog.exec();
}

void OperatingAreaFlick::on_lh08Btn_clicked()
{
    NumberPadDialog dialog(LH08_DIALOG, this);
    connect(&dialog, &NumberPadDialog::inputsAccepted, m_mainWindow, &MainWindow::handleInputsAccepted);

    // 自定义标签说明
    QStringList labels = {"8bit"};

    // 设置多个输入框和说明（传入标签列表）
    dialog.setInputFields(labels.size(), labels);
    dialog.exec();
}

void OperatingAreaFlick::on_pwmBtn_clicked()
{
    NumberPadDialog dialog(PWM_DIALOG, this);
    connect(&dialog, &NumberPadDialog::inputsAccepted, m_mainWindow, &MainWindow::handleInputsAccepted);

    // 自定义标签说明
    QStringList labels = {"占空比"};

    // 设置多个输入框和说明（传入标签列表）
    dialog.setInputFields(labels.size(), labels);
    dialog.exec();
}

void OperatingAreaFlick::on_motorCanAngleBtn_clicked()
{
    NumberPadDialog dialog(MOTOR_CAN_ANGLE_DIALOG, this);
    connect(&dialog, &NumberPadDialog::inputsAccepted, m_mainWindow, &MainWindow::handleInputsAccepted);

    // 自定义标签说明
    QStringList labels = {"角度", "电流", "速度", "模式"};

    // 设置多个输入框和说明（传入标签列表）
    dialog.setInputFields(labels.size(), labels);
    dialog.exec();
}

void OperatingAreaFlick::on_motorCanSpeedBtn_clicked()
{
    NumberPadDialog dialog(MOTOR_CAN_SPEED_DIALOG, this);
    connect(&dialog, &NumberPadDialog::inputsAccepted, m_mainWindow, &MainWindow::handleInputsAccepted);

    // 自定义标签说明
    QStringList labels = {"速度", "电流", "模式"};

    // 设置多个输入框和说明（传入标签列表）
    dialog.setInputFields(labels.size(), labels);
    dialog.exec();
}

void OperatingAreaFlick::set_window(MainWindow *mainwindow)
{
    m_mainWindow = mainwindow;
}
