#pragma once
#include <QtWidgets>
#include <aseba/qt-thymio-dm-client-lib/thymionode.h>
#include <QPushButton>


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
    void setThymio(std::shared_ptr<mobsya::ThymioNode> node);
    const std::shared_ptr<mobsya::ThymioNode> thymio() const;

    void addEvent(const QString& name);
    QStringList plottedEvents() const;


    void addVariable(const QString& name);
    QStringList plottedVariables() const;

    void hideEvent(QHideEvent*) override;
    void showEvent(QShowEvent*) override;

private Q_SLOTS:
    void onEvents(const mobsya::ThymioNode::EventMap& events, const QDateTime& timestamp);
    void onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars, const QDateTime& timestamp);
    void clearData();
    void exportData();
    void toggleTimeWindow(bool);
    void togglePause(bool);


private:
    void plot(const QString& name, const QVariant& value, QVector<QtCharts::QXYSeries*>& series,
              const QDateTime& timestamp);

    QMap<QString, QVector<QtCharts::QXYSeries*>> m_events;
    QMap<QString, QVector<QtCharts::QXYSeries*>> m_variables;

    std::shared_ptr<mobsya::ThymioNode> m_thymio;
    QtCharts::QChart* m_chart;
    QtCharts::QValueAxis* m_xAxis;
    QtCharts::QValueAxis* m_yAxis;
    
    QPushButton* reloadButton; 
    QPushButton* exportButton;
    QCheckBox *timewindowCb;
    QCheckBox *pauseCb;


    QAction* reloadAct;
    QSpacerItem* spacer;

    bool m_range_init = false;
    bool time_window_enabled = false;
    bool pause = false;

    QDateTime m_start = QDateTime::currentDateTime();
};

}  // namespace Aseba