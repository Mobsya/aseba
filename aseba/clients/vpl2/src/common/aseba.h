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
    AsebaNode* createNode(int nodeId);

private:
    mobsya::ThymioManager m_thymioManager;
    mobsya::ThymioListModel m_robots;
};

class AsebaNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    AsebaNode(mobsya::ThymioManager::Robot robot);
    QString name();
    static Aseba::CommonDefinitions commonDefinitionsFromEvents(QVariantMap events);
public Q_SLOTS:
    void setVariable(QString name, QList<int> value);
    QString setProgram(QVariantMap events, QString source);
    bool isReady() const;
    bool isConnected() const;

Q_SIGNALS:
    void readyChanged();
    void connectedChanged();
    void userMessageReceived(int type, QList<int>);


private:
    mobsya::ThymioManager::Robot m_robot;
};

#endif  // ASEBA_CLIENT_H
