#pragma once
#include <QAbstractItemModel>
#include "request.h"

namespace Aseba {

class TargetFunctionsModel : public QAbstractItemModel {
    Q_OBJECT

public:
    struct TreeItem;

public:
    TargetFunctionsModel(QObject* parent = nullptr);
    ~TargetFunctionsModel() override;

    Qt::DropActions supportedDropActions() const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

public Q_SLOTS:
    void recreateTreeFromDescription(const QVector<mobsya::AsebaVMFunctionDescription>& functions);

private:
    friend class AeslEditor;
    friend class StudioAeslEditor;
    TreeItem* getItem(const QModelIndex& index) const;
    QString getToolTip(const mobsya::AsebaVMFunctionDescription& function) const;
    std::unique_ptr<TreeItem> root;  //!< root of functions description tree
};

}  // namespace Aseba