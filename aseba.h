#ifndef ASEBA_CLIENT_H
#define ASEBA_CLIENT_H

#include <QVariantMap>
#include <QThread>
#include <QTimer>
#include "dashel/dashel.h"
#include "aseba/common/msg/msg.h"
#include "aseba/common/msg/NodesManager.h"

class DashelHub: public QObject, public Dashel::Hub {
	Q_OBJECT
public slots:
	void start(QString target);
	void send(Dashel::Stream* stream, const Aseba::Message& message);
signals:
	void connectionCreated(Dashel::Stream* stream) Q_DECL_OVERRIDE;
	void incomingData(Dashel::Stream* stream) Q_DECL_OVERRIDE;
	void connectionClosed(Dashel::Stream* stream, bool abnormal) Q_DECL_OVERRIDE;
	void error(QString source, QString reason);
private:
	void exception(Dashel::DashelException&);
};

class AsebaDescriptionsManager: public QObject, public Aseba::NodesManager {
	Q_OBJECT
signals:
	void nodeProtocolVersionMismatch(unsigned nodeId, const std::wstring &nodeName, uint16 protocolVersion) Q_DECL_OVERRIDE;
	void nodeDescriptionReceived(unsigned nodeId) Q_DECL_OVERRIDE;
	void nodeConnected(unsigned nodeId) Q_DECL_OVERRIDE;
	void nodeDisconnected(unsigned nodeId) Q_DECL_OVERRIDE;
	void sendMessage(const Aseba::Message& message) Q_DECL_OVERRIDE;
public slots:
	void pingNetwork() { Aseba::NodesManager::pingNetwork(); }
};

class AsebaNode;

class AsebaClient: public QObject {
	Q_OBJECT
	Q_PROPERTY(const QList<QObject*> nodes MEMBER nodes NOTIFY nodesChanged)
	QThread thread;
	DashelHub hub;
	AsebaDescriptionsManager manager;
	QTimer managerTimer;
	Dashel::Stream* stream;
	QList<QObject*> nodes;
public:
	AsebaClient();
	~AsebaClient();
public slots:
	void start(QString target = ASEBA_DEFAULT_TARGET);
	void send(const Aseba::Message& message);
	void receive(Aseba::Message* message);
	void sendUserMessage(int eventId, QList<int> args);
signals:
	void userMessage(unsigned type, QList<int> data);
	void nodesChanged();
	void connectionError(QString source, QString reason);
};

class AsebaNode: public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name CONSTANT)
	unsigned nodeId;
	Aseba::TargetDescription description;
	Aseba::VariablesMap variablesMap;
public:
	explicit AsebaNode(AsebaClient* parent, unsigned nodeId, const Aseba::TargetDescription* description);
	AsebaClient* parent() const { return static_cast<AsebaClient*>(QObject::parent()); }
	unsigned id() const { return nodeId; }
	QString name() { return QString::fromStdWString(description.name); }
	static Aseba::CommonDefinitions commonDefinitionsFromEvents(QVariantMap events);
public slots:
	void setVariable(QString name, QList<int> value);
	QString setProgram(QVariantMap events, QString source);
};

#endif // ASEBA_CLIENT_H
