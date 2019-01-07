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

#include "TargetModels.h"
#include <QtDebug>
#include <QtWidgets>
#include <utility>

namespace Aseba {
/** \addtogroup studio */
/*@{*/

VariableListener::VariableListener(TargetVariablesModel* variablesModel) : variablesModel(variablesModel) {}

VariableListener::~VariableListener() {
    if(variablesModel)
        variablesModel->unsubscribeViewPlugin(this);
}

bool VariableListener::subscribeToVariableOfInterest(const QString& name) {
    return variablesModel->subscribeToVariableOfInterest(this, name);
}

void VariableListener::unsubscribeToVariableOfInterest(const QString& name) {
    variablesModel->unsubscribeToVariableOfInterest(this, name);
}

void VariableListener::unsubscribeToVariablesOfInterest() {
    variablesModel->unsubscribeToVariablesOfInterest(this);
}

void VariableListener::invalidateVariableModel() {
    variablesModel = nullptr;
}


TargetVariablesModel::~TargetVariablesModel() {
    for(VariableListenersNameMap::iterator it = variableListenersMap.begin(); it != variableListenersMap.end(); ++it) {
        it.key()->invalidateVariableModel();
    }
}

TargetVariablesModel::TargetVariablesModel(QObject* parent) : QAbstractItemModel(parent) {}

Qt::DropActions TargetVariablesModel::supportedDropActions() const {
    return Qt::CopyAction;
}

int TargetVariablesModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        if(parent.parent().isValid() || (variables.at(parent.row()).value.size() == 1))
            return 0;
        else
            return variables.at(parent.row()).value.size();
    } else
        return variables.size();
}

int TargetVariablesModel::columnCount(const QModelIndex& parent) const {
    return 2;
}

QModelIndex TargetVariablesModel::index(int row, int column, const QModelIndex& parent) const {
    if(parent.isValid())
        return createIndex(row, column, parent.row());
    else {
        // top-level indices shall not point outside the variable array
        if(row < 0 || row >= variables.length())
            return QModelIndex();
        else
            return createIndex(row, column, -1);
    }
}

QModelIndex TargetVariablesModel::parent(const QModelIndex& index) const {
    if(index.isValid() && (index.internalId() != -1))
        return createIndex(index.internalId(), 0, -1);
    else
        return QModelIndex();
}

QVariant TargetVariablesModel::data(const QModelIndex& index, int role) const {
    if(index.parent().isValid()) {
        if(role != Qt::DisplayRole)
            return QVariant();

        if(index.column() == 0)
            return index.row();
        else
            return variables.at(index.parent().row()).value[index.row()];
    } else {
        QString name = variables.at(index.row()).name;
        // hidden variable
        if((name.left(1) == QLatin1String("_")) || name.contains(QStringLiteral("._"))) {
            if(role == Qt::ForegroundRole)
                return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
            else if(role == Qt::FontRole) {
                QFont font;
                font.setItalic(true);
                return font;
            }
        }
        if(index.column() == 0) {
            if(role == Qt::DisplayRole)
                return name;
            return QVariant();
        } else {
            if(role == Qt::DisplayRole) {
                if(variables.at(index.row()).value.size() == 1)
                    return variables.at(index.row()).value[0];
                else
                    return QStringLiteral("(%0)").arg(variables.at(index.row()).value.size());
            } else if(role == Qt::ForegroundRole) {
                if(variables.at(index.row()).value.size() == 1)
                    return QVariant();
                else
                    return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
            } else
                return QVariant();
        }
    }
}

QVariant TargetVariablesModel::headerData(int section, Qt::Orientation orientation, int role) const {
    // Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if(section == 0)
            return tr("names");
        else
            return tr("values");
    }
    return QVariant();
}

Qt::ItemFlags TargetVariablesModel::flags(const QModelIndex& index) const {
    if(!index.isValid())
        return nullptr;

    if(index.column() == 1) {
        if(index.parent().isValid())
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
        else if(variables.at(index.row()).value.size() == 1)
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
        else
            return nullptr;
    } else {
        if(index.parent().isValid())
            return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
    }
}

bool TargetVariablesModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if(index.isValid() && role == Qt::EditRole) {
        if(index.parent().isValid()) {
            int variableValue;
            bool ok;
            variableValue = value.toInt(&ok);
            Q_ASSERT(ok);

            variables[index.parent().row()].value[index.row()] = variableValue;
            emit variableValuesChanged(variables[index.parent().row()].pos + index.row(),
                                       VariablesDataVector(1, variableValue));

            return true;
        } else if(variables.at(index.row()).value.size() == 1) {
            int variableValue;
            bool ok;
            variableValue = value.toInt(&ok);
            Q_ASSERT(ok);

            variables[index.row()].value[0] = variableValue;
            emit variableValuesChanged(variables[index.row()].pos, VariablesDataVector(1, variableValue));

            return true;
        }
    }
    return false;
}

QStringList TargetVariablesModel::mimeTypes() const {
    QStringList types;
    types << QStringLiteral("text/plain");
    return types;
}

QMimeData* TargetVariablesModel::mimeData(const QModelIndexList& indexes) const {
    QString texts;
    foreach(QModelIndex index, indexes) {
        if(index.isValid()) {
            const QString text = data(index, Qt::DisplayRole).toString();
            if(index.parent().isValid()) {
                const QString varName = data(index.parent(), Qt::DisplayRole).toString();
                texts += varName + "[" + text + "]";
            } else
                texts += text;
        }
    }

    auto* mimeData = new QMimeData();
    mimeData->setText(texts);
    return mimeData;
}

unsigned TargetVariablesModel::getVariablePos(const QString& name) const {
    for(int i = 0; i < variables.size(); ++i) {
        const Variable& variable(variables[i]);
        if(variable.name == name)
            return variable.pos;
    }
    return 0;
}

unsigned TargetVariablesModel::getVariableSize(const QString& name) const {
    for(int i = 0; i < variables.size(); ++i) {
        const Variable& variable(variables[i]);
        if(variable.name == name)
            return variable.value.size();
    }
    return 0;
}

VariablesDataVector TargetVariablesModel::getVariableValue(const QString& name) const {
    for(int i = 0; i < variables.size(); ++i) {
        const Variable& variable(variables[i]);
        if(variable.name == name)
            return variable.value;
    }
    return VariablesDataVector();
}

void TargetVariablesModel::updateVariablesStructure(const VariablesMap* variablesMap) {
    // Build a new list of variables
    QList<Variable> newVariables;
    for(auto it = variablesMap->begin(); it != variablesMap->end(); ++it) {
        // create new variable
        Variable var;
        var.name = QString::fromStdWString(it->first);
        var.pos = it->second.first;
        var.value.resize(it->second.second);

        // find its right place in the array
        int i;
        for(i = 0; i < newVariables.size(); ++i) {
            if(var.pos < newVariables[i].pos)
                break;
        }
        newVariables.insert(i, var);
    }

    // compute the difference
    int i(0);
    int count(std::min(variables.length(), newVariables.length()));
    while(i < count && variables[i].name == newVariables[i].name && variables[i].pos == newVariables[i].pos &&
          variables[i].value.size() == newVariables[i].value.size())
        ++i;

    // update starting from the first change point
    // qDebug() << "change from " << i << " to " << variables.length();
    if(i != variables.length()) {
        beginRemoveRows(QModelIndex(), i, variables.length() - 1);
        int removeCount(variables.length() - i);
        for(int j = 0; j < removeCount; ++j)
            variables.removeLast();
        endRemoveRows();
    }

    // qDebug() << "size: " << variables.length();

    if(i != newVariables.length()) {
        beginInsertRows(QModelIndex(), i, newVariables.length() - 1);
        for(int j = i; j < newVariables.length(); ++j)
            variables.append(newVariables[j]);
        endInsertRows();
    }

    /*variables.clear();
    for (Compiler::VariablesMap::const_iterator it = variablesMap->begin(); it !=
    variablesMap->end(); ++it)
    {
        // create new variable
        Variable var;
        var.name = QString::fromStdWString(it->first);
        var.pos = it->second.first;
        var.value.resize(it->second.second);

        // find its right place in the array
        int i;
        for (i = 0; i < variables.size(); ++i)
        {
            if (var.pos < variables[i].pos)
                break;
        }
        variables.insert(i, var);
    }

    reset();*/
}

void TargetVariablesModel::setVariablesData(unsigned start, const VariablesDataVector& data) {
    size_t dataLength = data.size();
    for(int i = 0; i < variables.size(); ++i) {
        Variable& var = variables[i];
        auto varLen = (int)var.value.size();
        int varStart = (int)start - (int)var.pos;
        auto copyLen = (int)dataLength;
        int copyStart = 0;
        // crop data before us
        if(varStart < 0) {
            copyLen += varStart;
            copyStart -= varStart;
            varStart = 0;
        }
        // if nothing to copy, continue
        if(copyLen <= 0)
            continue;
        // crop data after us
        if(varStart + copyLen > varLen) {
            copyLen = varLen - varStart;
        }
        // if nothing to copy, continue
        if(copyLen <= 0)
            continue;

        // copy
        copy(data.begin() + copyStart, data.begin() + copyStart + copyLen, var.value.begin() + varStart);

        // notify gui
        QModelIndex parentIndex = index(i, 0);
        emit dataChanged(index(varStart, 0, parentIndex), index(varStart + copyLen, 0, parentIndex));

        // and notify view plugins
        for(VariableListenersNameMap::iterator it = variableListenersMap.begin(); it != variableListenersMap.end();
            ++it) {
            QStringList& list = it.value();
            for(int v = 0; v < list.size(); v++) {
                if(list[v] == var.name)
                    it.key()->variableValueUpdated(var.name, var.value);
            }
        }
    }
}

bool TargetVariablesModel::setVariableValues(const QString& name, const VariablesDataVector& values) {
    for(int i = 0; i < variables.size(); ++i) {
        Variable& variable(variables[i]);
        if(variable.name == name) {
            // 				setVariablesData(variable.pos, values);
            emit variableValuesChanged(variable.pos, values);
            return true;
        }
    }
    return false;
}

void TargetVariablesModel::unsubscribeViewPlugin(VariableListener* listener) {
    variableListenersMap.remove(listener);
}

bool TargetVariablesModel::subscribeToVariableOfInterest(VariableListener* listener, const QString& name) {
    QStringList& list = variableListenersMap[listener];
    list.push_back(name);
    for(int i = 0; i < variables.size(); i++)
        if(variables[i].name == name)
            return true;
    return false;
}

void TargetVariablesModel::unsubscribeToVariableOfInterest(VariableListener* listener, const QString& name) {
    QStringList& list = variableListenersMap[listener];
    list.removeAll(name);
}

void TargetVariablesModel::unsubscribeToVariablesOfInterest(VariableListener* plugin) {
    if(variableListenersMap.contains(plugin))
        variableListenersMap.remove(plugin);
}


TargetSubroutinesModel::TargetSubroutinesModel(QObject* parent) : QStringListModel(parent) {}

void TargetSubroutinesModel::updateSubroutineTable(const Compiler::SubroutineTable& subroutineTable) {
    QStringList subroutineNames;
    for(auto it = subroutineTable.begin(); it != subroutineTable.end(); ++it)
        subroutineNames.push_back(QString::fromStdWString(it->name));
    setStringList(subroutineNames);
}

/*@}*/
}  // namespace Aseba
