#include "PlotTab.h"
#include <QtCharts>
#include <QVBoxLayout>

namespace Aseba {

PlotTab::PlotTab(QWidget* parent) : QSplitter(parent) {

    auto layout = new QVBoxLayout;
    m_chart = new QtCharts::QChart();
    QChartView* chartView = new QChartView(m_chart);
    chartView->setRubberBand(QChartView::VerticalRubberBand);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);
    this->setLayout(layout);


    m_xAxis = new QDateTimeAxis;
    // m_xAxis->setTickCount(1000);
    m_xAxis->setFormat("hh::mm:ss.z");
    m_chart->addAxis(m_xAxis, Qt::AlignBottom);
    m_xAxis->setRange(QDateTime::currentDateTime(), QDateTime::currentDateTime().addSecs(1));

    m_yAxis = new QValueAxis;
    m_yAxis->setLabelFormat("%i");
    m_yAxis->setTitleText(tr("Values"));
    m_chart->addAxis(m_yAxis, Qt::AlignLeft);
}


void PlotTab::setThymio(std::shared_ptr<const mobsya::ThymioNode> node) {
    m_thymio = node;
    if(node) {
        // node->setWatchEventsEnabled(true);
        connect(node.get(), &mobsya::ThymioNode::events, this, &PlotTab::onEvents);

        // connect(node.get(), &mobsya::ThymioNode::vmExecutionStateChanged, this, &NodeTab::updateStatusLabel);
        // connect(node.get(), &mobsya::ThymioNode::statusChanged, this, &NodeTab::updateStatusLabel);
    }
}

const std::shared_ptr<const mobsya::ThymioNode> PlotTab::thymio() const {
    return m_thymio;
}


void PlotTab::addEvent(const QString& name) {
    if(!m_events.contains(name))
        m_events.insert(name, {});
}

QStringList PlotTab::plottedEvents() const {
    return m_events.keys();
}

void PlotTab::onEvents(const mobsya::ThymioNode::EventMap& events) {
    for(auto it = events.begin(); it != events.end(); ++it) {
        auto event = m_events.find(it.key());
        if(event == m_events.end())
            continue;
        const QVariant& v = it.value();
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
                event.value().resize(std::max(i + 1, event.value().size()));
                auto& serie = event.value()[i];
                if(!serie) {
                    serie = new QtCharts::QSplineSeries(this);
                    serie->setName(QStringLiteral("%1[%2]").arg(event.key()).arg(i));
                    m_chart->addSeries(serie);
                    serie->attachAxis(m_xAxis);
                    serie->attachAxis(m_yAxis);
                    serie->setPointsVisible();
                }
                serie->append(QDateTime::currentMSecsSinceEpoch(), d);
                if(d < m_yAxis->min()) {
                    m_yAxis->setMin(d - 0.2 * std::abs(d));
                }
                if(d > m_yAxis->max() * 0.9) {
                    m_yAxis->setMax(d + 0.2 * std::abs(d));
                }
            }
        }
    }

    m_xAxis->setMax(QDateTime::currentDateTime());

    qreal x = m_chart->plotArea().width() / (m_xAxis->tickCount() * 100);
    m_chart->scroll(1, 0);
}

}  // namespace Aseba