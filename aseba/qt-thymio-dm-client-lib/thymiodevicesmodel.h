#pragma once
#include <QAbstractListModel>

namespace mobsya {

class ThymioDeviceManagerClient;

class ThymioDevicesModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { StatusRole = Qt::UserRole + 1, NodeIdRole };

    ThymioDevicesModel(const ThymioDeviceManagerClient& manager, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

private Q_SLOTS:
    void updateModel();

private:
    const ThymioDeviceManagerClient& m_manager;
};

}  // namespace mobsya
