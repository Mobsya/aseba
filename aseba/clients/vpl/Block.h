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

#ifndef VPL_CARD_H
#define VPL_CARD_H

#include <QObject>
#include <QGraphicsObject>
#include <QList>
#include <QDomElement>

class QMimeData;
class QSlider;
class QDomDocument;

namespace Aseba {
namespace ThymioVPL {
    /** \addtogroup studio */
    /*@{*/

    class GeometryShapeButton;

    /**
        An "event" or "action" block.

        These blocks have a type (event or action) and a name (prox, etc.)
        and may provide several values (set/get by setValue()/getValue()).
        These values are set by the user through buttons (typically
        GeometryShapeButton), sliders, or specific widgets.
    */
    class Block : public QGraphicsObject {
        Q_OBJECT

    public:
        // TODO: move that somewhere else
        class ThymioBody : public QGraphicsItem {
        public:
            ThymioBody(QGraphicsItem* parent = nullptr, int yShift = 0)
                : QGraphicsItem(parent), bodyColor(Qt::white), yShift(yShift), up(true) {}

            void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
            QRectF boundingRect() const override {
                return QRectF(-128, -128 + yShift, 256, 256);
            }
            void setUp(bool u) {
                up = u;
            }

            static void drawBody(QPainter* painter, int xShift, int yShift, bool up, const QColor& bodyColor);

            QColor bodyColor;

        private:
            const int yShift;
            bool up;
        };

        static Block* createBlock(const QString& name, bool advanced = false, QGraphicsItem* parent = nullptr);

        Block(QString type, QString name, QGraphicsItem* parent = nullptr);
        ~Block() override;

        QString getType() const {
            return type;
        }
        QString getName() const {
            return name;
        }
        QString getTranslatedName() const;
        unsigned getNameAsUInt4() const;

        virtual unsigned valuesCount() const = 0;
        virtual int getValue(unsigned i) const = 0;
        virtual void setValue(unsigned i, int value) = 0;
        virtual QVector<uint16_t> getValuesCompressed() const = 0;
        virtual bool isAnyValueSet() const;
        void resetValues();

        virtual bool isAdvancedBlock() const {
            return false;
        }
        virtual bool isAnyAdvancedFeature() const {
            return isAdvancedBlock();
        }
        virtual void setAdvanced(bool) {}

        QMimeData* mimeData() const;
        QDomElement serialize(QDomDocument& document) const;
        static Block* deserialize(const QDomElement& element, bool advanced);
        static Block* deserialize(const QByteArray& data, bool advanced);
        static QString deserializeType(const QByteArray& data);
        static QString deserializeName(const QByteArray& data);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        QRectF boundingRect() const override {
            return QRectF(0, 0, 256, 256);
        }
        virtual QImage image(qreal factor = 1);
        QImage translucidImage(qreal factor = 1);

        void render(QPainter& painter);

    protected slots:
        void clearChangedFlag();
        void setChangedFlag();
        void emitUndoCheckpointAndClearIfChanged();

    signals:
        void contentChanged();
        void undoCheckpoint();

    public:
        const QString type;
        const QString name;
        bool beingDragged;
        bool keepAfterDrop;

    protected:
        void renderChildItems(QPainter& painter, QGraphicsItem* item, QStyleOptionGraphicsItem& opt);
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

        bool changed;
    };

    class BlockWithNoValues : public Block {
    public:
        BlockWithNoValues(const QString& type, const QString& name, QGraphicsItem* parent);

        unsigned valuesCount() const override {
            return 0;
        }
        int getValue(unsigned) const override {
            return -1;
        }
        void setValue(unsigned, int) override {}
        QVector<uint16_t> getValuesCompressed() const override {
            return QVector<uint16_t>();
        }
    };

    class BlockWithBody : public Block {
    public:
        BlockWithBody(const QString& type, const QString& name, bool up, QGraphicsItem* parent);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    public:
        const bool up;

    protected:
        QColor bodyColor;
    };

    class BlockWithButtons : public BlockWithBody {
    public:
        BlockWithButtons(const QString& type, const QString& name, bool up, QGraphicsItem* parent);

        unsigned valuesCount() const override;
        int getValue(unsigned i) const override;
        void setValue(unsigned i, int value) override;
        QVector<uint16_t> getValuesCompressed() const override;

    protected:
        QList<GeometryShapeButton*> buttons;
    };

    class BlockWithButtonsAndRange : public BlockWithButtons {
        Q_OBJECT

    public:
        enum PixelToValModel { PIXEL_TO_VAL_LINEAR, PIXEL_TO_VAL_SQUARE };

    public:
        BlockWithButtonsAndRange(const QString& type, const QString& name, bool up,
                                 const PixelToValModel pixelToValModel, int lowerBound, int upperBound, int defaultLow,
                                 int defaultHigh, QColor lowColor, QColor highColor, bool advanced,
                                 QGraphicsItem* parent);

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        unsigned valuesCount() const override;
        int getValue(unsigned i) const override;
        void setValue(unsigned i, int value) override;
        QVector<uint16_t> getValuesCompressed() const override;
        bool isAnyValueSet() const override;

        bool isAnyAdvancedFeature() const override;
        void setAdvanced(bool advanced) override;

    public:
        const PixelToValModel pixelToValModel;  //< whether we have a linear or square mapping
        const int lowerBound;
        const int upperBound;
        const int range;
        const int defaultLow;
        const int defaultHigh;
        const QColor lowColor;
        const QColor highColor;

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

        QRectF rangeRect() const;
        float pixelToVal(float pixel) const;
        float valToPixel(float val) const;

        QGraphicsItem* createIndicationLED(int x, int y);

    protected slots:
        void updateIndicationLEDsOpacity();

    protected:
        int low;                 //< low activation threshold (at right)
        int high;                //< high activation threshold (at left)
        int buttonsCountSimple;  //< only show that number of buttons in basic mode, default is
                                 //-1, no limitation
        bool lastPressedIn;      //< whether last mouse press event was in
        bool showRangeControl;   //< whether we are in advanced mode

        QList<QGraphicsItem*> indicationLEDs;  //< indication LEDs on the robot's body
    };

    class StateFilterBlock : public BlockWithButtons {
    public:
        StateFilterBlock(const QString& type, const QString& name, QGraphicsItem* parent = nullptr);

        bool isAdvancedBlock() const override {
            return true;
        }
    };

    /*@}*/
}  // namespace ThymioVPL
}  // namespace Aseba

#endif
