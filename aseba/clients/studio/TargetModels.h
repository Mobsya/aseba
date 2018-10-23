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

#ifndef TARGET_MODELS_H
#define TARGET_MODELS_H

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QStringListModel>
#include <QVector>
#include <QList>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include "common/msg/msg.h"
#include "compiler/compiler.h"

namespace Aseba {
/** \addtogroup studio */
/*@{*/

struct TargetDescription;
class TargetVariablesModel;

//! Classes that want to listen to variable changes should inherit from this class
class VariableListener {
protected:
    TargetVariablesModel* variablesModel;

public:
    VariableListener(TargetVariablesModel* variablesModel);
    virtual ~VariableListener();

    bool subscribeToVariableOfInterest(const QString& name);
    void unsubscribeToVariableOfInterest(const QString& name);
    void unsubscribeToVariablesOfInterest();
    void invalidateVariableModel();

protected:
    friend class TargetVariablesModel;
    //! New values are available for a variable this plugin is interested in
    virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values) = 0;
};

class TargetVariablesModel : public QAbstractItemModel {
    Q_OBJECT

    // FIXME: uses map for fast variable access

public:
    // variables
    struct Variable {
        QString name;
        unsigned pos;
        VariablesDataVector value;
    };

public:
    TargetVariablesModel(QObject* parent = nullptr);
    ~TargetVariablesModel() override;

    Qt::DropActions supportedDropActions() const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    const QList<Variable>& getVariables() const {
        return variables;
    }
    unsigned getVariablePos(const QString& name) const;
    unsigned getVariableSize(const QString& name) const;
    VariablesDataVector getVariableValue(const QString& name) const;

public slots:
    void updateVariablesStructure(const VariablesMap* variablesMap);
    void setVariablesData(unsigned start, const VariablesDataVector& data);
    bool setVariableValues(const QString& name, const VariablesDataVector& values);

signals:
    //! Emitted on setData, when the user change the data, not when nodes have sent updated
    //! variables
    void variableValuesChanged(unsigned index, const VariablesDataVector& values);

private:
    friend class VariableListener;
    // VariableListener API

    //! Unsubscribe the plugin from any variables it is listening to
    void unsubscribeViewPlugin(VariableListener* plugin);
    //! Subscribe to a variable of interest, return true if variable exists, false otherwise
    bool subscribeToVariableOfInterest(VariableListener* plugin, const QString& name);
    //! Unsubscribe to a variable of interest
    void unsubscribeToVariableOfInterest(VariableListener* plugin, const QString& name);
    //! Unsubscribe to all variables of interest for a given plugin
    void unsubscribeToVariablesOfInterest(VariableListener* plugin);

private:
    QList<Variable> variables;

    // VariablesViewPlugin API
    typedef QMap<VariableListener*, QStringList> VariableListenersNameMap;
    VariableListenersNameMap variableListenersMap;
};

class TargetSubroutinesModel : public QStringListModel {
public:
    TargetSubroutinesModel(QObject* parent = nullptr);

    void updateSubroutineTable(const Compiler::SubroutineTable& subroutineTable);
};

/*@}*/
}  // namespace Aseba

#endif
