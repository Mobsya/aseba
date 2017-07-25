#include "aseba/targets/playground/EnkiGlue.h"
#include "aseba/targets/playground/DirectAsebaGlue.h"
#include "aseba/common/msg/NodesManager.h"
#include "aseba.h"
#include "simulator.h"
#include <iostream>
#include <QDebug>
#include <QVector2D>
#include <QVector3D>
#include <QColor>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

using namespace Aseba;
using namespace Enki;
using namespace std;

struct SimulatorNodesManager: NodesManager
{
	DirectAsebaThymio2* thymio;

	SimulatorNodesManager(DirectAsebaThymio2* thymio): thymio(thymio) {}

	virtual void sendMessage(const Message& message)
	{
		thymio->inQueue.emplace(message.clone());
	}

	void step()
	{
		while (!thymio->outQueue.empty())
		{
			processMessage(thymio->outQueue.front().get());
			thymio->outQueue.pop();
		}
	}
};

//! An environment for this simulation
struct QtSimulatorEnvironment: SimulatorEnvironment
{
	const Simulator* parent;
	World* world;

	QtSimulatorEnvironment(const Simulator* parent, World* world): parent(parent), world(world) {}

	virtual void notify(const EnvironmentNotificationType type, const std::string& description, const strings& arguments) override
	{
		QString level;
		switch (type)
		{
			case EnvironmentNotificationType::DISPLAY_INFO: level = "display";
			case EnvironmentNotificationType::LOG_INFO: level = "info";
			case EnvironmentNotificationType::LOG_WARNING: level = "warning";
			case EnvironmentNotificationType::LOG_ERROR: level = "error";
			case EnvironmentNotificationType::FATAL_ERROR: level = "error";
		}
		QStringList qArguments;
		transform(arguments.begin(), arguments.end(), back_inserter(qArguments), [] (const string& stdString) { return QString::fromStdString(stdString); });
		emit parent->notify(level, QString::fromStdString(description), qArguments);
	}

	virtual std::string getSDFilePath(const std::string& robotName, unsigned fileNumber) const override
	{
		auto fileName(QString("%1/%2/U%3.DAT")
			.arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
			.arg(QString::fromStdString(robotName))
			.arg(fileNumber)
		);
		QDir().mkpath(QFileInfo(fileName).absolutePath());
		return fileName.toStdString();
	}

	virtual World* getWorld() const override
	{
		return world;
	}
};

struct SimulatorEnvironmentLifeSpanManager
{
	~SimulatorEnvironmentLifeSpanManager()
	{
		simulatorEnvironment.reset();
	}
};

template<typename T>
std::tuple<QString, T> getValue(const QVariantMap& v, const QString& name, const QString& context = "Scenario")
{
	if (!v.contains(name))
		return std::make_tuple(QString("%1 is missing property \"%2\"").arg(context).arg(name), T());
	const auto& entry(v.value(name));
	if (!entry.canConvert<T>())
		return std::make_tuple(QString("%1 property \"%2\" is not of type %3").arg(context).arg(name).arg(typeid(T).name()), T());
	return std::make_tuple("", entry.value<T>());
}

template<class T>
std::remove_reference_t<T> const& as_const(T&&t) { return t; }

QString Simulator::runProgram(const QVariantMap& scenario, const QVariantMap& events, const QString& source) const
{
	std::lock_guard<std::mutex> guard(simulationMutex);

	// Parameters
	const double dt(0.1);

	// validate events
	unsigned i = 0;
	for (auto event = begin(events); event != end(events); ++event)
	{
		if (!event.value().canConvert<int>())
			return QString("Event %1 (index %2) is not an integer").arg(event.key()).arg(i);
		++i;
	}

	// get duration
	double duration;
	QString error;
	tie(error, duration) = getValue<double>(scenario, "duration");
	if (!error.isEmpty()) return error;

	// Create world, robot and nodes manager
	// world
	QVector2D worldSize;
	tie(error, worldSize) = getValue<QVector2D>(scenario, "worldSize");
	if (!error.isEmpty()) return error;
	World world(worldSize.x(), worldSize.y());
	// Make sure that the global Enki pointer to the world is reset when this function is exited
	// this is not clean and is a work-around through the non-reentrant World lookup interface
	SimulatorEnvironmentLifeSpanManager simulatorEnvironmentLifeSpanManager;
	simulatorEnvironment.reset(new QtSimulatorEnvironment(this, &world));

	// walls
	QVariantList walls;
	tie(error, walls) = getValue<QVariantList>(scenario, "walls");
	if (!error.isEmpty()) return error;
	i = 0;
	for (auto&& wall : as_const(walls)) // need this cast to prevent copy as walls is non-const
	{
		// read from description
		const auto context(QString("Wall %1").arg(i));
		if (!wall.canConvert<QVariantMap>())
			return QString("Wall %1 is not an object").arg(i);
		const auto& description(wall.value<QVariantMap>());
		QVector2D position;
		tie(error, position) = getValue<QVector2D>(description, "position", context);
		if (!error.isEmpty()) return error;
		double angle;
		tie(error, angle) = getValue<double>(description, "angle", context);
		if (!error.isEmpty()) return error;
		QVector3D size;
		tie(error, size) = getValue<QVector3D>(description, "size", context);
		if (!error.isEmpty()) return error;
		QColor color;
		tie(error, color) = getValue<QColor>(description, "color", context);
		if (!error.isEmpty()) return error;

		// set to object
		auto wallObject(new Enki::PhysicalObject());
		wallObject->pos = { position.x(), position.y() };
		wallObject->angle = angle;
		wallObject->setRectangular(size[0], size[1], size[2], 0);
		wallObject->setColor(Color(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
		world.addObject(wallObject);
		++i;
	}

	// robot
	QVariantMap thymioDescription;
	tie(error, thymioDescription) = getValue<QVariantMap>(scenario, "thymio");
	if (!error.isEmpty()) return error;
	// read from description
	QVector2D initialPosition;
	tie(error, initialPosition) = getValue<QVector2D>(thymioDescription, "position", "Thymio");
	if (!error.isEmpty()) return error;
	double initialAngle;
	tie(error, initialAngle) = getValue<double>(thymioDescription, "angle", "Thymio");
	if (!error.isEmpty()) return error;
	// set to Thymio
	DirectAsebaThymio2* thymio(new DirectAsebaThymio2("thymio"));
	thymio->logThymioNativeCalls = true;
	thymio->pos = { initialPosition.x(), initialPosition.y() };
	thymio->angle = initialAngle;
	world.addObject(thymio);

	// nod manager
	SimulatorNodesManager SimulatorNodesManager(thymio);

	// List the nodes and step, the robot should send its description to the nodes manager
	thymio->inQueue.emplace(ListNodes().clone());

	// Define a step lambda
	auto step = [&]() {
		world.step(dt);
		SimulatorNodesManager.step();
	};

	// Step twice for the detection and enumeration round-trip
	step();
	step();

	// Check that the nodes manager has received the description from the robot
	bool ok(false);
	unsigned nodeId(SimulatorNodesManager.getNodeId(L"thymio-II", 0, &ok));
	if (!ok)
	{
		qCritical() << "nodes manager did not find \"thymio-II\"";
		return ""; // error in code, not in passed objects
	}
	if (nodeId != 1)
	{
		qCritical() << "nodes manager did not return the right nodeId for \"thymio-II\", should be 1, was " << nodeId;
		return ""; // error in code, not in passed objects
	}

	const TargetDescription *targetDescription(SimulatorNodesManager.getDescription(nodeId));
	if (!targetDescription)
	{
		qCritical() << "nodes manager did not return a target description for \"thymio-II\"";
		return ""; // error in code, not in passed objects
	}

	// Compile the code
	Compiler compiler;
	CommonDefinitions commonDefinitions(AsebaNode::commonDefinitionsFromEvents(events));
	compiler.setTargetDescription(targetDescription);
	compiler.setCommonDefinitions(&commonDefinitions);

	wistringstream input(source.toStdWString());
	BytecodeVector bytecode;
	unsigned allocatedVariablesCount;
	Error compilationError;
	const bool compilationResult(compiler.compile(input, bytecode, allocatedVariablesCount, compilationError));

	if (!compilationResult)
	{
		qWarning() << "compilation error: " << QString::fromStdWString(compilationError.toWString());
		qWarning() << source;
		return QString::fromStdWString(compilationError.message);
	}

	// Fill the bytecode messages
	vector<unique_ptr<Message>> setBytecodeMessages;
	sendBytecode(setBytecodeMessages, nodeId, vector<uint16_t>(bytecode.begin(), bytecode.end()));
	for_each(setBytecodeMessages.begin(), setBytecodeMessages.end(), [=](unique_ptr<Message>& message) { thymio->inQueue.emplace(message.release()); });

	// Run the code and log needed information
	QVariantList log;
	thymio->inQueue.emplace(new Run(nodeId));
	double currentTime(0);
	for (uint64_t i = 0; i < uint64_t(duration/dt); ++i)
	{
		// do step
		step();
		// log time
		QVariantMap logEntry;
		logEntry["time"] = currentTime;
		// log position
		QVariantList position;
		position.append(thymio->pos.x);
		position.append(thymio->pos.y);
		position.append(thymio->angle);
		logEntry["position"] = position;
		// log sensors
		QVariantList sensors;
		sensors.append(thymio->infraredSensor0.getValue());
		sensors.append(thymio->infraredSensor1.getValue());
		sensors.append(thymio->infraredSensor2.getValue());
		sensors.append(thymio->infraredSensor3.getValue());
		sensors.append(thymio->infraredSensor4.getValue());
		sensors.append(thymio->groundSensor0.getValue());
		sensors.append(thymio->groundSensor1.getValue());
		logEntry["sensors"] = sensors;
		qDebug() << thymio->thymioNativeCallLog.size();
		// log native calls
		QVariantList nativeCalls;
		for (const auto& nativeCallEntry: thymio->thymioNativeCallLog)
		{
			QVariantMap nativeCall;
			nativeCall["id"] = nativeCallEntry.first;
			QVariantList qValues;
			transform(nativeCallEntry.second.begin(), nativeCallEntry.second.end(), back_inserter(qValues), [] (int16_t value) { return QVariant(value); } );
			nativeCall["values"] = qValues;
			nativeCalls.append(nativeCall);
		}
		thymio->thymioNativeCallLog.clear();
		logEntry["nativeCalls"] = nativeCalls;
		log.append(logEntry);
		currentTime += dt;
	}

	emit simulationCompleted(log);
	return "";
}
