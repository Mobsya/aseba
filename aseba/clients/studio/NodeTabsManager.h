#pragma once
#include <QtWidgets>
#include "Target.h"
#include "NodeTab.h"
#include "PlotTab.h"
#include <aseba/qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>


namespace Aseba {

class NodeTabsManager : public QTabWidget {
    Q_OBJECT

public:
    NodeTabsManager(const mobsya::ThymioDeviceManagerClient& client);
    ~NodeTabsManager() override;
    void addTab(const QUuid& device);

    void highlightTab(int index, QColor color = Qt::red);
    void setExecutionMode(int index, Target::ExecutionMode state);

    auto devicesTabs() const {
        return ranges::view::ints(0, count()) |
            ranges::view::transform([this](int i) { return qobject_cast<NodeTab*>(this->widget(i)); }) |
            ranges::view::filter([](NodeTab* n) { return n; });
    }

    auto eventsTabs() const {
        return ranges::view::ints(0, count()) |
            ranges::view::transform([this](int i) { return qobject_cast<PlotTab*>(this->widget(i)); }) |
            ranges::view::filter([](PlotTab* n) { return n; });
    }

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
    enum class plot_type { event, variable };

    void createPlotTab(NodeTab* tab, const QString& name, plot_type type);
    void createPlotTab(std::shared_ptr<const mobsya::ThymioNode>, const QString& name, plot_type type);
    void resetHighlight(int index);
    void tabInserted(int index) override;

private:
    void addThymiosFromGroups();
    NodeTab* tabForNode(std::shared_ptr<const mobsya::ThymioNode>) const;


    auto groups() const {
        auto grps = ranges::view::all(m_tabs) | ranges::view::filter([](NodeTab* n) { return n && n->thymio(); }) |
            ranges::view::transform([](NodeTab* n) { return n->thymio()->group_id(); }) | ranges::to_vector;
        return grps | ranges::move | ranges::action::sort | ranges::action::unique;
    }

    auto nodes_for_groups() const {
        auto grps = groups();
        return grps | ranges::view::transform([this](auto&& g) { return m_client.nodes(g); }) | ranges::view::join |
            ranges::to_vector;
    }

    const mobsya::ThymioDeviceManagerClient& m_client;
    mobsya::detail::UnsignedQMap<QUuid, NodeTab*> m_tabs;
};


}  // namespace Aseba