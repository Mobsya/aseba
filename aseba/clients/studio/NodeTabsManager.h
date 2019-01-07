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

protected slots:
    void tabChanged(int index);
    void onTabClosed(int index);

Q_SIGNALS:
    void tabAdded(int index);
    void nodeStatusChanged();

protected:
    void resetHighlight(int index);
    void tabInserted(int index) override;
    void tabRemoved(int index) override;

private:
    const mobsya::ThymioDeviceManagerClient& m_client;
    QMap<QUuid, NodeTab*> m_tabs;
};


}  // namespace Aseba