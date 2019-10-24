#include "NodeTabsManager.h"
#include "LockButton.h"
#include <range/v3/algorithm.hpp>

namespace Aseba {

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
    auto thymio = m_client.node(device);
    NodeTab* tab = nullptr;
    if(thymio && thymio->group()) {
        auto scratchpads = thymio->group()->scratchpads();
        auto it =
            ranges::find_if(scratchpads, [&thymio](const mobsya::Scratchpad& s) { return s.nodeId == thymio->uuid(); });
        if(it) {
            tab = m_scratchpads.value(it->id);
            if(tab)
                m_tabs[device] = tab;
        }
    }
    if(!tab) {
        tab = createTab();
        QTabWidget::addTab(tab, device.toString());
        setupButtons(tab);
    }
    m_tabs[device] = tab;
    if(thymio) {
        tab->setThymio(thymio);
        QTabWidget::setTabText(indexOf(tab), thymio->name());
        addThymiosFromGroups();
    }
}

NodeTab* NodeTabsManager::createTab() {
    auto tab = new NodeTab();
    connect(tab, &NodeTab::executionStateChanged, this, &NodeTabsManager::nodeStatusChanged);

    connect(tab, &NodeTab::plotEventRequested, this,
            [this](QString event) { createPlotTab(qobject_cast<NodeTab*>(sender()), event, plot_type::event); });

    connect(tab, &NodeTab::plotVariableRequested, this, [this](QString variable) {
        createPlotTab(qobject_cast<NodeTab*>(sender()), variable, plot_type::variable);
    });
    return tab;
}

void NodeTabsManager::setupButtons(NodeTab* tab) {
    const auto index = indexOf(tab);
    // Remove the close button on node tabs
    tabBar()->setTabButton(index, QTabBar::RightSide, 0);
    tabBar()->setTabButton(index, QTabBar::LeftSide, 0);

    auto lock = new LockButton(this);
    tabBar()->setTabButton(index, QTabBar::RightSide, lock);

    connect(tab, &NodeTab::statusChanged, this, [tab, lock]() {
        const auto thymio = tab->thymio();
        if(!thymio) {
            lock->setUnAvailable();
            return;
        }
        switch(thymio->status()) {
            case mobsya::ThymioNode::Status::Ready: lock->setLocked(); break;
            case mobsya::ThymioNode::Status::Available: lock->setUnlocked(); break;
            default: lock->setUnAvailable(); break;
        }
    });
    connect(lock, &QAbstractButton::clicked, tab, &NodeTab::toggleLock);
}

void NodeTabsManager::createPlotTab(NodeTab* tab, const QString& name, plot_type type) {
    if(!tab)
        return;
    auto thymio = tab->thymio();
    if(!thymio)
        return;
    createPlotTab(thymio, name, type);
}

void NodeTabsManager::createPlotTab(std::shared_ptr<mobsya::ThymioNode> thymio, const QString& name, plot_type type) {
    PlotTab* t = nullptr;
    auto v =
        eventsTabs() | ranges::view::filter([&](PlotTab* t) {
            return t->thymio() == thymio &&
                (type == plot_type::event ? t->plottedEvents().contains(name) : t->plottedVariables().contains(name));
        });
    if(v.begin() == v.end()) {
        t = new PlotTab(this);
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
    }
}

void NodeTabsManager::onNodeModified(std::shared_ptr<mobsya::ThymioNode> thymio) {
    if(thymio->status() == mobsya::ThymioNode::Status::Connected)
        return;
    auto it = m_tabs.find(thymio->uuid());
    if(it != m_tabs.end()) {
        auto idx = QTabWidget::indexOf(*it);
        if(thymio->status() != mobsya::ThymioNode::Status::Disconnected)
            tabBar()->setTabText(idx, thymio->name());
    }
}


void NodeTabsManager::addThymiosFromGroups() {
    for(auto&& node : nodes_for_groups()) {
        if(node->status() == mobsya::ThymioNode::Status::Connected)
            return;
        addTab(node->uuid());
    }

    for(auto&& g : groups()) {
        g->watchScratchpadsChanges(true);
        g->connect(g.get(), &mobsya::ThymioGroup::scratchPadChanged, this, &NodeTabsManager::onScratchpadChanged,
                   Qt::UniqueConnection);
        for(auto&& s : g->scratchpads()) {
            onScratchpadChanged(s);
        }
    }
}

void NodeTabsManager::onScratchpadChanged(const mobsya::Scratchpad& s) {
    auto t = m_scratchpads.value(s.id);
    if(t && s.deleted) {
        const auto it = m_tabs.find(s.nodeId);
        if(it == m_tabs.end() || it.value() != t) {
            deleteTab(t);
        }
    }
    if(s.deleted) {
        return;
    }
    if(!t) {
        t = m_tabs.value(s.nodeId);
    }
    if(!t) {
        auto id = s.nodeId.isNull() ? s.id : s.nodeId;
        t = createTab();
        QTabWidget::addTab(t, id.toString());
        setupButtons(t);
        m_scratchpads[s.id] = t;
    }
    if(!m_tabs.contains(s.nodeId) && !s.name.isEmpty())
        tabBar()->setTabText(QTabWidget::indexOf(t), s.name);
    if(t->thymio())
        return;
    t->onScratchpadChanged(s.code, s.language);
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
    t->deleteLater();
    QTabWidget::setCurrentIndex(index - 1);
}

void NodeTabsManager::deleteTab(NodeTab* tab) {
    for(auto it = m_tabs.begin(); it != m_tabs.end();) {
        if(it.value() == tab)
            it = m_tabs.erase(it);
        else {
            ++it;
        }
    }

    for(auto it = m_scratchpads.begin(); it != m_scratchpads.end();) {
        if(it.value() == tab)
            it = m_scratchpads.erase(it);
        else {
            ++it;
        }
    }
    removeTab(indexOf(tab));
    tab->deleteLater();
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