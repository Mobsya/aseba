#include "VariablesModel.h"
#include <QMimeData>
#include <algorithm>
#include <QDataStream>


namespace Aseba {
struct VariablesModel::TreeItem {
    QVariant key;
    QVariant value;
    bool constant = false;
    bool modified = false;
    TreeItem* parent = nullptr;
    std::vector<std::unique_ptr<TreeItem>> children;


    bool has_aseba_integer_value() const {
        if(children.size() != 1)
            return false;
        if(value.type() != QVariant::List)
            return false;
        auto l = value.toList();
        if(l.size() == 1) {
            const auto& item = l[0];
            if(item.type() == QVariant::UInt || item.type() == QVariant::Int || item.type() == QVariant::ULongLong ||
               item.type() == QVariant::LongLong)
                return true;
        }
        return false;
    }
};

namespace {
    VariablesModel::TreeItem* child_by_name(const VariablesModel::TreeItem& item, const QVariant& key) {
        auto it = std::lower_bound(item.children.begin(), item.children.end(), key,
                                   [](const auto& ptr, const QVariant& key) { return ptr->key < key; });
        return it != item.children.end() && (*it)->key == key ? it->get() : nullptr;
    }

    auto remove_child(VariablesModel::TreeItem& item, const QVariant& key) {
        auto it = std::find_if(item.children.begin(), item.children.end(),
                               [&key](const auto& ptr) { return ptr->key == key; });

        if(it != item.children.end()) {
            item.children.erase(it);
        }
    }

    VariablesModel::TreeItem* find_or_create_child(VariablesModel::TreeItem& item, const QVariant& key) {
        auto it = std::lower_bound(item.children.begin(), item.children.end(), key,
                                   [&key](const auto& ptr, const QVariant& key) { return ptr->key < key; });
        if(it != item.children.end() && (*it)->key == key)
            return it->get();

        auto newItem = std::make_unique<VariablesModel::TreeItem>();
        newItem->key = key;
        newItem->parent = &item;
        return item.children.emplace(it, std::move(newItem))->get();
    }
}  // namespace

VariablesModel::VariablesModel(QObject* parent) : QAbstractItemModel(parent) {}

VariablesModel::~VariablesModel() {}

int VariablesModel::rowCount(const QModelIndex& index) const {
    auto item = getItem(index);
    if(!item)
        return 0;
    if(item->has_aseba_integer_value())
        return 0;
    return item->children.size();
}

int VariablesModel::columnCount(const QModelIndex&) const {
    return 2;
}

namespace {
    QVariant pretty_print_variable(const QVariant& v) {
        if(v.type() == QVariant::List) {
            auto l = v.toList();
            if(l.size() == 1) {
                return l[0];
            }
            if(l.size() < 10 && std::all_of(l.begin(), l.end(), [](const QVariant& v) {
                   return v.type() == QVariant::Int || v.type() == QVariant::UInt || v.type() == QVariant::LongLong ||
                       v.type() == QVariant::ULongLong;
               })) {
                QStringList values;
                std::transform(l.begin(), l.end(), std::back_inserter(values),
                               [](const QVariant& v) { return QString::number(v.toLongLong()); });
                QString s = "[" + values.join(", ") + "]";
                if(s.size() < 30)
                    return s;
            }
        }
        if(v.type() == QVariant::List || v.type() == QVariant::StringList) {
            return QObject::tr("list <%1 elements>").arg(v.toList().size());
        }
        if(v.type() == QVariant::Map) {
            return QObject::tr("map <%1 elements>").arg(v.toMap().size());
        }
        return v;
    }
}  // namespace

QVariant VariablesModel::data(const QModelIndex& index, int role) const {
    auto item = getItem(index);
    if(!item)
        return {};
    if(index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        const auto& k = item->key;
        if(k.type() == QVariant::UInt || k.type() == QVariant::Int)
            return QStringLiteral("[%1]").arg(k.toUInt());
        return item->key;
    }
    if(index.column() == 1 && role == Qt::DisplayRole) {
        return pretty_print_variable(item->value);
    }
    if(index.column() == 1 && role == Qt::EditRole) {
        return item->value;
    }

    return QVariant();
}

QVariant VariablesModel::headerData(int, Qt::Orientation, int) const {
    return QVariant();
}

Qt::ItemFlags VariablesModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    if(!index.isValid())
        return flags;

    auto item = getItem(index);

    if(index.column() == 0 && item->constant) {
        flags |= Qt::ItemIsEditable;
    }
    if(index.column() == 1 && (item->constant || item->children.empty() || item->has_aseba_integer_value())) {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

bool VariablesModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    auto n = index.row();
    auto item = getItem(index);
    if(!item || role != Qt::EditRole)
        return {};
    if(!item) {
        return false;
    }
    if(item->parent && item->parent != m_root.get()) {
        auto v = item->parent->value;
        if(v.type() == QVariant::List || v.type() == QVariant::List) {
            auto l = v.toList();
            if(n < 0 && n >= l.size())
                return false;
            l[n] = value;
            return setData(index.parent(), l, role);
        }
        if(v.type() == QVariant::Map) {
            auto m = v.toMap();
            const auto k = item->key.toString();
            if(!m.contains(k))
                return false;
            m[k] = value;
            return setData(index.parent(), m, role);
        }
        Q_ASSERT(false);
    }
    item->value = value;
    Q_EMIT dataChanged(index, index);
    Q_EMIT variableChanged(item->key.toString(), mobsya::ThymioVariable(value, item->constant));
    return true;
}

void VariablesModel::setVariables(const mobsya::ThymioNode::VariableMap& vars) {
    for(const auto& v : vars.toStdMap()) {
        auto root = get_or_create_root();
        setVariable(*root, v.first, v.second.value(), v.second.isConstant(), {});
    }
}


void VariablesModel::setVariable(const QString& name, const mobsya::ThymioVariable& v) {
    auto root = get_or_create_root();
    setVariable(*root, name, v.value(), v.isConstant(), {});
}

void VariablesModel::setVariable(TreeItem& item, const QVariant& key, const QVariant& v, bool constant,
                                 const QModelIndex& parent) {
    auto node = child_by_name(item, key);
    bool created = !node;
    auto index = getIndex(key, parent, 0);

    if(created) {
        beginInsertRows(parent, index.row(), index.row());
        beginInsertColumns(parent, 0, 2);
    }

    if(!node)
        node = find_or_create_child(item, key);

    node->constant = constant;
    if(!created && node->value == v) {
        return;
    }
    node->value = v;

    if(created) {
        endInsertColumns();
        endInsertRows();
    }

    if(v.type() == QVariant::List || v.type() == QVariant::StringList) {
        int idx = 0;
        for(const auto& e : v.toList()) {
            setVariable(*node, QVariant::fromValue(idx), e, constant, index);
            idx++;
        }
        node->children.resize(idx);
    } else if(v.type() == QVariant::Map) {
        QVariantMap m = v.toMap();
        auto keys = m.keys();
        auto& children = node->children;
        children.erase(std::remove_if(std::begin(children), std::end(children),
                                      [&keys](const auto& item) { return keys.contains(item->key.toString()); }),
                       std::end(children));
        for(const auto& k : keys) {
            setVariable(*node, k, m[k], constant, index);
        }
    }

    auto end = getIndex(key, parent, 1);
    dataChanged(index, end);
}

void VariablesModel::removeVariable(const QString& name) {
    auto item = m_root.get();
    if(!item)
        return;
    remove_child(*item, name);
}

void VariablesModel::clear() {
    beginResetModel();
    m_root.reset();
    endResetModel();
}

auto VariablesModel::getItem(const QModelIndex& idx) const -> TreeItem* {
    if(idx.isValid()) {
        auto* item = static_cast<TreeItem*>(idx.internalPointer());
        return item;
    }
    return m_root.get();
}


QModelIndex VariablesModel::parent(const QModelIndex& index) const {
    if(!index.isValid())
        return QModelIndex();

    TreeItem* item = getItem(index);
    if(!item)
        return {};
    item = item->parent;

    if(item == m_root.get())
        return QModelIndex();

    if(item->parent) {
        const auto& siblings = item->parent->children;
        auto it = std::find_if(std::begin(siblings), std::end(siblings),
                               [item](const auto& ptr) { return ptr.get() == item; });
        auto distance = std::distance(std::begin(siblings), it);
        return createIndex(distance, index.column(), item);
    } else
        return {};
}
QModelIndex VariablesModel::index(int row, int column, const QModelIndex& parent) const {

    auto parentItem = getItem(parent);
    if(!parentItem)
        return {};
    auto item = row >= 0 && row < parentItem->children.size() ? parentItem->children[row].get() : nullptr;
    return createIndex(row, column, item);
}

QModelIndex VariablesModel::getIndex(const QVariant& key, const QModelIndex& parent, int col) {
    auto item = getItem(parent);
    if(!item) {
        return {};
    }
    auto it = std::lower_bound(item->children.begin(), item->children.end(), key,
                               [](const auto& ptr, const QVariant& key) { return ptr->key < key; });
    return index(std::distance(item->children.begin(), it), col, parent);
}

VariablesModel::TreeItem* VariablesModel::get_or_create_root() {
    if(!m_root)
        m_root = std::make_unique<TreeItem>();
    return m_root.get();
}

QStringList VariablesModel::mimeTypes() const {
    QStringList types;
    types << QStringLiteral("text/plain");
    if(privateMimeType != QLatin1String(""))
        types << privateMimeType;
    return types;
}

QMimeData* VariablesModel::mimeData(const QModelIndexList& indexes) const {
    auto* mimeData = new QMimeData();

    // "text/plain"
    QString texts;
    foreach(QModelIndex index, indexes) {
        if(index.isValid() && (index.column() == 0)) {
            QString text = data(index, Qt::DisplayRole).toString();
            texts += text;
        }
    }
    mimeData->setText(texts);

    if(privateMimeType == QLatin1String(""))
        return mimeData;

    // privateMimeType
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    foreach(QModelIndex itemIndex, indexes) {
        if(!itemIndex.isValid())
            continue;
        QString name = data(index(itemIndex.row(), 0, itemIndex.parent()), Qt::DisplayRole).toString();
        int nbArgs = data(index(itemIndex.row(), 1, itemIndex.parent()), Qt::DisplayRole).toInt();
        dataStream << name << nbArgs;
    }
    mimeData->setData(privateMimeType, itemData);

    return mimeData;
}
}  // namespace Aseba