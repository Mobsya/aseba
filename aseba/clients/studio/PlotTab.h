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

class PlotTab : public QWidget {
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
    void onEvents(const mobsya::ThymioNode::EventMap& events, const QDateTime& timestamp);
    void onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars, const QDateTime& timestamp);

private:
    void plot(const QString& name, const QVariant& value, QVector<QtCharts::QXYSeries*>& series,
              const QDateTime& timestamp);

    QMap<QString, QVector<QtCharts::QXYSeries*>> m_events;
    QMap<QString, QVector<QtCharts::QXYSeries*>> m_variables;

    std::shared_ptr<const mobsya::ThymioNode> m_thymio;
    QtCharts::QChart* m_chart;
    QtCharts::QValueAxis* m_xAxis;
    QtCharts::QValueAxis* m_yAxis;

    bool m_range_init = false;
    QDateTime m_start = QDateTime::currentDateTime();
};

}  // namespace Aseba