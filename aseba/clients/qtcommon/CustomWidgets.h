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

#ifndef CUSTOM_WIDGETS_H
#define CUSTOM_WIDGETS_H

#include <QListWidget>
#include <QTableView>

namespace Aseba {
/** \addtogroup studio */
/*@{*/

class DraggableListWidget : public QListWidget {
protected:
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QList<QListWidgetItem*> items) const override;
};

class FixedWidthTableView : public QTableView {
protected:
    int col1Width;

public:
    FixedWidthTableView();
    void setSecondColumnLongestContent(const QString& content);

protected:
    void resizeEvent(QResizeEvent* event) override;

    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    QPixmap getDragPixmap(QString text);
    bool modelMatchMimeFormat(QStringList candidates);
};

/*@}*/
}  // namespace Aseba

#endif
