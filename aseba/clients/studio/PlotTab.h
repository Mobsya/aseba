#pragma once
#include <QtWidgets>
#include <aseba/qt-thymio-dm-client-lib/thymionode.h>


namespace QtCharts {
class QChart;
class QXYSeries;
class QDateTimeAxis;
class QValueAxis;
}  // namespace QtCharts

namespace Aseba {

class PlotTab : public QSplitter {
    Q_OBJECT

public:
    PlotTab(QWidget* parent = nullptr);
    void setThymio(std::shared_ptr<const mobsya::ThymioNode> node);
    const std::shared_ptr<const mobsya::ThymioNode> thymio() const;

    void addEvent(const QString& name);
    QStringList plottedEvents() const;

private Q_SLOTS:
    void onEvents(const mobsya::ThymioNode::EventMap& events);

private:
    QMap<QString, QVector<QtCharts::QXYSeries*>> m_events;
    std::shared_ptr<const mobsya::ThymioNode> m_thymio;
    QtCharts::QChart* m_chart;
    QtCharts::QDateTimeAxis* m_xAxis;
    QtCharts::QValueAxis* m_yAxis;

    double m_min = 0, m_max = 0;
};

}  // namespace Aseba