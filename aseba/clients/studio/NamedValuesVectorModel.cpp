/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "NamedValuesVectorModel.h"
#include <QtDebug>
#include <QtGui>
#include <QMessageBox>
#include <utility>

namespace Aseba {

FlatVariablesModel::FlatVariablesModel(QObject* parent) : QAbstractTableModel(parent) {}

int FlatVariablesModel::rowCount(const QModelIndex&) const {
    return m_values.size();
}

int FlatVariablesModel::columnCount(const QModelIndex&) const {
    return 2;
}

QVariant FlatVariablesModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() >= m_values.size())
        return QVariant();

    auto item = m_values[index.row()];
    if(role == Qt::DisplayRole || role == Qt::EditRole) {
        if(index.column() == 0)
            return item.first;
        if(index.column() == 1)
            return item.second;
    }
    return QVariant();
}

QVariant FlatVariablesModel::headerData(int, Qt::Orientation, int) const {
    return QVariant();
}

Qt::ItemFlags FlatVariablesModel::flags(const QModelIndex& index) const {
    if(!index.isValid())
        return Qt::ItemIsDropEnabled;

    Qt::ItemFlags commonFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    return commonFlags;
}

QStringList FlatVariablesModel::mimeTypes() const {
    QStringList types;
    types << QStringLiteral("text/plain");
    if(privateMimeType != QLatin1String(""))
        types << privateMimeType;
    return types;
}

QMimeData* FlatVariablesModel::mimeData(const QModelIndexList& indexes) const {
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
        QString name = data(index(itemIndex.row(), 0), Qt::DisplayRole).toString();
        int nbArgs = data(index(itemIndex.row(), 1), Qt::DisplayRole).toInt();
        dataStream << name << nbArgs;
    }
    mimeData->setData(privateMimeType, itemData);

    return mimeData;
}

void FlatVariablesModel::addVariable(const QString& name, const QVariant& value) {
    auto it = std::lower_bound(m_values.begin(), m_values.end(), name,
                               [](const auto& v, const QString& n) { return v.first < n; });
    auto dest = std::distance(m_values.begin(), it);
    if(it != m_values.end() && it->first == name) {
        it->second = value;
        Q_EMIT dataChanged(index(dest, 1), index(dest, 1));
        return;
    }

    beginInsertRows({}, dest, dest);
    m_values.insert(it, {name, value});
    endInsertRows();
}

void FlatVariablesModel::removeVariable(const QString& name) {
    auto it = std::lower_bound(m_values.begin(), m_values.end(), name,
                               [](const auto& v, const QString& n) { return v.first < n; });
    if(it == std::end(m_values))
        return;
    if(it->first != name)
        return;
    auto dest = std::distance(m_values.begin(), it);
    beginRemoveRows({}, dest, dest);
    m_values.erase(it);
    endRemoveRows();
}

void FlatVariablesModel::clear() {
    if(m_values.empty())
        return;

    beginResetModel();
    m_values.clear();
    endResetModel();
}


bool ConstantsModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if(index.isValid() && role == Qt::EditRole) {
        if(index.column() == 0) {
            Q_EMIT constantModified(m_values[index.row()].first, {});
            Q_EMIT constantModified(value.toString(), m_values[index.row()].second);
            return true;
        }
        if(index.column() == 1) {
            Q_EMIT constantModified(m_values[index.row()].first, value);
            return true;
        }
    }
    return false;
}

Qt::ItemFlags ConstantsModel::flags(const QModelIndex& index) const {
    return FlatVariablesModel::flags(index) | Qt::ItemIsEditable;
}

QVariant MaskableVariablesModel::data(const QModelIndex& index, int role) const {
    if(index.column() != 2)
        return FlatVariablesModel::data(index, role);
    bool visible = true;
    if(m_events_visibility.size() > size_t(index.row()))
        visible = m_events_visibility[index.row()];


    if(role == Qt::DisplayRole) {
        return QVariant();
    } else if(role == Qt::DecorationRole) {
        return visible ? QPixmap(QStringLiteral(":/images/eye.png")) : QPixmap(QStringLiteral(":/images/eyeclose.png"));
    } else if(role == Qt::ToolTipRole) {
        return visible ? tr("Hide") : tr("View");
    } else
        return QVariant();
}

int MaskableVariablesModel::columnCount(const QModelIndex& parent) const {
    return FlatVariablesModel::columnCount(parent) + 1;
}
bool MaskableVariablesModel::isVisible(const unsigned id) const {
    if(id >= m_events_visibility.size() || m_events_visibility[id])
        return true;
    return false;
}

bool MaskableVariablesModel::isVisible(const QString& key) const {
    int i = 0;
    for(auto&& v : m_values) {
        if(v.first == key) {
            return isVisible(i);
        }
        i++;
    }
    return true;
}

void MaskableVariablesModel::toggle(const QModelIndex& index) {
    if(size_t(index.row()) >= m_events_visibility.size())
        m_events_visibility.resize(index.row() + 1);

    m_events_visibility[index.row()] = !m_events_visibility[index.row()];
    const auto changed = this->index(index.row(), 2);
    Q_EMIT dataChanged(changed, changed);
}

}  // namespace Aseba
