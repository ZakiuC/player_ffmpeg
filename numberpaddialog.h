#ifndef NUMBERPADDIALOG_H
#define NUMBERPADDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <nlohmann/json.hpp>

namespace Ui
{
    class NumberPadDialog;
}

typedef enum
{
    MOTOR_CAN_ANGLE_DIALOG,
    MOTOR_CAN_SPEED_DIALOG,
    MOTOR_485_DIALOG,
    LH08_DIALOG,
    PWM_DIALOG
} dialog_type_e;

class NumberPadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NumberPadDialog(dialog_type_e type, QWidget *parent = nullptr);
    ~NumberPadDialog();

    // 动态设置输入框数量、说明和布局
    void setInputFields(int numFields, const QStringList &labels);

signals:
    // 创建一个信号，用于传递输入数据到 MainWindow
    void inputsAccepted(const QStringList &inputs, dialog_type_e type);

private slots:
    void addDigitToFocusedLineEdit(const QString &digit);
    void handleBackspace();

private:
    QStringList getAllInputs() const;
    void onAccepted();
    void onRejected();

    Ui::NumberPadDialog *ui;
    QVBoxLayout *layout;
    dialog_type_e m_dialog_type;
};

#endif // NUMBERPADDIALOG_H
