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

#ifndef NAMED_VALUES_VECTOR_MODEL_H
#define NAMED_VALUES_VECTOR_MODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include "compiler/compiler.h"


namespace Aseba {
/** \addtogroup studio */
/*@{*/

class NamedValuesVectorModel : public QAbstractTableModel {
    Q_OBJECT

public:
    NamedValuesVectorModel(NamedValuesVector* namedValues, QString  tooltipText, QObject* parent = nullptr);
    NamedValuesVectorModel(NamedValuesVector* namedValues, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    void setExtraMimeType(QString mime) {
        privateMimeType = mime;
    }
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDropActions() const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    bool checkIfModified() {
        return wasModified;
    }
    void clearWasModified() {
        wasModified = false;
    }
    void setEditable(bool editable);

    virtual bool moveRow(int oldRow, int& newRow);

    virtual bool validateName(const QString& name) const;

public slots:
    void addNamedValue(const NamedValue& namedValue, int index = -1);
    void delNamedValue(int index);
    void clear();

signals:
    void publicRowsInserted();
    void publicRowsRemoved();

protected:
    NamedValuesVector* namedValues;
    bool wasModified;
    QString privateMimeType;

private:
    QString tooltipText;
    bool editable;
};

class ConstantsModel : public NamedValuesVectorModel {
    Q_OBJECT

public:
    ConstantsModel(NamedValuesVector* namedValues, const QString& tooltipText, QObject* parent = nullptr);
    ConstantsModel(NamedValuesVector* namedValues, QObject* parent = nullptr);

    bool validateName(const QString& name) const override;
};

class MaskableNamedValuesVectorModel : public NamedValuesVectorModel {
    Q_OBJECT

public:
    MaskableNamedValuesVectorModel(NamedValuesVector* namedValues, const QString& tooltipText, QObject* parent = nullptr);
    MaskableNamedValuesVectorModel(NamedValuesVector* namedValues, QObject* parent = nullptr);

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool isVisible(const unsigned id);

    bool moveRow(int oldRow, int& newRow) override;

public slots:
    void addNamedValue(const NamedValue& namedValue);
    void delNamedValue(int index);
    void toggle(const QModelIndex& index);

private:
    std::vector<bool> viewEvent;
};

/*@}*/
}  // namespace Aseba

#endif
