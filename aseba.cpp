#include "aseba.h"
#include <memory>

#include <QDebug>
#include <sstream>

Q_DECLARE_METATYPE(Aseba::Message*)

std::vector<int16_t> toAsebaVector(const QList<int>& values) {
    return std::vector<int16_t>(values.begin(), values.end());
}

QList<int> fromAsebaVector(const std::vector<int16_t>& values) {
    QList<int> data;
    data.reserve(values.size());
    for(auto value = values.begin(); value != values.end(); ++value)
        data.push_back(*value);
    return data;
}
AsebaClient::AsebaClient() {
    // Isn't that a bit backward ?
    QObject::connect(&managerTimer, &QTimer::timeout, &m_nodesManager,
                     &AsebaDescriptionsManager::pingNetwork);
    QObject::connect(&m_nodesManager, &AsebaDescriptionsManager::sendMessage, this,
                     &AsebaClient::send);
    QObject::connect(&m_nodesManager, &AsebaDescriptionsManager::nodeConnected, this,
                     [this](unsigned nodeId) {
                         auto description = m_nodesManager.getDescription(nodeId);
                         nodes.append(new AsebaNode(this, nodeId, description));
                         emit this->nodesChanged();
                     });
    QObject::connect(&m_nodesManager, &AsebaDescriptionsManager::nodeDisconnected, this,
                     [this](unsigned nodeId) {
                         for(auto it = nodes.begin(); it != nodes.end(); ++it) {
                             auto node = qobject_cast<AsebaNode*>(*it);
                             if(node->id() == nodeId) {
                                 nodes.erase(it);
                                 node->deleteLater();
                                 emit this->nodesChanged();
                                 break;
                             }
                         }
                     });
    managerTimer.start(200);
}


void AsebaClient::connect(const mobsya::ThymioInfo& thymio) {
    m_connection = m_thymioManager.openConnection(thymio);
    if(!m_connection) {
        return;
    }
    QObject::connect(m_connection.get(), &mobsya::DeviceQtConnection::messageReceived, this,
                     &AsebaClient::messageReceived);
    // m_nodesManager.pingNetwork();
}

AsebaClient::~AsebaClient() {
    thread.quit();
    thread.wait();
}

void AsebaClient::start(QString target) {
    if(!m_connection && m_thymioManager.hasDevice()) {
        connect(m_thymioManager.first());
    }
}

void AsebaClient::send(const Aseba::Message& message) {
    if(!m_connection || !m_connection->isOpen()) {
        qDebug() << "Not connected";
        return;
    }
    m_connection->sendMessage(message);
}

void AsebaClient::messageReceived(std::shared_ptr<Aseba::Message> message) {
    m_nodesManager.processMessage(message.get());

    Aseba::UserMessage* userMessage(dynamic_cast<Aseba::UserMessage*>(message.get()));
    if(userMessage)
        emit this->userMessage(userMessage->type, fromAsebaVector(userMessage->data));
}

void AsebaClient::sendUserMessage(int type, QList<int> data) {
    Aseba::VariablesDataVector vector(data.begin(), data.end());
    Aseba::UserMessage message(type, vector);
    send(message);
}


AsebaNode::AsebaNode(AsebaClient* parent, unsigned nodeId,
                     const Aseba::TargetDescription* description)
    : QObject(parent)
    , nodeId(nodeId)
    , description(*description) {
    unsigned dummy;
    variablesMap = description->getVariablesMap(dummy);
}

void AsebaNode::setVariable(QString name, QList<int> value) {
    uint16_t start = variablesMap[name.toStdWString()].first;
    Aseba::VariablesDataVector variablesVector(value.begin(), value.end());
    Aseba::SetVariables message(nodeId, start, variablesVector);
    parent()->send(message);
}

Aseba::CommonDefinitions AsebaNode::commonDefinitionsFromEvents(QVariantMap events) {
    Aseba::CommonDefinitions commonDefinitions;
    for(auto event(events.begin()); event != events.end(); ++event) {
        auto name(event.key().toStdWString());
        auto value(event.value().toInt());
        commonDefinitions.events.push_back(Aseba::NamedValue(name, value));
    }
    return commonDefinitions;
}

QString AsebaNode::setProgram(QVariantMap events, QString source) {
    Aseba::Compiler compiler;
    Aseba::CommonDefinitions commonDefinitions(commonDefinitionsFromEvents(events));
    compiler.setTargetDescription(&description);
    compiler.setCommonDefinitions(&commonDefinitions);

    std::wistringstream input(source.toStdWString());
    Aseba::BytecodeVector bytecode;
    unsigned allocatedVariablesCount;
    Aseba::Error error;
    bool result = compiler.compile(input, bytecode, allocatedVariablesCount, error);

    if(!result) {
        qWarning() << QString::fromStdWString(error.toWString());
        qWarning() << source;
        return QString::fromStdWString(error.message);
    }

    std::vector<std::unique_ptr<Aseba::Message>> messages;
    Aseba::sendBytecode(messages, nodeId, std::vector<uint16_t>(bytecode.begin(), bytecode.end()));
    for(auto& message : messages) {
        parent()->send(*message);
    }

    Aseba::Run run(nodeId);
    parent()->send(run);
    return "";
}
