#pragma once
#include <QtWidgets>
#include "Target.h"
#include "NodeTab.h"
#include "PlotTab.h"
#include <aseba/qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>
#ifndef Q_MOC_RUN
#    include <range/v3/view/iota.hpp>
#    include <range/v3/view/transform.hpp>
#    include <range/v3/view/filter.hpp>
#    include <range/v3/view/join.hpp>
#    include <range/v3/action/sort.hpp>
#    include <range/v3/action/unique.hpp>
#endif


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

    auto groups() const {
        auto grps = ranges::view::all(m_tabs) | ranges::view::filter([](NodeTab* n) { return n && n->thymio(); }) |
            ranges::view::transform([](NodeTab* n) { return n->thymio()->group(); }) | ranges::to_vector;
        return grps | ranges::move | ranges::action::sort | ranges::action::unique;
    }

public Q_SLOTS:
    void onNodeAdded(std::shared_ptr<mobsya::ThymioNode>);
    void onNodeRemoved(std::shared_ptr<mobsya::ThymioNode>);
    void onNodeModified(std::shared_ptr<mobsya::ThymioNode>);

protected Q_SLOTS:
    void tabChanged(int index);
    void onTabClosed(int index);
    void onScratchpadChanged(const mobsya::Scratchpad& s);

Q_SIGNALS:
    void tabAdded(int index);
    void nodeStatusChanged();


protected:
    enum class plot_type { event, variable };

    void createPlotTab(NodeTab* tab, const QString& name, plot_type type);
    void createPlotTab(std::shared_ptr<mobsya::ThymioNode>, const QString& name, plot_type type);
    void resetHighlight(int index);
    void tabInserted(int index) override;

private:
    NodeTab* createTab();
    void setupButtons(NodeTab* tab);
    void deleteTab(NodeTab*);
    void addThymiosFromGroups();
    NodeTab* tabForNode(std::shared_ptr<const mobsya::ThymioNode>) const;


    auto nodes_for_groups() const {
        std::vector<std::shared_ptr<mobsya::ThymioNode>> nodes;
        for(auto&& g : groups()) {
            for(auto&& n : g->nodes())
                nodes.push_back(n);
        }
        return nodes;
    }

    const mobsya::ThymioDeviceManagerClient& m_client;
    mobsya::detail::UnsignedQMap<QUuid, NodeTab*> m_tabs;
    mobsya::detail::UnsignedQMap<QUuid, NodeTab*> m_scratchpads;
};


}  // namespace Aseba