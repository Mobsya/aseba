#pragma once
#include <QWidget>
#include "QPushButton"
#include "CustomWidgets.h"

namespace Aseba {

class ConstantsWidget : public QWidget {
    Q_OBJECT
public:
    ConstantsWidget(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel* model);

Q_SIGNALS:
    void constantModified(const QString& name, const QVariant& value);

public Q_SLOTS:
    void setEditable(bool editable);

private Q_SLOTS:
    void addConstant();
    void removeConstant();
    void constantsSelectionChanged();

private:
    FixedWidthTableView* m_view;
    QPushButton* m_removeConstantButton;
    QPushButton* m_addConstantButton;
    bool m_editable = false;
};

}  // namespace Aseba