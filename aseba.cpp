#include "aseba.h"
#include <memory>

#include <QDebug>
#include <sstream>

std::vector<sint16> toAsebaVector(const QList<int>& values)
{
	return std::vector<sint16>(values.begin(), values.end());
}

QList<int> fromAsebaVector(const std::vector<sint16>& values)
{
	QList<int> data;
	data.reserve(values.size());
	for (auto value = values.begin(); value != values.end(); ++value)
		data.push_back(*value);
	return data;
}

static const char* exceptionSource(Dashel::DashelException::Source source) {
	switch(source) {
	case Dashel::DashelException::SyncError: return "SyncError";
	case Dashel::DashelException::InvalidTarget: return "InvalidTarget";
	case Dashel::DashelException::InvalidOperation: return "InvalidOperation";
	case Dashel::DashelException::ConnectionLost: return "ConnectionLost";
	case Dashel::DashelException::IOError: return "IOError";
	case Dashel::DashelException::ConnectionFailed: return "ConnectionFailed";
	case Dashel::DashelException::EnumerationError: return "EnumerationError";
	case Dashel::DashelException::PreviousIncomingDataNotRead: return "PreviousIncomingDataNotRead";
	case Dashel::DashelException::Unknown: return "Unknown";
	}
	qFatal("undeclared dashel exception source %i", source);
}

void DashelHub::start(QString target) {
	try {
		auto closeStream = [this](Dashel::Stream* stream) {
			if (dataStreams.find(stream) != dataStreams.end())
				Dashel::Hub::closeStream(stream);
		};
		std::unique_ptr<Dashel::Stream, decltype(closeStream)> stream(Dashel::Hub::connect(target.toStdString()), closeStream);
		run();
	} catch (Dashel::DashelException& e) {
		const char* source(exceptionSource(e.source));
		const char* reason(e.what());
		qWarning("DashelException(%s, %i, %s, %s, %p)", source, e.sysError, strerror(e.sysError), reason, e.stream);
		emit error(source, reason);
	}
}

AsebaClient::AsebaClient() {
	hub.moveToThread(&thread);
	manager.moveToThread(&thread);

	QObject::connect(&hub, &DashelHub::connectionCreated, this, [this](Dashel::Stream* stream) {
		this->stream = stream;
		managerTimer.start(1000);
	}, Qt::QueuedConnection);

	QObject::connect(&managerTimer, &QTimer::timeout, &manager, &AsebaDescriptionsManager::pingNetwork, Qt::DirectConnection);

	QObject::connect(&hub, &DashelHub::connectionClosed, this, [this](Dashel::Stream* stream, bool abnormal) {
		emit hub.error("ConnectionClosed", abnormal ? "abnormal" : "normal");
		hub.stop();
	}, Qt::DirectConnection);

	QObject::connect(&hub, &DashelHub::incomingData, this, [this](Dashel::Stream* stream) {
		auto message(Aseba::Message::receive(stream));
		std::unique_ptr<Aseba::Message> ptr(message);
		Q_UNUSED(ptr);

		std::wostringstream dump;
		message->dump(dump);
		qDebug() << "received" << QString::fromStdWString(dump.str());

		manager.processMessage(message);

		Aseba::UserMessage* userMessage(dynamic_cast<Aseba::UserMessage*>(message));
		if (userMessage)
			emit this->userMessage(userMessage->type, fromAsebaVector(userMessage->data));

	}, Qt::DirectConnection);

	QObject::connect(&hub, &DashelHub::error, this, [this](QString source, QString reason) {
		managerTimer.stop();
		stream = nullptr;

		manager.reset();
		for (auto node: nodes) {
			delete node;
		}
		nodes.clear();
		emit this->nodesChanged();
		emit this->connectionError(source, reason);
	}, Qt::QueuedConnection);

	QObject::connect(&manager, &AsebaDescriptionsManager::nodeConnected, this, [this](unsigned nodeId) {
		auto description = manager.getDescription(nodeId);
		nodes.append(new AsebaNode(this, nodeId, description));
		emit this->nodesChanged();
	}, Qt::QueuedConnection);

	QObject::connect(&manager, &AsebaDescriptionsManager::nodeDisconnected, this, [this](unsigned nodeId) {
		for (auto it = nodes.begin(); it != nodes.end(); ++it) {
			auto node = qobject_cast<AsebaNode*>(*it);
			if (node->id() == nodeId) {
				nodes.erase(it);
				node->deleteLater();
				emit this->nodesChanged();
				break;
			}
		}
	}, Qt::QueuedConnection);

	QObject::connect(&manager, &AsebaDescriptionsManager::sendMessage, this, &AsebaClient::send, Qt::DirectConnection);

	thread.start();
}

AsebaClient::~AsebaClient() {
	hub.stop();
	thread.quit();
	thread.wait();
}

void AsebaClient::start(QString target) {
	QMetaObject::invokeMethod(&hub, "start", Qt::QueuedConnection, Q_ARG(QString, target));
}

void AsebaClient::send(const Aseba::Message& message) {
	if (stream) {
		message.serialize(stream);
		stream->flush();
	}
}

AsebaNode::AsebaNode(AsebaClient* parent, unsigned nodeId, const Aseba::TargetDescription* description)
	: QObject(parent), nodeId(nodeId), description(*description) {
	unsigned dummy;
	variablesMap = description->getVariablesMap(dummy);
}

void AsebaNode::setVariable(QString name, QList<int> value) {
	uint16 start = variablesMap[name.toStdWString()].first;
	Aseba::SetVariables::VariablesVector variablesVector(value.begin(), value.end());
	Aseba::SetVariables message(nodeId, start, variablesVector);
	parent()->send(message);
}

QString AsebaNode::setProgram(QString source) {
	Aseba::Compiler compiler;
	compiler.setTargetDescription(&description);
	Aseba::CommonDefinitions commonDefinitions;
	commonDefinitions.events.push_back(Aseba::NamedValue(L"transition", 2));
	compiler.setCommonDefinitions(&commonDefinitions);

	std::wistringstream input(source.toStdWString());
	Aseba::BytecodeVector bytecode;
	unsigned allocatedVariablesCount;
	Aseba::Error error;
	bool result = compiler.compile(input, bytecode, allocatedVariablesCount, error);

	if (!result) {
		qWarning() << QString::fromStdWString(error.toWString());
		qWarning() << source;
		return QString::fromStdWString(error.message);
	}

	std::vector<Aseba::Message*> messages;
	Aseba::sendBytecode(messages, nodeId, std::vector<uint16>(bytecode.begin(), bytecode.end()));
	foreach (auto message, messages) {
		parent()->send(*message);
		delete message;
	}

	Aseba::Run run(nodeId);
	parent()->send(run);
	return "";
}
