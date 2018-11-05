/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TCPTARGET_H
#define TCPTARGET_H

#include "Target.h"
#include "common/consts.h"
#include "common/msg/NodesManager.h"
#include <QString>
#include <QDialog>
#include <QQueue>
#include <QTimer>
#include <QThread>
#include <QTime>
#include <QMap>
#include <QSet>
#include <QVariant>
#include <QMessageBox>
#include <map>
#include <dashel/dashel.h>
#ifdef ZEROCONF_SUPPORT
#    include "common/zeroconf/zeroconf-qt.h"
#endif  // ZEROCONF_SUPPORT

class QPushButton;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QListWidget;
class QComboBox;
class QTranslator;
class QListWidgetItem;

namespace Dashel {
class Stream;
}

namespace Aseba {
/** \addtogroup studio */
/*@{*/

class DashelConnectionDialog : public QDialog {
    Q_OBJECT

protected:
    QPushButton* connectButton;
    QListWidget* discoveredList;
    QLineEdit* currentTarget;
    QComboBox* languageSelectionBox;
#ifdef ZEROCONF_SUPPORT
    Aseba::QtZeroconf zeroconf;
#endif  // ZEROCONF_SUPPORT

public:
    DashelConnectionDialog();
    std::string getTarget();
    QString getLocaleName();

protected:
    bool updatePortList(const QString& toSelect);
    void timerEvent(QTimerEvent* event) override;

protected slots:
#ifdef ZEROCONF_SUPPORT
    void zeroconfTargetFound(const Aseba::Zeroconf::TargetInformation& target);
    void zeroconfTargetRemoved(const Aseba::Zeroconf::TargetInformation& target);
#endif  // ZEROCONF_SUPPORT
    void updateCurrentTarget();
    void connectToItem(QListWidgetItem* item);
    QListWidgetItem* addEntry(const QString& title, const QString& connectionType, const QString& dashelTarget,
                              const QString& additionalInfo = "", const QVariantList& additionalData = QVariantList());

private slots:
    void targetTemplateSerial();
    void targetTemplateLocalTCP();
    void targetTemplateTCP();
    void targetTemplateDoc();
};

class Message;
class UserMessage;

class DashelInterface : public QThread, public Dashel::Hub, public NodesManager {
    Q_OBJECT

public:
    bool isRunning;
    Dashel::Stream* stream;
    std::string lastConnectedTarget;
    std::string lastConnectedTargetName;
    QString language;

public:
    DashelInterface(const QString& commandLineTarget);
    bool attemptToReconnect();

    // from Dashel::Hub
    virtual void stop();

signals:
    void messageAvailable(Message* message);
    void dashelDisconnection();
    void nodeDescriptionReceivedSignal(unsigned nodeId);
    void nodeConnectedSignal(unsigned nodeId);
    void nodeDisconnectedSignal(unsigned nodeId);

protected:
    // from QThread
    void run() override;

    // from Dashel::Hub
    void incomingData(Dashel::Stream* stream) override;
    void connectionClosed(Dashel::Stream* stream, bool abnormal) override;

    // from NodesManager
    void nodeProtocolVersionMismatch(unsigned nodeId, const std::wstring& nodeName, uint16_t protocolVersion) override;
    void nodeDescriptionReceived(unsigned nodeId) override;
    void nodeConnected(unsigned nodeId) override;
    void nodeDisconnected(unsigned nodeId) override;

public:
    // from NodesManager, now as public as we want DashelTarget to call this method
    void sendMessage(const Message& message) override;
};

class ReconnectionDialog : public QMessageBox {
    Q_OBJECT

public:
    ReconnectionDialog(DashelInterface& dashelInterface);

protected:
    void timerEvent(QTimerEvent* event) override;

    DashelInterface& dashelInterface;
    unsigned counter;
};

class DashelTarget : public Target {
    Q_OBJECT

protected:
    struct Node {
        Node();

        BytecodeVector debugBytecode;                             //!< bytecode with debug information
        BytecodeVector::EventAddressesToIdsMap eventAddressToId;  //!< map event start addresses to event identifiers
        unsigned steppingInNext;                                  //!< state of node when in next and stepping
        unsigned lineInNext;                                      //!< line of node to execute when in next and stepping
        ExecutionMode executionMode;                              //!< last known execution mode if this node
    };

    using MessageHandler = void (DashelTarget::*)(Message*);
    typedef std::map<unsigned, MessageHandler> MessagesHandlersMap;
    typedef std::map<unsigned, Node> NodesMap;

    DashelInterface dashelInterface;

    MessagesHandlersMap messagesHandlersMap;

    QQueue<UserMessage*> userEventsQueue;
    NodesMap nodes;
    QTimer userEventsTimer;
    // Note: this timer is here rather than in DashelInterface because writeBlocked is here, if
    // wiretBlocked is removed, this timer should be moved
    QTimer listNodesTimer;
    bool writeBlocked;  //!< true if write is being blocked by invasive plugins, false if write is
                        //!< allowed

public:
    DashelTarget(const QString& commandLineTarget);
    ~DashelTarget() override;

    QString getLanguage() const override {
        return dashelInterface.language;
    }
    QList<unsigned> getNodesList() const override;

    virtual void disconnect();

    const TargetDescription* getDescription(unsigned node) const override;

    void uploadBytecode(unsigned node, const BytecodeVector& bytecode) override;
    void writeBytecode(unsigned node) override;
    void reboot(unsigned node) override;

    void sendEvent(unsigned id, const VariablesDataVector& data) override;

    void setVariables(unsigned node, unsigned start, const VariablesDataVector& data) override;
    void getVariables(unsigned node, unsigned start, unsigned length) override;

    void reset(unsigned node) override;
    void run(unsigned node) override;
    void pause(unsigned node) override;
    void next(unsigned node) override;
    void stop(unsigned node) override;

    void setBreakpoint(unsigned node, unsigned line) override;
    void clearBreakpoint(unsigned node, unsigned line) override;
    void clearBreakpoints(unsigned node) override;

protected:
    void blockWrite() override;
    void unblockWrite() override;

protected slots:
    void updateUserEvents();
    void listNodes();
    void messageFromDashel(Message* message);
    void disconnectionFromDashel();
    void nodeDescriptionReceived(unsigned node);

protected:
    void receivedDescription(Message* message);
    void receivedLocalEventDescription(Message* message);
    void receivedNativeFunctionDescription(Message* message);
    void receivedVariables(Message* message);
    void receivedArrayAccessOutOfBounds(Message* message);
    void receivedDivisionByZero(Message* message);
    void receivedEventExecutionKilled(Message* message);
    void receivedNodeSpecificError(Message* message);
    void receivedExecutionStateChanged(Message* message);
    void receivedBreakpointSetResult(Message* message);
    void receivedBootloaderAck(Message* message);

protected:
    bool emitNodeConnectedIfDescriptionComplete(unsigned id, const Node& node);
    int getPCFromLine(unsigned node, unsigned line);
    int getLineFromPC(unsigned node, unsigned pc);
};

/*@}*/
}  // namespace Aseba

#endif
