#ifndef ASEBA_CLIENT_H
#define ASEBA_CLIENT_H

#include <QVariantMap>
#include <QThread>
#include <QTimer>
#include "aseba/common/msg/msg.h"
#include "aseba/compiler/compiler.h"
#include "thymio/ThymioManager.h"
#include "thymio/ThymioListModel.h"

class AsebaNode;

class AsebaClient : public QObject {
    Q_OBJECT
public:
    using ThymioModel = mobsya::ThymioListModel;
    Q_PROPERTY(mobsya::ThymioListModel* model READ model CONSTANT)

public:
    AsebaClient();
    ~AsebaClient();

public Q_SLOTS:
    ThymioModel* model() {
        return &m_robots;
    }

    /*    void start(QString target = ASEBA_DEFAULT_TARGET);
        void send(const Aseba::Message& message);
        void messageReceived(std::shared_ptr<Aseba::Message>);
        void sendUserMessage(int eventId, QList<int> args);
    Q_SIGNALS:
        void userMessage(unsigned type, QList<int> data);
        void nodesChanged();
        void connectionError(QString source, QString reason);
    */
private:
    mobsya::ThymioManager m_thymioManager;
    mobsya::ThymioListModel m_robots;
    // void connect(const mobsya::ThymioProviderInfo& thymio);
};

class AsebaNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    unsigned nodeId;
    Aseba::TargetDescription description;
    Aseba::VariablesMap variablesMap;

public:
    explicit AsebaNode(AsebaClient* parent, unsigned nodeId,
                       const Aseba::TargetDescription* description);
    AsebaClient* parent() const {
        return static_cast<AsebaClient*>(QObject::parent());
    }
    unsigned id() const {
        return nodeId;
    }
    QString name() {
        return QString::fromStdWString(description.name);
    }
    static Aseba::CommonDefinitions commonDefinitionsFromEvents(QVariantMap events);
public slots:
    void setVariable(QString name, QList<int> value);
    QString setProgram(QVariantMap events, QString source);
};

#endif    // ASEBA_CLIENT_H
