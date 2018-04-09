#include "aseba.h"
#include <memory>

#include <QDebug>
#include <QQmlEngine>
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

Aseba::CommonDefinitions AsebaNode::commonDefinitionsFromEvents(QVariantMap events) {
    Aseba::CommonDefinitions commonDefinitions;
    for(auto event(events.begin()); event != events.end(); ++event) {
        auto name(event.key().toStdWString());
        auto value(event.value().toInt());
        commonDefinitions.events.push_back(Aseba::NamedValue(name, value));
    }
    return commonDefinitions;
}


AsebaClient::AsebaClient()
    : m_robots(&m_thymioManager) {
}


AsebaNode* AsebaClient::createNode(int nodeId) {
    auto r = m_thymioManager.robotFromId(uint16_t(nodeId));
    if(!r) {
        return nullptr;
    }
    auto node = new AsebaNode(r);
    QQmlEngine::setObjectOwnership(node, QQmlEngine::JavaScriptOwnership);
    return node;
}


AsebaClient::~AsebaClient() {
}

AsebaNode::AsebaNode(mobsya::ThymioManager::Robot robot)
    : m_robot(robot) {
    connect(m_robot.get(), &mobsya::ThymioNode::ready, this, &AsebaNode::readyChanged);
}
QString AsebaNode::name() {
    return m_robot->name();
}

bool AsebaNode::isReady() const {
    return m_robot->isReady();
}


void AsebaNode::setVariable(QString name, QList<int> value) {
    m_robot->setVariable(name, value);
}

QString AsebaNode::setProgram(QVariantMap events, QString source) {
    Aseba::Compiler compiler;
    Aseba::CommonDefinitions commonDefinitions(commonDefinitionsFromEvents(events));
    compiler.setTargetDescription(m_robot->description());
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
    Aseba::sendBytecode(messages, m_robot->id(),
                        std::vector<uint16_t>(bytecode.begin(), bytecode.end()));
    for(auto& message : messages) {
        m_robot->sendMessage(*message);
    }

    Aseba::Run run(m_robot->id());
    m_robot->sendMessage(run);
    return "";
}
