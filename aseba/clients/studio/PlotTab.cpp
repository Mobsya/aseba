#include "PlotTab.h"
#include <QtCharts>
#include <QVBoxLayout>

namespace Aseba {

PlotTab::PlotTab(QWidget* parent) : QWidget(parent) {

    auto layout = new QVBoxLayout;
    m_chart = new QtCharts::QChart();
    QChartView* chartView = new QChartView(m_chart, this);
    chartView->setRubberBand(QChartView::VerticalRubberBand);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);
    this->setLayout(layout);


    m_xAxis = new QValueAxis;
    // m_xAxis->setFormat("hh::mm:ss.z");
    m_chart->addAxis(m_xAxis, Qt::AlignBottom);

    m_yAxis = new QValueAxis;
    m_yAxis->setLabelFormat("%i");
    m_yAxis->setTitleText(tr("Values"));
    m_chart->addAxis(m_yAxis, Qt::AlignLeft);

    /*QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this]() { m_chart->scroll(-5, 0); });
    t->start(100);
    */
}


void PlotTab::setThymio(std::shared_ptr<mobsya::ThymioNode> node) {
    m_thymio = node;
    if(node) {
        m_thymio->setWatchEventsEnabled(true);
        connect(node.get(), &mobsya::ThymioNode::events, this, &PlotTab::onEvents);
        connect(node.get(), &mobsya::ThymioNode::variablesChanged, this, &PlotTab::onVariablesChanged);
        // connect(node.get(), &mobsya::ThymioNode::vmExecutionStateChanged, this, &NodeTab::updateStatusLabel);
        // connect(node.get(), &mobsya::ThymioNode::statusChanged, this, &NodeTab::updateStatusLabel);
    }
}

const std::shared_ptr<mobsya::ThymioNode> PlotTab::thymio() const {
    return m_thymio;
}


void PlotTab::addEvent(const QString& name) {
    if(!m_events.contains(name))
        m_events.insert(name, {});
}

QStringList PlotTab::plottedEvents() const {
    return m_events.keys();
}

void PlotTab::addVariable(const QString& name) {
    if(!m_variables.contains(name))
        m_variables.insert(name, {});
}

QStringList PlotTab::plottedVariables() const {
    return m_variables.keys();
}

void PlotTab::onEvents(const mobsya::ThymioNode::EventMap& events, const QDateTime& timestamp) {
    for(auto it = events.begin(); it != events.end(); ++it) {
        auto event = m_events.find(it.key());
        if(event == m_events.end())
            continue;
        const QVariant& v = it.value();
        plot(event.key(), v, event.value(), timestamp);
    }
    // previous 2000 data point ( 1 every 10 milliseconds) = 20 seconds window
    const int nPreviousData = 2000;
    m_xAxis->setMax(m_start.msecsTo(timestamp) + 10);
    m_xAxis->setMin(m_start.msecsTo(timestamp) - 10 * nPreviousData);
}

void PlotTab::onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars, const QDateTime& timestamp) {
    for(auto it = vars.begin(); it != vars.end(); ++it) {
        auto event = m_variables.find(it.key());
        if(event == m_variables.end())
            continue;
        const QVariant& v = it.value().value();
        plot(event.key(), v, event.value(), timestamp);
    }
    // previous 2000 data point ( 1 every 10 milliseconds) = 20 seconds window
    const int nPreviousData = 2000;
    m_xAxis->setMax(m_start.msecsTo(timestamp) + 10);
    m_xAxis->setMin(m_start.msecsTo(timestamp) - 10 * nPreviousData);
}


void PlotTab::plot(const QString& name, const QVariant& v, QVector<QtCharts::QXYSeries*>& series,
                   const QDateTime& timestamp) {
    if(v.type() == QVariant::List) {
        auto list = v.toList();
        auto i = -1;
        for(auto&& e : list) {
            i++;
            bool ok = false;
            double d = e.toDouble(&ok);
            if(!ok) {
                continue;
            }
            series.resize(std::max(i + 1, series.size()));
            auto& serie = series[i];
            if(!serie) {
                serie = new QtCharts::QLineSeries(this);
                serie->setName(QStringLiteral("%1[%2]").arg(name).arg(i));
                serie->setUseOpenGL(true);
                m_chart->addSeries(serie);
                serie->attachAxis(m_xAxis);
                serie->attachAxis(m_yAxis);
                serie->setPointsVisible();
                if(!m_range_init) {
                    m_yAxis->setRange(d, d);
                }
            }
            if(d < m_yAxis->min()) {
                m_yAxis->setMin(d - 1);
            }
            if(d > m_yAxis->max()) {
                m_yAxis->setMax(d + 1);
            }
            serie->append(m_start.msecsTo(timestamp), d);
        }
    }
}

void PlotTab::hideEvent(QHideEvent*) {
    if(m_thymio) {
        m_thymio->setWatchVariablesEnabled(false);
    }
}

void PlotTab::showEvent(QShowEvent*) {
    if(m_thymio) {
        m_thymio->setWatchVariablesEnabled(true);
    }
}

}  // namespace Aseba