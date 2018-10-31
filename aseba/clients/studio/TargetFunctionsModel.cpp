#include "TargetFunctionsModel.h"
#include <QMimeData>

namespace Aseba {

struct TargetFunctionsModel::TreeItem {
    TreeItem* parent;
    QVector<TreeItem*> children;
    QString name;
    std::optional<mobsya::AsebaVMFunctionDescription> desc;

    bool enabled;
    bool draggable;

    TreeItem() : parent(nullptr), enabled(true), draggable(false) {}

    TreeItem(TreeItem* parent, mobsya::AsebaVMFunctionDescription desc, bool enabled, bool draggable)
        : parent(parent), name(desc.name()), desc(std::move(desc)), enabled(enabled), draggable(draggable) {}

    TreeItem(TreeItem* parent, QString name, bool enabled, bool draggable = false)
        : parent(parent), name(name), enabled(enabled), draggable(draggable) {}

    ~TreeItem() {
        qDeleteAll(children);
    }

    TreeItem* getEntry(const QString& name, bool enabled = true) {
        auto insertion_point = std::lower_bound(children.begin(), children.end(), name,
                                                [](TreeItem* item, const QString& name) { return item->name < name; });
        if(insertion_point != std::end(children) && (*insertion_point)->name == name)
            return (*insertion_point);
        return *children.insert(insertion_point, new TreeItem(this, name, enabled, draggable));
    }


    QString getToolTip() const {
        QString text;
        QSet<QString> variablesNames;
        if(!desc)
            return name;

        text += QStringLiteral("<b>%1</b>(").arg(desc->name());
        auto parameters = desc->parameters();
        for(auto i = 0; i < parameters.size(); i++) {
            const auto& p = parameters[i];

            variablesNames.insert(p.name());
            text += p.name();
            if(p.size() > 1)
                text += QStringLiteral("[%1]").arg(p.size());
            if(i + 1 < parameters.size())
                text += QStringLiteral(", ");
        }

        QString description = desc->description();
        QStringList descriptionWords = description.split(QRegExp("\\b"));
        for(int i = 0; i < descriptionWords.size(); ++i)
            if(variablesNames.contains(descriptionWords.at(i)))
                descriptionWords[i] = QStringLiteral("<tt>%1</tt>").arg(descriptionWords[i]);

        text += QStringLiteral(")<br/>") + descriptionWords.join(QStringLiteral(" "));
        return text;
    }
};


TargetFunctionsModel::TargetFunctionsModel(QObject* parent) : QAbstractItemModel(parent), root(nullptr) {}

TargetFunctionsModel::~TargetFunctionsModel() {}

Qt::DropActions TargetFunctionsModel::supportedDropActions() const {
    return Qt::CopyAction;
}

TargetFunctionsModel::TreeItem* TargetFunctionsModel::getItem(const QModelIndex& index) const {
    if(index.isValid()) {
        auto* item = static_cast<TreeItem*>(index.internalPointer());
        if(item)
            return item;
    }
    return root.get();
}

int TargetFunctionsModel::rowCount(const QModelIndex& parent) const {
    auto i = getItem(parent);
    return i ? i->children.count() : 0;
}

int TargetFunctionsModel::columnCount(const QModelIndex& /* parent */) const {
    return 1;
}

void TargetFunctionsModel::recreateTreeFromDescription(const QVector<mobsya::AsebaVMFunctionDescription>& functions) {
    beginResetModel();

    root.reset(new TreeItem);
    std::unique_ptr<TreeItem> hidden_root = std::make_unique<TreeItem>(root.get(), tr("hidden"), false);
    for(const auto& fun : functions) {

        QString name = fun.name();
        QStringList parts = fun.name().split(QStringLiteral("."), QString::SkipEmptyParts);

        if(parts.empty())
            continue;
        auto is_hidden = name[0] == '_' || name.contains(QStringLiteral("._"));
        TreeItem* entry = is_hidden ? hidden_root.get() : root.get();
        for(int j = 0; j < parts.size() - 1; ++j)
            entry = entry->getEntry(parts[j], entry->enabled);
        entry->children.push_back(new TreeItem(entry, fun, entry->enabled, true));
    }

    if(!hidden_root->children.empty())
        // Put the hidden item at the end
        root->children.push_back(hidden_root.release());

    endResetModel();
}

QModelIndex TargetFunctionsModel::parent(const QModelIndex& index) const {
    if(!index.isValid())
        return QModelIndex();

    TreeItem* childItem = getItem(index);
    TreeItem* parentItem = childItem->parent;

    if(parentItem == root.get())
        return QModelIndex();

    if(parentItem->parent)
        return createIndex(parentItem->parent->children.indexOf(const_cast<TreeItem*>(parentItem)), 0, parentItem);
    else
        return createIndex(0, 0, parentItem);
}

QModelIndex TargetFunctionsModel::index(int row, int column, const QModelIndex& parent) const {
    TreeItem* parentItem = getItem(parent);
    if(!parentItem)
        return {};
    TreeItem* childItem = parentItem->children.value(row);
    Q_ASSERT(childItem);

    if(childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QVariant TargetFunctionsModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::WhatsThisRole))
        return QVariant();

    if(role == Qt::DisplayRole) {
        return getItem(index)->name;
    } else {
        return getItem(index)->getToolTip();
    }
}

QVariant TargetFunctionsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

Qt::ItemFlags TargetFunctionsModel::flags(const QModelIndex& index) const {
    auto* item = static_cast<TreeItem*>(index.internalPointer());
    if(item) {
        QFlags<Qt::ItemFlag> flags;
        flags |= item->enabled ? Qt::ItemIsEnabled : QFlags<Qt::ItemFlag>();
        flags |= item->draggable ? Qt::ItemIsDragEnabled | Qt::ItemIsSelectable : QFlags<Qt::ItemFlag>();
        return flags;
    } else
        return Qt::ItemIsEnabled;
}

QStringList TargetFunctionsModel::mimeTypes() const {
    QStringList types;
    types << QStringLiteral("text/plain");
    return types;
}

QMimeData* TargetFunctionsModel::mimeData(const QModelIndexList& indexes) const {
    QString texts;
    foreach(QModelIndex index, indexes) {
        auto item = getItem(index);
        if(!item || !item->desc)
            continue;
        const auto desc = *item->desc;
        QStringList params;
        for(const auto& p : desc.parameters()) {
            params.push_back(p.name());
        }
        texts += QStringLiteral("%1(%2)").arg(desc.name(), params.join(", "));
    }

    auto* mimeData = new QMimeData();
    mimeData->setText(texts);
    return mimeData;
}

}  // namespace Aseba