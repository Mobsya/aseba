#include "NodeTabsManager.h"
namespace Aseba {

//////

NodeTabsManager::NodeTabsManager(const mobsya::ThymioDeviceManagerClient& client) : m_client(client) {
    connect(this, &QTabWidget::currentChanged, this, &NodeTabsManager::tabChanged);
    connect(this, &QTabWidget::tabCloseRequested, this, &NodeTabsManager::onTabClosed);

    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeAdded, this, &NodeTabsManager::onNodeAdded);
    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeModified, this, &NodeTabsManager::onNodeModified);
    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeRemoved, this, &NodeTabsManager::onNodeRemoved);

    setTabsClosable(true);
}

NodeTabsManager::~NodeTabsManager() {}

void NodeTabsManager::addTab(const QUuid& device) {
    if(m_tabs.contains(device))
        return;
    auto tab = new NodeTab();
    const int index = QTabWidget::addTab(tab, device.toString());
    m_tabs[device] = tab;
    if(auto thymio = m_client.node(device)) {
        tab->setThymio(thymio);
        QTabWidget::setTabText(index, thymio->name());
        addThymiosFromGroups();
    } else {
        QTabWidget::setTabEnabled(index, false);
    }

    tabBar()->setTabButton(index, QTabBar::RightSide, 0);
    tabBar()->setTabButton(index, QTabBar::LeftSide, 0);

    connect(tab, &NodeTab::executionStateChanged, this, &NodeTabsManager::nodeStatusChanged);

    connect(tab, &NodeTab::plotEventRequested, this,
            [this](QString event) { createPlotTab(qobject_cast<NodeTab*>(sender()), event, plot_type::event); });

    connect(tab, &NodeTab::plotVariableRequested, this, [this](QString variable) {
        createPlotTab(qobject_cast<NodeTab*>(sender()), variable, plot_type::variable);
    });
}

void NodeTabsManager::createPlotTab(NodeTab* tab, const QString& name, plot_type type) {
    if(!tab)
        return;
    auto thymio = tab->thymio();
    if(!thymio)
        return;
    createPlotTab(thymio, name, type);
}

void NodeTabsManager::createPlotTab(std::shared_ptr<const mobsya::ThymioNode> thymio, const QString& name,
                                    plot_type type) {
    PlotTab* t = nullptr;
    auto v =
        eventsTabs() | ranges::view::filter([&](PlotTab* t) {
            return t->thymio() == thymio &&
                (type == plot_type::event ? t->plottedEvents().contains(name) : t->plottedVariables().contains(name));
        });
    if(v.begin() == v.end()) {
        t = new PlotTab;
        t->setThymio(thymio);
        auto deviceTab = tabForNode(thymio);
        insertTab(indexOf(deviceTab) + 1, t, tr("%1 on %2").arg(name, thymio->name()));
    } else {
        t = *v.begin();
    }
    type == plot_type::event ? t->addEvent(name) : t->addVariable(name);
    setCurrentWidget(t);
}

void NodeTabsManager::onNodeAdded(std::shared_ptr<mobsya::ThymioNode> thymio) {
    if(thymio->status() == mobsya::ThymioNode::Status::Connected)
        return;

    auto it = m_tabs.find(thymio->uuid());
    if(it != m_tabs.end()) {
        (*it)->setThymio(thymio);
        auto idx = QTabWidget::indexOf(*it);
        QTabWidget::setTabEnabled(idx, true);
        tabBar()->setTabText(idx, thymio->name());
    }
    addThymiosFromGroups();
}

void NodeTabsManager::onNodeRemoved(std::shared_ptr<mobsya::ThymioNode> thymio) {
    auto it = m_tabs.find(thymio->uuid());
    if(it != m_tabs.end()) {
        (*it)->setThymio({});
        auto idx = QTabWidget::indexOf(*it);
        QTabWidget::setTabEnabled(idx, false);
    }
}

void NodeTabsManager::onNodeModified(std::shared_ptr<mobsya::ThymioNode> thymio) {
    auto it = m_tabs.find(thymio->uuid());
    if(it != m_tabs.end()) {
        auto idx = QTabWidget::indexOf(*it);
        if(thymio->status() != mobsya::ThymioNode::Status::Disconnected)
            tabBar()->setTabText(idx, thymio->name());
    }
    addThymiosFromGroups();
}


void NodeTabsManager::addThymiosFromGroups() {
    for(auto&& node : nodes_for_groups()) {
        addTab(node->uuid());
    }
}


void NodeTabsManager::highlightTab(int index, QColor color) {
    tabBar()->setTabTextColor(index, color);
}

void NodeTabsManager::setExecutionMode(int index, Target::ExecutionMode state) {
    if(state == Target::EXECUTION_UNKNOWN) {
        setTabIcon(index, QIcon());
    } else if(state == Target::EXECUTION_RUN) {
        setTabIcon(index, QIcon(":/images/play.png"));
    } else if(state == Target::EXECUTION_STEP_BY_STEP) {
        setTabIcon(index, QIcon(":/images/pause.png"));
    } else if(state == Target::EXECUTION_STOP) {
        setTabIcon(index, QIcon(":/images/stop.png"));
    }
}

void NodeTabsManager::tabChanged(int index) {
    // resize the vmMemoryView, to match the user choice
    auto* tab = dynamic_cast<NodeTab*>(currentWidget());
    if(!tab)
        return;

    // reset the tab highlight
    resetHighlight(index);
}

void NodeTabsManager::tabInserted(int index) {
    Q_EMIT tabAdded(index);
}

void NodeTabsManager::onTabClosed(int index) {
    auto t = qobject_cast<PlotTab*>(QTabWidget::widget(index));
    if(!t)
        return;
    QTabWidget::removeTab(index);
    QTabWidget::setCurrentIndex(index - 1);
}

NodeTab* NodeTabsManager::tabForNode(std::shared_ptr<const mobsya::ThymioNode> n) const {
    auto v = devicesTabs() | ranges::view::filter([&](auto&& tab) { return n == tab->thymio(); });
    for(auto&& e : v) {
        return e;
    }
    return nullptr;
}

void NodeTabsManager::resetHighlight(int index) {
    tabBar()->setTabTextColor(index, Qt::black);
}

}  // namespace Aseba