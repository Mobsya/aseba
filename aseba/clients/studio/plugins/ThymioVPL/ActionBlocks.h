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

#ifndef VPL_ACTION_BLOCKS_H
#define VPL_ACTION_BLOCKS_H

#include <QList>

#include "Block.h"

class QSlider;
class QTimeLine;

namespace Aseba {
namespace ThymioVPL {
    /** \addtogroup studio */
    /*@{*/

    class MoveActionBlock : public Block {
        Q_OBJECT

    public:
        MoveActionBlock(QGraphicsItem* parent = nullptr);
        ~MoveActionBlock() override;

        unsigned valuesCount() const override {
            return 2;
        }
        int getValue(unsigned i) const override;
        void setValue(unsigned i, int value) override;
        QVector<uint16_t> getValuesCompressed() const override;

    private slots:
        void frameChanged(int frame);
        void valueChangeDetected();

    private:
        QList<QSlider*> sliders;
        QTimeLine* timer;
        ThymioBody* thymioBody;
    };

    class ColorActionBlock : public BlockWithBody {
        Q_OBJECT

    protected:
        ColorActionBlock(QGraphicsItem* parent, bool top);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    public:
        unsigned valuesCount() const override {
            return 3;
        }
        int getValue(unsigned i) const override;
        void setValue(unsigned i, int value) override;
        QVector<uint16_t> getValuesCompressed() const override;

    private slots:
        void valueChangeDetected();

    private:
        QList<QSlider*> sliders;
    };

    struct TopColorActionBlock : public ColorActionBlock {
        TopColorActionBlock(QGraphicsItem* parent = nullptr);
    };

    struct BottomColorActionBlock : public ColorActionBlock {
        BottomColorActionBlock(QGraphicsItem* parent = nullptr);
    };

    class SoundActionBlock : public Block {
    public:
        SoundActionBlock(QGraphicsItem* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        unsigned valuesCount() const override {
            return 6;
        }
        int getValue(unsigned i) const override;
        void setValue(unsigned i, int value) override;
        QVector<uint16_t> getValuesCompressed() const override;

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

        void idxAndValFromPos(const QPointF& pos, bool* ok, unsigned& noteIdx, unsigned& noteVal);
        void setNote(unsigned noteIdx, unsigned noteVal);
        void setDuration(unsigned noteIdx, unsigned durationVal);

    protected:
        bool dragging;
        unsigned notes[6];
        unsigned durations[6];
    };

    class TimerActionBlock : public Block {
        Q_OBJECT

    public:
        TimerActionBlock(QGraphicsItem* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        unsigned valuesCount() const override {
            return 1;
        }
        int getValue(unsigned i) const override;
        void setValue(unsigned i, int value) override;
        QVector<uint16_t> getValuesCompressed() const override;

        bool isAdvancedBlock() const override {
            return true;
        }

    protected slots:
        void frameChanged(int frame);

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

        unsigned durationFromPos(const QPointF& pos, bool* ok) const;
        void setDuration(unsigned duration);

    protected:
        bool dragging;
        unsigned duration;
        QTimeLine* timer;
    };

    class StateFilterActionBlock : public StateFilterBlock {
    public:
        StateFilterActionBlock(QGraphicsItem* parent = nullptr);
    };

    /*@}*/
}  // namespace ThymioVPL
}  // namespace Aseba

#endif
