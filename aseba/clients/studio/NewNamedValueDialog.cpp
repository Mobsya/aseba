#include "NewNamedValueDialog.h"
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Aseba {

NewNamedValueDialog::NewNamedValueDialog(QString* name, int* value, int min, int max) : name(name), value(value) {
    // create the widgets
    label1 = new QLabel(tr("Name", "Name of the named value (can be a constant, event,...)"));
    line1 = new QLineEdit(*name);
    label2 = new QLabel(tr("Default description", "When no description is given for the named value"));
    line2 = new QSpinBox();
    line2->setRange(min, max);
    line2->setValue(*value);
    QLabel* lineHelp = new QLabel(QStringLiteral("(%1 ... %2)").arg(min).arg(max));
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // create the layout
    auto* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(label1);
    mainLayout->addWidget(line1);
    mainLayout->addWidget(label2);
    mainLayout->addWidget(line2);
    mainLayout->addWidget(lineHelp);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    // set modal
    setModal(true);

    // connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewNamedValueDialog::okSlot);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewNamedValueDialog::cancelSlot);
}

bool NewNamedValueDialog::getNamedValue(QString* name, int* value, int min, int max, QString title, QString valueName,
                                        QString valueDescription) {
    NewNamedValueDialog dialog(name, value, min, max);
    dialog.setWindowTitle(title);
    dialog.label1->setText(valueName);
    dialog.label2->setText(valueDescription);
    dialog.resize(500, 0);  // make it wide enough

    int ret = dialog.exec();

    if(ret)
        return true;
    else
        return false;
}

bool NewNamedValueDialog::modifyNamedValue(QString* name, int* value, int min, int max, QString title,
                                           QString valueName, QString valueDescription) {
    NewNamedValueDialog dialog(name, value, min, max);
    dialog.setWindowTitle(title);
    dialog.label1->setText(valueName);
    dialog.line1->setReadOnly(true);
    dialog.label2->setText(valueDescription);
    dialog.resize(500, 0);  // make it wide enough

    int ret = dialog.exec();

    if(ret)
        return true;
    else
        return false;
}


void NewNamedValueDialog::okSlot() {
    *name = line1->text();
    *value = line2->value();
    accept();
}

void NewNamedValueDialog::cancelSlot() {
    *name = QLatin1String("");
    *value = -1;
    reject();
}

}  // namespace Aseba