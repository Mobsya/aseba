#include "EventsWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>

#include <aseba/common/consts.h>
#include "CustomWidgets.h"
#include "CustomDelegate.h"


namespace Aseba {


EventsWidget::EventsWidget(QWidget* parent) : QWidget(parent) {

    auto* eventsDockLayout = new QVBoxLayout;


    auto eventsDescriptionsView = new FixedWidthTableView;
    eventsDescriptionsView->setShowGrid(false);
    eventsDescriptionsView->verticalHeader()->hide();
    eventsDescriptionsView->horizontalHeader()->hide();
    // eventsDescriptionsView->setModel(eventsDescriptionsModel);
    eventsDescriptionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    eventsDescriptionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    eventsDescriptionsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventsDescriptionsView->setDragDropMode(QAbstractItemView::InternalMove);
    eventsDescriptionsView->setDragEnabled(true);
    eventsDescriptionsView->setDropIndicatorShown(true);
    eventsDescriptionsView->setItemDelegateForColumn(1, new SpinBoxDelegate(0, ASEBA_MAX_EVENT_ARG_COUNT, this));
    eventsDescriptionsView->setMinimumHeight(100);
    // eventsDescriptionsView->setSecondColumnLongestContent("255###");
    eventsDescriptionsView->resizeRowsToContents();
    eventsDescriptionsView->setContextMenuPolicy(Qt::CustomContextMenu);

    QHBoxLayout* eventsAddRemoveLayout = new QHBoxLayout;
    eventsAddRemoveLayout->addWidget(new QLabel(tr("<b>Events</b>")));
    eventsAddRemoveLayout->addStretch();
    auto addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
    eventsAddRemoveLayout->addWidget(addEventNameButton);
    auto removeEventNameButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
    removeEventNameButton->setEnabled(false);
    eventsAddRemoveLayout->addWidget(removeEventNameButton);
    auto sendEventButton = new QPushButton(QPixmap(QString(":/images/newmsg.png")), "");
    sendEventButton->setEnabled(false);
    eventsAddRemoveLayout->addWidget(sendEventButton);

    eventsDockLayout->addLayout(eventsAddRemoveLayout);

    eventsDockLayout->addWidget(eventsDescriptionsView, 1);


    addEventNameButton->setToolTip(tr("Add a new event"));
    removeEventNameButton->setToolTip(tr("Remove this event"));
    sendEventButton->setToolTip(tr("Send this event"));

    auto* eventsLayout = new QGridLayout;
    eventsLayout->addWidget(new QLabel(tr("<b>Global Events</b>")), 0, 0, 1, 4);
    eventsLayout->addWidget(addEventNameButton, 1, 0);
    // eventsLayout->setColumnStretch(2, 0);
    eventsLayout->addWidget(removeEventNameButton, 1, 1);
    // eventsLayout->setColumnStretch(3, 0);
    // eventsLayout->setColumnStretch(0, 1);
    eventsLayout->addWidget(sendEventButton, 1, 2);
    // eventsLayout->setColumnStretch(1, 0);
    eventsLayout->addWidget(eventsDescriptionsView, 2, 0, 1, 4);


    auto logger = new QListWidget;
    logger->setMinimumSize(80, 100);
    logger->setSelectionMode(QAbstractItemView::NoSelection);
    eventsDockLayout->addWidget(logger, 3);
    auto clearLogger = new QPushButton(tr("Clear"));
    eventsDockLayout->addWidget(clearLogger);

    setLayout(eventsDockLayout);
}
}  // namespace Aseba

/**/

/*
 */

// panel
// auto* rightPanelSplitter = new QSplitter(Qt::Vertical);

// QWidget* constantsWidget = new QWidget;
// constantsWidget->setLayout(constantsLayout);
// rightPanelSplitter->addWidget(constantsWidget);

// QWidget* eventsWidget = new QWidget;
// eventsWidget->setLayout(eventsLayout);
// rightPanelSplitter->addWidget(eventsWidget);

// QWidget* loggerWidget = new QWidget;
// loggerWidget->setLayout(loggerLayout);
// rightPanelSplitter->addWidget(loggerWidget);

// main window

// splitter->addWidget(rightPanelSplitter);