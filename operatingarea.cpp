#include "operatingarea.h"
#include "ui_operatingarea.h"

OperatingArea::OperatingArea(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OperatingArea)
{
    ui->setupUi(this);
}

OperatingArea::~OperatingArea()
{
    delete ui;
}

QPushButton* OperatingArea::getPlay8StopButton() const
{
    return ui->play8stopBtn;
}

QPushButton* OperatingArea::getUp8DownButton() const
{
    return ui->up8downBtn;
}

QPushButton* OperatingArea::getForwardButton() const
{
    return ui->forwardBtn;
}

QPushButton* OperatingArea::getBackwardButton() const
{
    return ui->backwardBtn;
}

QPushButton* OperatingArea::getStopMovingButton() const
{
    return ui->stopMovingBtn;
}

QPushButton* OperatingArea::getFanCtrlButton() const
{
    return ui->fanCtrlBtn;
}

QPushButton* OperatingArea::getGPSButton() const
{
    return ui->GPSBtn;
}

QPushButton* OperatingArea::getOpenSelectedButton() const
{
    return ui->openSelectedBtn;
}

QPushButton* OperatingArea::getCloseSelectedButton() const
{
    return ui->closeSelectedBtn;
}

QPushButton* OperatingArea::getOpen8CloseButton() const
{
    return ui->open8closeBtn;
}

QCheckBox* OperatingArea::getCheckBox1() const
{
    return ui->ch1;
}

QCheckBox* OperatingArea::getCheckBox2() const
{
    return ui->ch2;
}

QCheckBox* OperatingArea::getCheckBox3() const
{
    return ui->ch3;
}

QCheckBox* OperatingArea::getCheckBox4() const
{
    return ui->ch4;
}

QCheckBox* OperatingArea::getCheckBox5() const
{
    return ui->ch5;
}

QCheckBox* OperatingArea::getCheckBox6() const
{
    return ui->ch6;
}

QCheckBox* OperatingArea::getCheckBox7() const
{
    return ui->ch7;
}

QCheckBox* OperatingArea::getCheckBox8() const
{
    return ui->ch8;
}
