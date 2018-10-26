#pragma once
#include <QWidget>
#include <QAbstractItemModel>
#include <QPushButton>

#include "CustomWidgets.h"
namespace Aseba {

class EventsWidget : public QWidget {
    Q_OBJECT
public:
    EventsWidget(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel* model);

Q_SIGNALS:
    void eventAdded(const QString& name, int size);
    void eventRemoved(const QString& name);
private Q_SLOTS:
    void addEvent();
    void removeEvent();
    void eventsSelectionChanged();

private:
    FixedWidthTableView* m_view;
    QPushButton* m_removeEventButton;
};

}  // namespace Aseba