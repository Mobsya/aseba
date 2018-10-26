#include "EventsWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>

#include <aseba/common/consts.h>
#include "CustomWidgets.h"
#include "CustomDelegate.h"
#include "NewNamedValueDialog.h"


namespace Aseba {


EventsWidget::EventsWidget(QWidget* parent) : QWidget(parent) {

    auto* eventsDockLayout = new QVBoxLayout;


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
    m_view->setItemDelegateForColumn(1, new SpinBoxDelegate(0, ASEBA_MAX_EVENT_ARG_COUNT, this));
    m_view->setMinimumHeight(100);
    m_view->setSecondColumnLongestContent("255###");
    m_view->resizeRowsToContents();
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);

    QHBoxLayout* eventsAddRemoveLayout = new QHBoxLayout;
    eventsAddRemoveLayout->addWidget(new QLabel(tr("<b>Events</b>")));
    eventsAddRemoveLayout->addStretch();
    auto addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
    eventsAddRemoveLayout->addWidget(addEventNameButton);
    m_removeEventButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
    m_removeEventButton->setEnabled(false);
    eventsAddRemoveLayout->addWidget(m_removeEventButton);
    m_sendEventButton = new QPushButton(QPixmap(QString(":/images/newmsg.png")), "");
    m_sendEventButton->setEnabled(false);
    eventsAddRemoveLayout->addWidget(m_sendEventButton);

    eventsDockLayout->addLayout(eventsAddRemoveLayout);

    eventsDockLayout->addWidget(m_view, 1);


    addEventNameButton->setToolTip(tr("Add a new event"));
    m_removeEventButton->setToolTip(tr("Remove this event"));
    m_sendEventButton->setToolTip(tr("Send this event"));

    auto* eventsLayout = new QGridLayout;
    eventsLayout->addWidget(new QLabel(tr("<b>Global Events</b>")), 0, 0, 1, 4);
    eventsLayout->addWidget(addEventNameButton, 1, 0);
    eventsLayout->addWidget(m_removeEventButton, 1, 1);
    eventsLayout->addWidget(m_sendEventButton, 1, 2);
    eventsLayout->addWidget(m_view, 2, 0, 1, 4);


    auto logger = new QListWidget;
    logger->setMinimumSize(80, 100);
    logger->setSelectionMode(QAbstractItemView::NoSelection);
    eventsDockLayout->addWidget(logger, 3);
    auto clearLogger = new QPushButton(tr("Clear"));
    eventsDockLayout->addWidget(clearLogger);

    setLayout(eventsDockLayout);


    connect(addEventNameButton, &QPushButton::clicked, this, &EventsWidget::addEvent);
    connect(m_removeEventButton, &QPushButton::clicked, this, &EventsWidget::removeEvent);
    connect(m_sendEventButton, &QPushButton::clicked, this, &EventsWidget::sendEvent);
}

void EventsWidget::setModel(QAbstractItemModel* model) {
    m_view->setModel(model);
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &EventsWidget::eventsSelectionChanged);
}


void EventsWidget::addEvent() {
    QString eventName;
    int eventNbArgs = 0;
    // prompt the user for the named value
    const bool ok = NewNamedValueDialog::getNamedValue(&eventName, &eventNbArgs, 0, ASEBA_MAX_EVENT_ARG_COUNT,
                                                       tr("Add a new event"), tr("Name:"),
                                                       tr("Number of arguments", "For the newly created event"));
    if(ok && !eventName.isEmpty()) {
        Q_EMIT eventAdded(eventName, eventNbArgs);
    }
}

void EventsWidget::removeEvent() {
    auto r = m_view->selectionModel()->selectedRows();
    if(r.empty())
        return;
    auto name = r.first().data(Qt::DisplayRole);
    Q_EMIT eventRemoved(name.toString());
}

void EventsWidget::eventsSelectionChanged() {
    bool isSelected = m_view->selectionModel()->currentIndex().isValid();
    m_removeEventButton->setEnabled(isSelected);
    m_sendEventButton->setEnabled(isSelected);
}

void EventsWidget::sendEvent() {
    auto r = m_view->selectionModel()->selectedRows();
    if(r.empty())
        return;
    auto name = r.first().data(Qt::DisplayRole).toString();
    auto count = m_view->selectionModel()->selectedRows(1).first().data(Qt::DisplayRole).toInt();
    QVariantList data;

    if(count > 0) {
        QString argList;
        while(true) {
            bool ok;
            argList = QInputDialog::getText(this, tr("Specify event arguments"),
                                            tr("Please specify the %0 arguments of event %1").arg(count).arg(name),
                                            QLineEdit::Normal, argList, &ok);
            if(ok) {
                QStringList args = argList.split(QRegExp("[\\s,]+"), QString::SkipEmptyParts);
                if(args.size() != count) {
                    QMessageBox::warning(
                        this, tr("Wrong number of arguments"),
                        tr("You gave %0 arguments where event %1 requires %2").arg(args.size()).arg(name).arg(count));
                    continue;
                }
                for(int i = 0; i < args.size(); i++) {
                    data.append(args.at(i).toShort(&ok));
                    if(!ok) {
                        QMessageBox::warning(this, tr("Invalid value"),
                                             tr("Invalid value for argument %0 of event %1").arg(i).arg(name));
                        break;
                    }
                }
                if(ok)
                    break;
            } else
                return;
        }
    }
    Q_EMIT eventEmitted(name, QVariant::fromValue(data));

    //  target->sendEvent(eventId, data);
    //  userEvent(eventId, data);
}

// void MainWindow::addEventNameClicked() {


/* // prompt the user for the named value


 eventName = eventName.trimmed();
 if(ok && !eventName.isEmpty()) {
     if(commonDefinitions.events.contains(eventName.toStdWString())) {
         QMessageBox::warning(this, tr("Event already exists"), tr("Event %0 already exists.").arg(eventName));
     } else if(!QRegExp(R"(\w(\w|\.)*)").exactMatch(eventName) || eventName[0].isDigit()) {
         QMessageBox::warning(this, tr("Invalid event name"),
                              tr("Event %0 has an invalid name. Valid names start with an "
                                 "alphabetical character or an \"_\", and continue with any "
                                 "number of alphanumeric characters, \"_\" and \".\"")
                                  .arg(eventName));
     } else {
         eventsDescriptionsModel->addNamedValue(NamedValue(eventName.toStdWString(), eventNbArgs));
     }
 }*/
//}  // namespace Aseba

/*void MainWindow::removeEventNameClicked() {
    /*    QModelIndex currentRow = m_view->selectionModel()->currentIndex();
        Q_ASSERT(currentRow.isValid());
        // eventsDescriptionsModel->delNamedValue(currentRow.row());

        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab)
                tab->isSynchronized = false;
        }
}

void MainWindow::eventsUpdated(bool indexChanged) {
    if(indexChanged) {
        // statusText->setText(tr("Desynchronised! Please reload."));
        // statusText->show();
    }
    recompileAll();
    updateWindowTitle();
}

void MainWindow::eventsUpdatedDirty() {
    eventsUpdated(true);
}

void MainWindow::eventsDescriptionsSelectionChanged() {
    // bool isSelected = m_view->selectionModel()->currentIndex().isValid();
    // removeEventNameButton->setEnabled(isSelected);
    // sendEventButton->setEnabled(isSelected);
#ifdef HAVE_QWT
    plotEventButton->setEnabled(isSelected);
#endif  // HAVE_QWT
}*/

}  // namespace Aseba
