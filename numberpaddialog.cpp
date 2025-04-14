#include "numberpaddialog.h"
#include "ui_numberpaddialog.h"
#include <QDebug>

NumberPadDialog::NumberPadDialog(dialog_type_e type, QWidget *parent) : QDialog(parent),
                                                                        ui(new Ui::NumberPadDialog),
                                                                        m_dialog_type(type)
{
    ui->setupUi(this);

    // 连接 OK 按钮
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &NumberPadDialog::onAccepted);
    // 连接 Cancel 按钮
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &NumberPadDialog::onRejected);

    // 连接数字按钮与槽
    connect(ui->num0Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("0"); });
    connect(ui->num1Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("1"); });
    connect(ui->num2Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("2"); });
    connect(ui->num3Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("3"); });
    connect(ui->num4Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("4"); });
    connect(ui->num5Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("5"); });
    connect(ui->num6Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("6"); });
    connect(ui->num7Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("7"); });
    connect(ui->num8Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("8"); });
    connect(ui->num9Btn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("9"); });
    connect(ui->dotBtn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("."); });
    connect(ui->minusBtn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit("-"); });
    connect(ui->blankspaceBtn, &QPushButton::clicked, this, [=]()
            { addDigitToFocusedLineEdit(" "); });

    connect(ui->backspaceBtn, &QPushButton::clicked, this, &NumberPadDialog::handleBackspace);
}


void NumberPadDialog::addDigitToFocusedLineEdit(const QString &digit)
{
    QWidget *focusedWidget = qApp->focusWidget();

    QLineEdit *focusedLineEdit = qobject_cast<QLineEdit*>(focusedWidget);

    if (focusedLineEdit)
    {
        int cursorPos = focusedLineEdit->cursorPosition();

        QString currentText = focusedLineEdit->text();

        currentText.insert(cursorPos, digit);

        focusedLineEdit->setText(currentText);
        focusedLineEdit->setCursorPosition(cursorPos + digit.length());
    }
    else
    {
        qDebug("no focus on line edit");
    }
}

void NumberPadDialog::handleBackspace()
{
    // 获取当前焦点的控件
    QWidget *focusedWidget = qApp->focusWidget();

    // 检查是否是QLineEdit或者是QLineEdit的子类
    QLineEdit *focusedLineEdit = qobject_cast<QLineEdit*>(focusedWidget);

    if (focusedLineEdit)
    {
        focusedLineEdit->backspace();
    }
    else
    {
        qDebug("no focus on line edit");
    }
}

NumberPadDialog::~NumberPadDialog()
{
    delete ui;
}

void NumberPadDialog::setInputFields(int numFields, const QStringList &labels)
{
    while (ui->formLayout->rowCount() > 0)
    {
        QLayoutItem *item = ui->formLayout->takeAt(0);
        if (!item)
            continue;

        // 删除标签（如果有）
        if (item->layout())
        {
            delete item->layout();
        }

        // 删除输入框（如果有）
        if (item->widget())
        {
            delete item->widget();
        }

        delete item;
    }

    // 动态添加输入框和标签
    for (int i = 0; i < numFields; ++i)
    {
        // 创建说明标签
        QLabel *label = new QLabel(labels.value(i, QString("Input %1").arg(i + 1)), this);

        // 创建输入框
        QLineEdit *input = new QLineEdit(this);
        input->setPlaceholderText(QString("Input %1").arg(i + 1));

        // 将标签和输入框添加到 QFormLayout
        ui->formLayout->addRow(label, input);
    }

    if (ui->formLayout->rowCount() > 0) {
        QLineEdit *firstInput = qobject_cast<QLineEdit *>(ui->formLayout->itemAt(0, QFormLayout::FieldRole)->widget());
        if (firstInput) {
            firstInput->setFocus();
        }
    }

    switch(m_dialog_type)
    {
    case MOTOR_CAN_ANGLE_DIALOG:
        setWindowTitle(QString("can电机角度参数"));
        break;

    case MOTOR_CAN_SPEED_DIALOG:
        setWindowTitle(QString("can电机速度参数"));
        break;

    case MOTOR_485_DIALOG:
        setWindowTitle(QString("485电机参数"));
        break;

    case LH08_DIALOG:
        setWindowTitle(QString("继电器参数"));
        break;

    case PWM_DIALOG:
        setWindowTitle(QString("PWM参数"));
        break;

    default:
        setWindowTitle(QString("未知(%1)").arg(numFields));
        break;
    }
}

QStringList NumberPadDialog::getAllInputs() const
{
    QStringList inputs;
    for (int i = 0; i < ui->formLayout->rowCount(); ++i)
    {
        QLineEdit *input = qobject_cast<QLineEdit *>(ui->formLayout->itemAt(i, QFormLayout::FieldRole)->widget());
        if (input)
        {
            inputs.append(input->text()); // 获取输入框的内容
        }
    }
    return inputs;
}

void NumberPadDialog::onAccepted()
{
    // 获取所有输入框的内容
    QStringList inputs = getAllInputs();

    qDebug() << "[dialog] ok.";
    emit inputsAccepted(inputs, m_dialog_type);
    // 关闭对话框（如果需要的话）
    accept(); // 这里会自动调用 QDialog::accept()
}

void NumberPadDialog::onRejected()
{
    qDebug() << "[dialog] cancel dialog.";
    reject(); // 这里会自动调用 QDialog::reject()
}
