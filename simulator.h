#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <mutex>
#include <QVariantMap>
#include <QVariantList>
#include <QVector2D>
#include <QJSValue>

//! A light wrapper of Aseba-enabled simulated Thymio in Enki
class Simulator : public QObject {
    Q_OBJECT

signals:
    //! The simulation is finished.
    void simulationCompleted(const QVariantList& log) const;
    //! The simulation has messages to print
    void notify(const QString& level, const QString& description,
                const QStringList& arguments) const;

public slots:
    //! Run the simulation.
    //! If there is an error in the arguments, it is returned, otherwise "" is returned.
    QString runProgram(const QVariantMap& scenario, const QVariantMap& events,
                       const QString& source, QJSValue callback = QJSValue()) const;
};

namespace Enki {
template<typename AsebaRobot>
class DirectlyConnected;
class AsebaThymio2;
typedef DirectlyConnected<AsebaThymio2> DirectAsebaThymio2;
}    // namespace Enki

//! A light wrapper of a Thymio in the simulator
class ThymioRobotInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVector2D position READ position)
    Q_PROPERTY(double orientation READ orientation)
    Q_PROPERTY(QVariantList horizontalSensors READ horizontalSensors)
    Q_PROPERTY(QVariantList groundSensors READ groundSensors)
    Q_PROPERTY(QVariantList nativeCalls READ nativeCalls)

public:
    ThymioRobotInterface(Enki::DirectAsebaThymio2& thymio);

    QVector2D position() const;
    double orientation() const;
    QVariantList horizontalSensors() const;
    QVariantList groundSensors() const;
    QVariantList nativeCalls() const;

    Q_INVOKABLE void tap();
    Q_INVOKABLE void clap();
    Q_INVOKABLE void pressBackwardButton();
    Q_INVOKABLE void releaseBackwardButton();
    Q_INVOKABLE void pressLeftButton();
    Q_INVOKABLE void releaseLeftButton();
    Q_INVOKABLE void pressCenterButton();
    Q_INVOKABLE void releaseCenterButton();
    Q_INVOKABLE void pressForwardButton();
    Q_INVOKABLE void releaseForwardButton();
    Q_INVOKABLE void pressRightButton();
    Q_INVOKABLE void releaseRightButton();

protected:
    Enki::DirectAsebaThymio2& thymio;
};

#endif    // SIMULATOR_H
