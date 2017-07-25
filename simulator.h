#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <mutex>
#include <QVariantMap>
#include <QVariantList>

//! A light wrapper of Aseba-enabled simulated Thymio in Enki
class Simulator: public QObject
{
	Q_OBJECT

signals:
	//! The simulation is finished.
	void simulationCompleted(const QVariantList& log) const;
	//! The simulation has messages to print
	void notify(const QString& level, const QString& description, const QStringList& arguments) const;

public slots:
	//! Run the simulation.
	//! If there is an error in the arguments, it is returned, otherwise "" is returned.
	QString runProgram(const QVariantMap& scenario, const QVariantMap& events, const QString& source) const;
};

#endif // SIMULATOR_H
