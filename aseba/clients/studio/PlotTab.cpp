#include "PlotTab.h"
#include <QtCharts>
#include <QVBoxLayout>
#include <QPushButton>

namespace Aseba {

/* The function contains the widget for showing the control panel for the GraphPlot
* 
* \return: 
*/
PlotTab::PlotTab(QWidget* parent) : QWidget(parent) {

    auto layout = new QVBoxLayout;
    auto topButtonsLayout = new QHBoxLayout;

    reloadButton = new QPushButton();
    reloadButton->setIcon(QIcon(":/images/rescan.png"));
    reloadButton->setToolTip(tr("Reload"));
    reloadButton->setText(tr("Reload"));

    topButtonsLayout->addWidget(reloadButton);
    
    exportButton = new QPushButton();
    exportButton->setIcon(QIcon(":/images/export.png"));
    exportButton->setToolTip(tr("Export Data to CSV file"));
    exportButton->setText(tr("Export Data"));
    topButtonsLayout->addWidget(exportButton);

    QCheckBox *timewindowCb = new QCheckBox("time window", this);;
    topButtonsLayout->addWidget(timewindowCb);

    QCheckBox *pauseCb = new QCheckBox("Pause", this);;
    topButtonsLayout->addWidget(pauseCb);
    
    layout->addLayout(topButtonsLayout);

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

    connect(reloadButton,   SIGNAL(clicked()), this, SLOT(clearData()));
    connect(exportButton,   SIGNAL(clicked()), this, SLOT(exportData()));
    connect(timewindowCb,   SIGNAL(toggled(bool)), this, SLOT(toggleTimeWindow(bool)));
    connect(pauseCb,        SIGNAL(toggled(bool)), this, SLOT(togglePause(bool)));

    /*QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this]() { m_chart->scroll(-5, 0); });
    t->start(100);
    */
}

void PlotTab::togglePause(bool selected){

    if(selected){
        pause = true;
    }

    else{
        pause = false;
    }
}

void PlotTab::toggleTimeWindow(bool selected){
    FILE* flog = fopen("/Users/vale/Projects/aseba/LOGGER", "a+");
    // fprintf(flog, "#!/bin/bash\ngdb --args %s ", cpg);
    // fprintf(flog, "TOGGLE \n");
    // fflush(flog);
    if(selected){
        time_window_enabled = true;
        fprintf(flog, "yes \n");
        fflush(flog);
    }
    else{
        time_window_enabled = false;
        fprintf(flog, "no \n");
        fflush(flog);
    }

    fclose(flog);

}


/* the function is internal 
* it prints to a file a CSV formatted header 
* for the data contained in the time serie
* \return: 1 if everything went fine, 0 otherwise 
*/
int print_header( QString name, int how_many,  FILE* flog){
    if (flog == nullptr)
        return 0;

    // we first print the table heading with all the values        
    for(int i = 0; i < how_many; i++){
            //the event name and index if there are several
            fprintf(flog, "\" %s[%d]\" ,",name.toStdString().c_str(),i);
    }
    //the last contained value would be the timenstamp
    fprintf(flog, " \"timestamp\"\n ");
    fflush(flog);
    return 1;
}

/* _internal_ : the function prints to a given file 
* several CSV formatted rows, one for each data entry for time serie 
* for a n values data the format of the row at time t_i is the following: 
*  data_val_1_i, data_val_2_i, ... , data_val_n_i, time_i
* \return: 1 if everything went fine, 0 otherwise 
*/
int print_rows( QVector<QtCharts::QXYSeries*> series, FILE* flog){
    
    // we now save many row containing a value for each element in the serie 
    // all the value has the same timestamp since all the elements are emitted 
    // by the same emitter 
    int val = 0;
    int max_val = series.at(0)->count();

    while(val < max_val ){

        for(int i = 0; i < series.count() ; i++){
            auto p_val =  series.at(i)->at(val);
            fprintf(flog, "%f,",p_val.ry());
        }

        auto timestamp_val = series.at(0)->at(val);
        // adding the time taken from the first element - we know is the same for all
        fprintf(flog, "%f\n",timestamp_val.rx());
        val++;
    }
    fflush(flog);
    return 1;
}


// for each of the events and variable logged we export a data file
// related to the plotting session
void PlotTab::exportData() {

    FILE* flog = fopen("/Users/vale/Projects/aseba/export.csv", "w+");



    //we iterate on each of the several events plotted if async
    for(auto it = m_events.begin(); it != m_events.end(); ++it) {
        QString name = it.key();
        QVector<QtCharts::QXYSeries*> series = it.value();
        
        if( print_header (name, series.count(), flog) <= 0 )
            return ;
        if( print_rows   (series, flog) <= 0 )
            return ;
    }

    //we iterate on each of the several variables plotted if any
    for(auto it = m_variables.begin(); it != m_variables.end(); ++it) {
        QString name = it.key();
        QVector<QtCharts::QXYSeries*> series = it.value();
        
        if( print_header (name, series.count(), flog) <= 0 )
            return ;
        if( print_rows   (series, flog) <= 0 )
            return ;
    }

    fclose(flog);
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

/* The function updates the local data structures containing all events data
* is called by the event callback system raised by ????
* \return : none
*/ 
void PlotTab::onEvents(const mobsya::ThymioNode::EventMap& events, const QDateTime& timestamp) {
    // if the system is on pause the event is not recorded - is not showed - the plot does not proceed
    if(pause)
        return;
    for(auto it = events.begin(); it != events.end(); ++it) {
        auto event = m_events.find(it.key());
        if(event == m_events.end())
            continue;
        const QVariant& v = it.value();
        plot(event.key(), v, event.value(), timestamp);
    }

    //[2do] this 10 is wrong since we don t know if 10ms were passed
    // from the previous rendering cycle
    m_xAxis->setMax(m_start.msecsTo(timestamp) + 10);
}

/* The function updates the local data structures containing all events data
* is called by the event callback system raised by ????
* \return : none
*/ 
void PlotTab::onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars, const QDateTime& timestamp) {
    // if the system is on pause the variable is not recorded - is not showed - the plot does not proceed
    if(pause)
        return;
    for(auto it = vars.begin(); it != vars.end(); ++it) {
        auto event = m_variables.find(it.key());
        if(event == m_variables.end())
            continue;
        const QVariant& v = it.value().value();
        plot(event.key(), v, event.value(), timestamp);
    }

    //[2do] this 10 is wrong since we don t know if 10ms were passed
    // from the previous rendering cycle
    m_xAxis->setMax(m_start.msecsTo(timestamp) + 10);
}

void PlotTab::clearData( ){
    for(auto it = m_events.begin(); it != m_events.end(); ++it) {
        auto series = it.value();
        for(auto it_s = series.begin(); it_s != series.end(); ++it_s) {
            auto serie = *it_s;
            serie->clear();
        }
    }
    for(auto it = m_variables.begin(); it != m_variables.end(); ++it) {
        auto series = it.value();
        for(auto it_s = series.begin(); it_s != series.end(); ++it_s) {
            auto serie = *it_s;
            serie->clear();
        }
    }

    m_start = QDateTime::currentDateTime();
    m_xAxis->setMin(0);
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


// we might pass trought this two functions for 
// properly enabling the pause function but since the effect of there async callbacks are 
// unknown we would not take this risk
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