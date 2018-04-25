#pragma once
#include <QAbstractListModel>

namespace mobsya {
class ThymioManager;

class ThymioListModel : public QAbstractListModel {
    Q_OBJECT

    enum class Role {
        NodeId = Qt::UserRole + 1,
        RobotName,
    };

public:
    ThymioListModel(const ThymioManager* const, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QVariantMap get(int index) const;

private Q_SLOTS:
    void onRobotAdded();

private:
    const ThymioManager* const m_manager;
};

}  // namespace mobsya
