#include "ConstantsWidget.h"
#include "CustomDelegate.h"
#include <QPushButton>
#include <QHeaderView>
#include <QLabel>
#include <QGridLayout>
#include <QSpinBox>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include "NewNamedValueDialog.h"

namespace Aseba {

ConstantsWidget::ConstantsWidget(QWidget* parent) : QWidget(parent) {

    m_addConstantButton = new QPushButton(QPixmap(QString(":/images/add.png")), QLatin1String(""));
    m_removeConstantButton = new QPushButton(QPixmap(QString(":/images/remove.png")), QLatin1String(""));
    m_addConstantButton->setToolTip(tr("Add a new constant"));
    m_removeConstantButton->setToolTip(tr("Remove this constant"));
    m_removeConstantButton->setEnabled(false);

    m_view = new FixedWidthTableView;
    m_view->setShowGrid(false);
    m_view->verticalHeader()->hide();
    m_view->horizontalHeader()->hide();

    m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setDragDropMode(QAbstractItemView::InternalMove);
    m_view->setDragEnabled(true);
    m_view->setDropIndicatorShown(true);
    m_view->setItemDelegateForColumn(1, new SpinBoxDelegate(-32768, 32767, this));
    m_view->setMinimumHeight(100);
    m_view->setSecondColumnLongestContent("-88888##");
    m_view->resizeRowsToContents();

    auto* constantsLayout = new QGridLayout;
    constantsLayout->addWidget(new QLabel(tr("<b>Constants</b>")), 0, 0);
    constantsLayout->setColumnStretch(0, 1);
    constantsLayout->addWidget(m_addConstantButton, 0, 1);
    constantsLayout->setColumnStretch(1, 0);
    constantsLayout->addWidget(m_removeConstantButton, 0, 2);
    constantsLayout->setColumnStretch(2, 0);
    constantsLayout->addWidget(m_view, 1, 0, 1, 3);
    this->setLayout(constantsLayout);


    connect(m_addConstantButton, &QPushButton::clicked, this, &ConstantsWidget::addConstant);
    connect(m_removeConstantButton, &QPushButton::clicked, this, &ConstantsWidget::removeConstant);
}

void ConstantsWidget::setModel(QAbstractItemModel* model) {
    m_view->setModel(model);
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ConstantsWidget::constantsSelectionChanged);
}


void ConstantsWidget::addConstant() {
    bool ok;
    QString constantName;
    int constantValue = 0;
    // prompt the user for the named value
    ok = NewNamedValueDialog::getNamedValue(&constantName, &constantValue, -32768, 32767, tr("Add a new constant"),
                                            tr("Name:"), tr("Value", "Value assigned to the constant"));

    if(ok && !constantName.isEmpty()) {
        Q_EMIT constantModified(constantName, constantValue);
    }
}

void ConstantsWidget::removeConstant() {
    auto r = m_view->selectionModel()->selectedRows();
    if(r.empty())
        return;
    auto name = r.first().data(Qt::DisplayRole);
    Q_EMIT constantModified(name.toString(), {});
}

void ConstantsWidget::constantsSelectionChanged() {
    bool isSelected = m_view->selectionModel()->currentIndex().isValid();
    m_removeConstantButton->setEnabled(isSelected);
}

}  // namespace Aseba