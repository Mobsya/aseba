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
	// Parameters
	const double dt(0.2);

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
	Enki::getWorld = [&]() { return &world; };

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
	DirectAsebaThymio2* thymio(new DirectAsebaThymio2());
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
	vector<Message*> setBytecodeMessages;
	sendBytecode(setBytecodeMessages, nodeId, vector<uint16>(bytecode.begin(), bytecode.end()));
	for_each(setBytecodeMessages.begin(), setBytecodeMessages.end(), [=](Message* message){ thymio->inQueue.emplace(message); });

	// Run the code and log needed information
	QVariantList positionLog;
	QVariantList sensorLog;
	thymio->inQueue.emplace(new Run(nodeId));
	for (uint64_t i = 0; i < uint64_t(duration/dt); ++i)
	{
		// do step
		step();
		// log position
		QVariantList position;
		position.append(thymio->pos.x);
		position.append(thymio->pos.y);
		position.append(thymio->angle);
		positionLog.append(position);
		// log sensors
		QVariantList sensor;
		sensor.append(thymio->infraredSensor0.getValue());
		sensor.append(thymio->infraredSensor1.getValue());
		sensor.append(thymio->infraredSensor2.getValue());
		sensor.append(thymio->infraredSensor3.getValue());
		sensor.append(thymio->infraredSensor4.getValue());
		sensor.append(thymio->groundSensor0.getValue());
		sensor.append(thymio->groundSensor1.getValue());
		sensorLog.append(sensor);
	}

	emit simulationCompleted(positionLog, sensorLog);
	return "";
}
