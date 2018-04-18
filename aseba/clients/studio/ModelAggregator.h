#ifndef MODELAGGREGATOR_H
#define MODELAGGREGATOR_H

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QModelIndex>
#include <QList>

namespace Aseba {
class ModelAggregator : public QAbstractItemModel {
public:
    ModelAggregator(QObject* parent = nullptr);

    // interface for the aggregator
    void addModel(QAbstractItemModel* model, unsigned int column = 0);

    // interface for QAbstractItemModel
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool hasIndex(int row, int column, const QModelIndex& parent) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;

protected:
    struct ModelDescription {
        QAbstractItemModel* model;
        unsigned int column;
    };

    typedef QList<ModelDescription> ModelList;
    ModelList models;
};


// keep only the leaves of a tree, and present them as a table
class TreeChainsawFilter : public QAbstractProxyModel {
    Q_OBJECT

public:
    TreeChainsawFilter(QObject* parent = nullptr) : QAbstractProxyModel(parent) {}

    // model interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool hasIndex(int row, int column, const QModelIndex& parent) const;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;

public slots:
    void resetInternalData();

public:
    // proxy interface
    void setSourceModel(QAbstractItemModel* sourceModel) override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

    void sort(int column, Qt::SortOrder order) override;

protected:
    struct ModelIndexLink {
        QModelIndex source;
        QModelIndex proxy;
    };

    void sortWalkTree(const QModelIndex& parent);

    typedef QList<ModelIndexLink> IndexLinkList;
    IndexLinkList indexList;
};
};  // namespace Aseba

#endif  // MODELAGGREGATOR_H
