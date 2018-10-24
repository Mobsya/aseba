#pragma once
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <memory>
#include "thymionode.h"

namespace Aseba {

class VariablesModel : public QAbstractItemModel {
    Q_OBJECT

public:
    VariablesModel(QObject* parent = nullptr);
    ~VariablesModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;

    QStringList mimeTypes() const override;
    void setExtraMimeType(QString mime) {
        privateMimeType = mime;
    }
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    bool checkIfModified() {
        // return wasModified;
    }
    void clearWasModified() {
        // wasModified = false;
    }
    // virtual bool moveRow(int oldRow, int& newRow);

public Q_SLOTS:
    void setVariables(const mobsya::ThymioNode::VariableMap& map);
    void setVariable(const QString& name, const mobsya::ThymioVariable& v);
    void removeVariable(const QString& name);
    void clear();

Q_SIGNALS:
    void variableChanged(const QString& name, const mobsya::ThymioVariable& value);

public:
    struct TreeItem;

private:
    void setVariable(TreeItem&, const QVariant& key, const QVariant& v, bool constant, const QModelIndex& parent);
    TreeItem* get_or_create_root();
    TreeItem* getItem(const QModelIndex& idx) const;
    QModelIndex getIndex(const QVariant& key, const QModelIndex& parent, int col);


    std::unique_ptr<TreeItem> m_root;
    QString privateMimeType;

private:
};

}  // namespace Aseba