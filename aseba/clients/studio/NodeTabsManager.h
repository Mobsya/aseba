#pragma once
#include <QtWidgets>
#include "Target.h"
#include <aseba/qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>

namespace Aseba {

class NodeTab;
class NodeTabsManager : public QTabWidget {
    Q_OBJECT

public:
    NodeTabsManager(const mobsya::ThymioDeviceManagerClient& client);
    ~NodeTabsManager() override;
    void addTab(const QUuid& device);

    void highlightTab(int index, QColor color = Qt::red);
    void setExecutionMode(int index, Target::ExecutionMode state);

public Q_SLOTS:
    void onNodeAdded(std::shared_ptr<mobsya::ThymioNode>);
    void onNodeRemoved(std::shared_ptr<mobsya::ThymioNode>);
    void onNodeModified(std::shared_ptr<mobsya::ThymioNode>);

    void removeAndDeleteTab(int index = -1);

protected slots:
    void vmMemoryResized(int, int, int);  // for vmMemoryView QTreeView child widget
    void tabChanged(int index);

protected:
    virtual void resetHighlight(int index);
    virtual void vmMemoryViewResize(NodeTab* tab);
    virtual void readSettings();
    virtual void writeSettings();

private:
    const mobsya::ThymioDeviceManagerClient& m_client;
    QMap<QUuid, NodeTab*> m_tabs;
    int vmMemorySize[2];
};


}  // namespace Aseba