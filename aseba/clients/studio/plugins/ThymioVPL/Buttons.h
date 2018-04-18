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

#ifndef VPL_BUTTONS_H
#define VPL_BUTTONS_H

#include <QGraphicsItem>
#include <QPushButton>

class QSlider;
class QTimeLine;
class QMimeData;

// FIXME: split this file into two

namespace Aseba {
namespace ThymioVPL {
    /** \addtogroup studio */
    /*@{*/

    class Block;
    class ThymioVisualProgramming;

    class GeometryShapeButton : public QGraphicsObject {
        Q_OBJECT

    public:
        enum ButtonType { CIRCULAR_BUTTON = 0, TRIANGULAR_BUTTON, RECTANGULAR_BUTTON, QUARTER_CIRCLE_BUTTON };

        //! Create a button with initially one state
        GeometryShapeButton(const QRectF rect, const ButtonType type, QGraphicsItem* parent = nullptr,
                            const QColor& initBrushColor = Qt::white, const QColor& initPenColor = Qt::black);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        QRectF boundingRect() const override {
            return boundingRectangle;
        }

        int getValue() const {
            return curState;
        }
        void setValue(int state);
        unsigned valuesCount() const;
        void setStateCountLimit(int limit);

        void setToggleState(bool state) {
            toggleState = state;
            update();
        }

        void addState(const QColor& brushColor = Qt::white, const QColor& penColor = Qt::black);

        void addSibling(GeometryShapeButton* s) {
            siblings.push_back(s);
        }

    signals:
        void stateChanged();

    protected:
        const ButtonType buttonType;
        const QRectF boundingRectangle;

        int stateCountLimit;  //!< limit of the value of curState, -1 if disabled
        int curState;
        bool toggleState;

        //! a (brush,pen) color pair
        typedef QPair<QColor, QColor> ColorPair;
        QList<ColorPair> colors;

        QList<GeometryShapeButton*> siblings;

        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    };

    class AddRemoveButton : public QGraphicsObject {
        Q_OBJECT

    public:
        AddRemoveButton(bool add, QGraphicsItem* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        QRectF boundingRect() const override;

    signals:
        void clicked();

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    protected:
        bool add;
    };

    class RemoveBlockButton : public QGraphicsObject {
        Q_OBJECT

    public:
        RemoveBlockButton(QGraphicsItem* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        QRectF boundingRect() const override;

    signals:
        void clicked();

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    };

    class BlockButton : public QPushButton {
        Q_OBJECT

    signals:
        void contentChanged();
        void undoCheckpoint();

    public:
        BlockButton(const QString& name, ThymioVisualProgramming* vpl, QWidget* parent = nullptr);
        ~BlockButton() override;

        QString getName() const;

        void updateBlockImage(bool advanced);

    protected:
        void mouseMoveEvent(QMouseEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    protected:
        Block* block;
        ThymioVisualProgramming* vpl;
    };

    /*@}*/
}  // namespace ThymioVPL
}  // namespace Aseba

#endif  // VPL_BUTTONS_H
