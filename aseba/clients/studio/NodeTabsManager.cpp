#include "NodeTabsManager.h"
#include "NodeTab.h"

namespace Aseba {

//////

NodeTabsManager::NodeTabsManager(const mobsya::ThymioDeviceManagerClient& client) : m_client(client) {
    readSettings();  // read user's preferences
    connect(this, &QTabWidget::currentChanged, this, &NodeTabsManager::tabChanged);

    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeAdded, this, &NodeTabsManager::onNodeAdded);
    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeModified, this, &NodeTabsManager::onNodeModified);
    connect(&m_client, &mobsya::ThymioDeviceManagerClient::nodeRemoved, this, &NodeTabsManager::onNodeRemoved);
}

NodeTabsManager::~NodeTabsManager() {
    // store user's preferences
    writeSettings();
}

void NodeTabsManager::addTab(const QUuid& device) {
    if(m_tabs.contains(device))
        return;
    auto tab = new NodeTab();
    const int index = QTabWidget::addTab(tab, device.toString());
    QPushButton* button = new QPushButton(QIcon(":/images/remove.png"), QLatin1String(""));
    button->setFlat(true);
    connect(button, &QAbstractButton::clicked, this, &NodeTabsManager::removeAndDeleteTab);
    tabBar()->setTabButton(index, QTabBar::RightSide, button);
    m_tabs[device] = tab;
    if(auto thymio = m_client.node(device)) {
        tab->setThymio(thymio);
    } else {
        QTabWidget::setTabEnabled(index, false);
    }
}


void NodeTabsManager::onNodeAdded(std::shared_ptr<mobsya::ThymioNode> thymio) {
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

void NodeTabsManager::removeAndDeleteTab(int index) {
    if(index < 0) {
        auto* button(qobject_cast<QWidget*>(sender()));
        for(int i = 0; i < count(); ++i) {
            if(tabBar()->tabButton(i, QTabBar::RightSide) == button) {
                index = i;
                break;
            }
        }
    }
    if(index >= 0) {
        QWidget* w(widget(index));
        QTabWidget::removeTab(index);
        w->deleteLater();
    }
}

void NodeTabsManager::vmMemoryResized(int col, int oldSize, int newSize) {
    // keep track of the current value, to apply it on the other tabs
    vmMemorySize[col] = newSize;
}

void NodeTabsManager::tabChanged(int index) {
    // resize the vmMemoryView, to match the user choice
    auto* tab = dynamic_cast<NodeTab*>(currentWidget());
    if(!tab)
        return;

    vmMemoryViewResize(tab);
    // reset the tab highlight
    resetHighlight(index);
}

void NodeTabsManager::resetHighlight(int index) {
    tabBar()->setTabTextColor(index, Qt::black);
}

void NodeTabsManager::vmMemoryViewResize(NodeTab* tab) {
    if(!tab)
        return;

    // resize the vmMemoryView QTreeView, according to the global value
    for(int i = 0; i < 2; i++)
        if(vmMemorySize[i] != -1)
            tab->vmMemoryView->header()->resizeSection(i, vmMemorySize[i]);
}

void NodeTabsManager::readSettings() {
    QSettings settings;
    vmMemorySize[0] = settings.value(QStringLiteral("vmMemoryView/col0"), -1).toInt();
    vmMemorySize[1] = settings.value(QStringLiteral("vmMemoryView/col1"), -1).toInt();
}

void NodeTabsManager::writeSettings() {
    QSettings settings;
    settings.setValue(QStringLiteral("vmMemoryView/col0"), vmMemorySize[0]);
    settings.setValue(QStringLiteral("vmMemoryView/col1"), vmMemorySize[1]);
}

}  // namespace Aseba