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

#pragma once

#include <QAbstractTableModel>
#include <aseba/qt-thymio-dm-client-lib/request.h>
#include <QVector>
#include <QString>


namespace Aseba {
/** \addtogroup studio */
/*@{*/

class FlatVariablesModel : public QAbstractTableModel {
    Q_OBJECT

public:
    FlatVariablesModel(QString tooltipText, QObject* parent = nullptr);
    FlatVariablesModel(QObject* parent = nullptr);

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
    bool dropMimeData(const QMimeData*, Qt::DropAction, int, int, const QModelIndex&) override {
        return false;
    }

public Q_SLOTS:
    void addVariable(const QString& name, const QVariant& value);
    void removeVariable(const QString& name);
    void clear();

protected:
    QVector<QPair<QString, QVariant>> m_values;
    QString privateMimeType;

private:
};

class ConstantsModel : public FlatVariablesModel {
    Q_OBJECT

public:
    using FlatVariablesModel::FlatVariablesModel;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

Q_SIGNALS:
    void constantModified(const QString& name, const QVariant& value);
};

class MaskableVariablesModel : public FlatVariablesModel {
    Q_OBJECT

public:
    using FlatVariablesModel::FlatVariablesModel;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool isVisible(const unsigned id) const;
    bool isVisible(const QString& key) const;

public Q_SLOTS:
    void toggle(const QModelIndex& index);

private:
    std::vector<bool> m_events_visibility;
};

}  // namespace Aseba
