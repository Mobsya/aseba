#include "ThymioListModel.h"
#include "ThymioManager.h"
#include "NetworkDeviceProber.h"

namespace mobsya {

namespace {
    QString label(const ThymioManager::Robot& robot) {
        QString name = robot->name();
        if(robot->provider().type() == ThymioProviderInfo::ProviderType::Tcp) {
            auto service = dns_service_for_provider(robot->provider());
            QString host = service.host();
            if(!host.isEmpty() && !service.domain().isEmpty()) {
                host += "." + service.domain();
            }
            if(!host.isEmpty()) {
                name = QObject::tr("%1 on %2").arg(name, host);
            }
        }
        return name;
    }

}    // namespace

ThymioListModel::ThymioListModel(const ThymioManager* const manager, QObject* parent)
    : QAbstractListModel(parent)
    , m_manager(manager) {

    connect(m_manager, &ThymioManager::robotAdded, this, &ThymioListModel::onRobotAdded);
    connect(m_manager, &ThymioManager::robotRemoved, this, &ThymioListModel::onRobotAdded);
}

void ThymioListModel::onRobotAdded() {
    beginResetModel();
    endResetModel();
}

int ThymioListModel::rowCount(const QModelIndex&) const {
    return int(m_manager->robotsCount());
}

QVariant ThymioListModel::data(const QModelIndex& index, int role) const {
    int row = index.row();
    if(row < 0 || row >= rowCount())
        return {};
    const auto& robot = m_manager->at(size_t(row));
    if(!robot)
        return {};
    switch(role) {
        case Qt::DisplayRole: return label(robot);
        case int(Role::NodeId): return robot->id();
        case int(Role::RobotName): return robot->name();
    }
    return {};
}

QHash<int, QByteArray> ThymioListModel::roleNames() const {
    return QHash<int, QByteArray>{
        {int(Role::NodeId), "nodeId"},
        {int(Role::RobotName), "name"},
    };
}

Q_INVOKABLE QVariantMap ThymioListModel::get(int index) const {
    return QVariantMap{
        {"nodeId", data(this->index(index), int(Role::NodeId))},
        {"name", data(this->index(index), int(Role::RobotName))},
    };
}


}    // namespace mobsya
