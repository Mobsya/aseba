#pragma once
#include <QWidget>

namespace Aseba {

class EventsWidget : public QWidget {
    Q_OBJECT
public:
    EventsWidget(QWidget* parent = nullptr);
};

}  // namespace Aseba