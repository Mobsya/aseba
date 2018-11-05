#pragma once
#include <QDialog>
#include <QLabel>
#include <QSpinBox>

namespace Aseba {

class NewNamedValueDialog : public QDialog {
    Q_OBJECT

public:
    NewNamedValueDialog(QString* name, int* value, int min, int max);
    static bool getNamedValue(QString* name, int* value, int min, int max, QString title, QString valueName,
                              QString valueDescription);

protected slots:
    void okSlot();
    void cancelSlot();

protected:
    QLabel* label1;
    QLineEdit* line1;
    QLabel* label2;
    QSpinBox* line2;

    QString* name;
    int* value;
};


}  // namespace Aseba