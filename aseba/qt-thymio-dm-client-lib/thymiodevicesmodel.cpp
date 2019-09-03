#include <utility>
#include "thymiodevicesmodel.h"
#include "thymiodevicemanagerclient.h"
#include "thymionode.h"

namespace mobsya {

ThymioDevicesModel::ThymioDevicesModel(const ThymioDeviceManagerClient& manager, QObject* parent)
    : QAbstractListModel(parent), m_manager(manager) {

    connect(&manager, &ThymioDeviceManagerClient::nodeAdded, this, &ThymioDevicesModel::updateModel);
    connect(&manager, &ThymioDeviceManagerClient::nodeRemoved, this, &ThymioDevicesModel::updateModel);
}
int ThymioDevicesModel::rowCount(const QModelIndex&) const {
    return int(m_manager.m_nodes.size());
}

QVariant ThymioDevicesModel::data(const QModelIndex& index, int role) const {
    size_t idx = index.row();
    if(idx >= m_manager.m_nodes.size())
        return {};
    auto item = (m_manager.m_nodes.begin() + int(idx)).value();
    switch(role) {
        case Qt::DisplayRole: return item->name();
        case StatusRole: return int(item->status());
        case NodeIdRole: return item->uuid().toString();
        case NodeTypeRole: return int(item->type());
        case NodeCapabilitiesRole: return quint64(item->capabilities());
        case InGroupRole: return item->isInGroup();
        case Object: return QVariant::fromValue(item.get());
    }
    return {};
}

QHash<int, QByteArray> ThymioDevicesModel::roleNames() const {
    auto roles = QAbstractListModel::roleNames();
    roles.insert(StatusRole, "status");
    roles.insert(Qt::DisplayRole, "name");
    roles.insert(NodeIdRole, "id");
    roles.insert(NodeTypeRole, "type");
    roles.insert(Object, "object");
    roles.insert(NodeCapabilitiesRole, "capabilities");
    roles.insert(InGroupRole, "isInGroup");
    return roles;
}

bool ThymioDevicesModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    size_t idx = index.row();
    if(idx >= m_manager.m_nodes.size())
        return false;
    auto item = (m_manager.m_nodes.begin() + int(idx)).value();
    switch(role) {
        case Qt::DisplayRole:
            if(!value.canConvert<QString>())
                return false;
            item->rename(value.toString());
            dataChanged(index, index);
            return true;
    }
    return false;
}

Qt::ItemFlags ThymioDevicesModel::flags(const QModelIndex& index) const {
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void ThymioDevicesModel::onNodeModified(std::shared_ptr<ThymioNode> node) {
    return onRawNodeModified(node.get());
}

void ThymioDevicesModel::onRawNodeModified(ThymioNode* n) {
    if(!n)
        return;
    auto idx = index(int(std::distance(m_manager.m_nodes.begin(),
                                       std::find_if(m_manager.m_nodes.begin(), m_manager.m_nodes.end(),
                                                    [n](const auto& ptr) { return ptr.get() == n; }))));
    Q_EMIT dataChanged(idx, idx);
}

void ThymioDevicesModel::onRawNodeSenderModified() {
    onRawNodeModified(qobject_cast<ThymioNode*>(sender()));
}

void ThymioDevicesModel::updateModel() {
    layoutAboutToBeChanged();
    for(auto&& node : m_manager.m_nodes) {
        connect(node.get(), &ThymioNode::modified, this, &ThymioDevicesModel::onRawNodeSenderModified);
    }
    layoutChanged();
}

}  // namespace mobsya
