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


    void addVariable(const QString& name);
    QStringList plottedVariables() const;

private Q_SLOTS:
    void onEvents(const mobsya::ThymioNode::EventMap& events);
    void onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars);

private:
    void plot(const QString& name, const QVariant& value, QVector<QtCharts::QXYSeries*>& series);

    QMap<QString, QVector<QtCharts::QXYSeries*>> m_events;
    QMap<QString, QVector<QtCharts::QXYSeries*>> m_variables;

    std::shared_ptr<const mobsya::ThymioNode> m_thymio;
    QtCharts::QChart* m_chart;
    QtCharts::QDateTimeAxis* m_xAxis;
    QtCharts::QValueAxis* m_yAxis;

    bool m_range_init = false;
};

}  // namespace Aseba