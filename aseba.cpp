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

static const char* exceptionSource(Dashel::DashelException::Source source) {
    switch(source) {
        case Dashel::DashelException::SyncError: return "SyncError";
        case Dashel::DashelException::InvalidTarget: return "InvalidTarget";
        case Dashel::DashelException::InvalidOperation: return "InvalidOperation";
        case Dashel::DashelException::ConnectionLost: return "ConnectionLost";
        case Dashel::DashelException::IOError: return "IOError";
        case Dashel::DashelException::ConnectionFailed: return "ConnectionFailed";
        case Dashel::DashelException::EnumerationError: return "EnumerationError";
        case Dashel::DashelException::PreviousIncomingDataNotRead:
            return "PreviousIncomingDataNotRead";
        case Dashel::DashelException::Unknown: return "Unknown";
    }
    qFatal("undeclared dashel exception source %i", source);
}

void DashelHub::start(QString target) {
    try {
        auto closeStream = [this](Dashel::Stream* stream) {
            lock();
            if(dataStreams.find(stream) != dataStreams.end())
                Dashel::Hub::closeStream(stream);
            unlock();
        };
        std::unique_ptr<Dashel::Stream, decltype(closeStream)> stream(
            Dashel::Hub::connect(target.toStdString()), closeStream);
        run();
    } catch(Dashel::DashelException& e) {
        exception(e);
    }
}

void DashelHub::send(Dashel::Stream* stream, const Aseba::Message& message) {
    try {
        lock();
        if(dataStreams.find(stream) != dataStreams.end()) {
            message.serialize(stream);
            stream->flush();
        }
        unlock();
    } catch(Dashel::DashelException& e) {
        unlock();
        exception(e);
        stop();
    }
}

void DashelHub::exception(Dashel::DashelException& exception) {
    auto source(exceptionSource(exception.source));
    auto reason(exception.what());
    auto sysError(exception.sysError);
    auto stream(exception.stream);
    qWarning() << "DashelException" << source << sysError << strerror(sysError) << reason << stream;
    emit error(source, reason);
}


AsebaClient::AsebaClient() : stream(nullptr) {
    hub.moveToThread(&thread);

    QObject::connect(&hub, &DashelHub::connectionCreated, this,
                     [this](Dashel::Stream* stream) { this->stream = stream; },
                     Qt::DirectConnection);

    QObject::connect(&hub, &DashelHub::connectionClosed, this,
                     [this](Dashel::Stream* stream, bool abnormal) {
                         emit hub.error("ConnectionClosed", abnormal ? "abnormal" : "normal");
                         hub.stop();
                     },
                     Qt::DirectConnection);

    QObject::connect(&hub, &DashelHub::incomingData, this,
                     [this](Dashel::Stream* stream) {
                         std::unique_ptr<Aseba::Message> message(Aseba::Message::receive(stream));

                         std::wostringstream dump;
                         message->dump(dump);
                         qDebug() << "received" << QString::fromStdWString(dump.str());

                         static auto asebaMessageMetaTypeId(qRegisterMetaType<Aseba::Message*>());
                         Q_UNUSED(asebaMessageMetaTypeId);
                         QMetaObject::invokeMethod(this, "receive", Qt::QueuedConnection,
                                                   Q_ARG(Aseba::Message*, message.release()));
                     },
                     Qt::DirectConnection);

    QObject::connect(&hub, &DashelHub::error, this,
                     [this](QString source, QString reason) { stream = nullptr; },
                     Qt::DirectConnection);
    QObject::connect(&hub, &DashelHub::error, this,
                     [this](QString source, QString reason) {
                         manager.reset();
                         if(!nodes.empty()) {
                             for(auto node : nodes) {
                                 delete node;
                             }
                             nodes.clear();
                             emit this->nodesChanged();
                         }
                         emit this->connectionError(source, reason);
                     },
                     Qt::QueuedConnection);

    QObject::connect(&managerTimer, &QTimer::timeout, &manager,
                     &AsebaDescriptionsManager::pingNetwork, Qt::DirectConnection);

    QObject::connect(&manager, &AsebaDescriptionsManager::nodeConnected, this,
                     [this](unsigned nodeId) {
                         auto description = manager.getDescription(nodeId);
                         nodes.append(new AsebaNode(this, nodeId, description));
                         emit this->nodesChanged();
                     },
                     Qt::DirectConnection);

    QObject::connect(&manager, &AsebaDescriptionsManager::nodeDisconnected, this,
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
                     },
                     Qt::DirectConnection);

    QObject::connect(&manager, &AsebaDescriptionsManager::sendMessage, this, &AsebaClient::send,
                     Qt::DirectConnection);

    thread.start();
    managerTimer.start(1000);
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
    hub.send(stream, message);
}

void AsebaClient::receive(Aseba::Message* message) {
    std::unique_ptr<Aseba::Message> ptr(message);
    Q_UNUSED(ptr);

    manager.processMessage(message);

    Aseba::UserMessage* userMessage(dynamic_cast<Aseba::UserMessage*>(message));
    if(userMessage)
        emit this->userMessage(userMessage->type, fromAsebaVector(userMessage->data));
}

void AsebaClient::sendUserMessage(int type, QList<int> data) {
    Aseba::VariablesDataVector vector(data.begin(), data.end());
    Aseba::UserMessage message(type, vector);
    send(message);
}


AsebaNode::AsebaNode(AsebaClient* parent, unsigned nodeId,
                     const Aseba::TargetDescription* description) :
    QObject(parent),
    nodeId(nodeId),
    description(*description) {
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
