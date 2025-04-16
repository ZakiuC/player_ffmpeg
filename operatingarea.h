#ifndef OPERATINGAREA_H
#define OPERATINGAREA_H

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>

namespace Ui {
class OperatingArea;
}

class OperatingArea : public QWidget
{
    Q_OBJECT

public:
    explicit OperatingArea(QWidget *parent = nullptr);
    ~OperatingArea();

    QPushButton* getSwitchMotorModeButton() const;
    QPushButton* getPlay8StopButton() const;
    QPushButton* getUp8DownButton() const;
    QPushButton* getForwardButton() const;
    QPushButton* getBackwardButton() const;
    QPushButton* getStopMovingButton() const;
    QPushButton* getFanCtrlButton() const;
    QPushButton* getGPSButton() const;
    QPushButton* getOpenSelectedButton() const;
    QPushButton* getCloseSelectedButton() const;
    QPushButton* getOpen8CloseButton() const;
    QCheckBox* getCheckBox1() const;
    QCheckBox* getCheckBox2() const;
    QCheckBox* getCheckBox3() const;
    QCheckBox* getCheckBox4() const;
    QCheckBox* getCheckBox5() const;
    QCheckBox* getCheckBox6() const;
    QCheckBox* getCheckBox7() const;
    QCheckBox* getCheckBox8() const;
    QVector<QCheckBox*> getCheckBoxes() const;

private:
    Ui::OperatingArea *ui;
};

#endif // OPERATINGAREA_H
