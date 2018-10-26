#include "NodeTabsManager.h"
#include "NodeTab.h"

namespace Aseba {

//////

NodeTabsManager::NodeTabsManager(const mobsya::ThymioDeviceManagerClient& client) : m_client(client) {
    connect(this, &QTabWidget::currentChanged, this, &NodeTabsManager::tabChanged);
    connect(this, &QTabWidget::tabCloseRequested, this, &NodeTabsManager::onTabClosed);

    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeAdded, this, &NodeTabsManager::onNodeAdded);
    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeModified, this, &NodeTabsManager::onNodeModified);
    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeRemoved, this, &NodeTabsManager::onNodeRemoved);
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
    } else {
        QTabWidget::setTabEnabled(index, false);
    }
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
    } else {
        addTab(thymio->uuid());
    }
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

void NodeTabsManager::tabInserted(int) {
    QTabWidget::setTabsClosable(count() > 1);
}
void NodeTabsManager::tabRemoved(int) {
    QTabWidget::setTabsClosable(count() > 1);
}

void NodeTabsManager::onTabClosed(int index) {
    auto t = qobject_cast<NodeTab*>(QTabWidget::widget(index));
    if(!t)
        return;
    for(auto it = m_tabs.begin(); it != m_tabs.end(); ++it) {
        if(it.value() == t) {
            QTabWidget::removeTab(index);
            m_tabs.erase(it);
            return;
        }
    }
}

void NodeTabsManager::resetHighlight(int index) {
    tabBar()->setTabTextColor(index, Qt::black);
}

}  // namespace Aseba