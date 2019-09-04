#pragma once
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <memory>
#include "thymionode.h"

namespace Aseba {

class VariablesModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum Role {
        HiddenRole = Qt::UserRole + 1,
    };

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

public Q_SLOTS:
    void setVariables(const mobsya::ThymioNode::VariableMap& map);
    void setVariable(const QString& name, const mobsya::ThymioVariable& v);
    void removeVariable(const QString& name);
    void clear();

Q_SIGNALS:
    void variableChanged(const QString& name, const mobsya::ThymioVariable& value);

public:
    struct TreeItem;
    QVariantMap getVariables() const;

private:
    void setVariable(TreeItem& parentItem, const QVariant& key, const QVariant& v, const QModelIndex& parent,
                     QVector<QModelIndex>& updated_indexes);
    TreeItem* get_or_create_root();
    std::pair<VariablesModel::TreeItem*, QModelIndex>
    getIndexedItem(const VariablesModel::TreeItem& item, const QVariant& key, QModelIndex parentIndex, int col);
    TreeItem* getItem(const QModelIndex& idx) const;
    QModelIndex getIndex(const QVariant& key, const QModelIndex& parent, int col);
    QModelIndex getIndex(const QVariant& key, const QModelIndex& parent, const TreeItem& parentItem, int col);
    void emit_data_changed(const QVector<QModelIndex>& updated_indexes);


    std::unique_ptr<TreeItem> m_root;
    QString privateMimeType;

private:
};

class VariablesFilterModel : public QSortFilterProxyModel {
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        auto name = sourceModel()->data(index, Qt::DisplayRole).toString();
        auto hidden = sourceModel()->data(index, VariablesModel::HiddenRole).toBool();
        if(sourceParent.isValid())
            return filterAcceptsRow(sourceParent.row(), sourceParent.parent());
        if(!m_show_hidden && hidden)
            return false;
        auto r = filterRegExp();
        return r.isEmpty() || r.exactMatch(name);
    }

public:
    void showHidden(bool show) {
        m_show_hidden = show;
        invalidateFilter();
    }

private:
    bool m_show_hidden = false;
};

}  // namespace Aseba