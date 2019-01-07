#pragma once
#include <QWidget>
#include <QAbstractItemModel>
#include <QPushButton>
#include <QListWidget>
#include "CustomWidgets.h"
#include "thymionode.h"

namespace Aseba {

class EventsWidget : public QWidget {
    Q_OBJECT
public:
    EventsWidget(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel* model);

public Q_SLOTS:
    void onEvents(const mobsya::ThymioNode::VariableMap& events);
    void logError(mobsya::ThymioNode::VMExecutionError error, const QString& message, uint32_t line);
    void setEditable(bool editable);

Q_SIGNALS:
    void eventAdded(const QString& name, int size);
    void eventRemoved(const QString& name);
    void eventEmitted(const QString& name, const QVariant& value);

private Q_SLOTS:
    void addEvent();
    void removeEvent();
    void eventsSelectionChanged();
    void sendSelectedEvent();
    void sendEvent(const QModelIndex& i);
    void onDoubleClick(const QModelIndex& index);

private:
    FixedWidthTableView* m_view;
    QListWidget* m_logger;
    QPushButton* m_addEventNameButton;
    QPushButton* m_removeEventButton;
    QPushButton* m_sendEventButton;
    bool m_editable = false;
};

}  // namespace Aseba