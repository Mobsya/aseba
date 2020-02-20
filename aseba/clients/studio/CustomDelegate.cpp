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

#include "CustomDelegate.h"
#include <QtWidgets>

namespace Aseba {
/** \addtogroup studio */
/*@{*/

SpinBoxDelegate::SpinBoxDelegate(int minValue, int maxValue, QObject* parent)
    : QItemDelegate(parent), minValue(minValue), maxValue(maxValue) {}

QWidget* SpinBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /* option */,
                                       const QModelIndex& /* index */) const {
    auto* editor = new QSpinBox(parent);
    editor->setMinimum(minValue);
    editor->setMaximum(maxValue);
    editor->installEventFilter(const_cast<SpinBoxDelegate*>(this));

    return editor;
}

void SpinBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    int value = index.model()->data(index, Qt::DisplayRole).toInt();

    auto* spinBox = static_cast<QSpinBox*>(editor);
    spinBox->setValue(value);
}

void SpinBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    auto* spinBox = static_cast<QSpinBox*>(editor);
    spinBox->interpretText();
    int value = spinBox->value();

    model->setData(index, value);
}

void SpinBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                           const QModelIndex& /* index */) const {
    editor->setGeometry(option.rect);
}

namespace {
    QString pretty_print_variable(const QVariant& v, const size_t max_size) {
        if(v.type() == QVariant::List) {
            auto l = v.toList();
            if(l.size() == 1) {
                return l[0].toString();
            }
            if(l.size() < 10 && std::all_of(l.begin(), l.end(), [](const QVariant& v) {
                   return v.type() == QVariant::Int || v.type() == QVariant::UInt || v.type() == QVariant::LongLong ||
                       v.type() == QVariant::ULongLong;
               })) {
                QStringList values;
                std::transform(l.begin(), l.end(), std::back_inserter(values),
                               [](const QVariant& v) { return QString::number(v.toLongLong()); });
                QString s = "[" + values.join(", ") + "]";
                if(size_t(s.size()) < max_size)
                    return s;
            }
        }
        if(v.type() == QVariant::List || v.type() == QVariant::StringList) {
            return QObject::tr("list <%1 elements>").arg(v.toList().size());
        }
        if(v.type() == QVariant::Map) {
            return QObject::tr("map <%1 elements>").arg(v.toMap().size());
        }
        return v.toString();
    }
}  // namespace

void VariableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_ASSERT(index.isValid());
    QStyleOptionViewItem opt = setOptions(index, option);

    // prepare
    painter->save();
    QString text;
    QRect displayRect = opt.rect;
    auto value = index.data(Qt::DisplayRole);
    if(value.isValid() && !value.isNull()) {
        const QVariant fontVal = index.data(Qt::FontRole);
        const QFont fnt = qvariant_cast<QFont>(fontVal).resolve(option.font);
        auto rect = displayRect.adjusted(-5, -5, 0, 0);
        auto max_count = rect.width() / QFontMetrics(fnt).averageCharWidth();
        text = pretty_print_variable(value, max_count);
    }
    QRect u1, u2;
    // do the layout
    doLayout(opt, &u1, &u2, &displayRect, false);
    // draw the item
    drawBackground(painter, opt, index);
    drawDisplay(painter, opt, displayRect, text);
    drawFocus(painter, opt, displayRect);
    // done
    painter->restore();
}

/*@}*/
}  // namespace Aseba
