#ifndef OPERATINGAREA_H
#define OPERATINGAREA_H

#include <QWidget>
#include <QPushButton>

namespace Ui {
class OperatingArea;
}

class OperatingArea : public QWidget
{
    Q_OBJECT

public:
    explicit OperatingArea(QWidget *parent = nullptr);
    ~OperatingArea();

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

private slots:

private:
    Ui::OperatingArea *ui;
};

#endif // OPERATINGAREA_H
