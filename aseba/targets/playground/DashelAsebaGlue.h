/*
    Aseba - an event-based framework for distributed robot control
    Copyright (C) 2007--2013:
        Stephane Magnenat <stephane at magnenat dot net>
        (http://stephane.magnenat.net)
        and other contributors, see authors.txt for details

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

#pragma once

#include "AsebaGlue.h"
#include "EnkiGlue.h"
#include <qzeroconf.h>
#include <QTcpServer>
#include <QTcpSocket>
// Implementation of the connection using Dashel

namespace Aseba {
class SimpleConnectionBase : public QObject, public AbstractNodeConnection {
    Q_OBJECT
public:
    SimpleConnectionBase(const QString& type, const QString& name, unsigned& port);
    ~SimpleConnectionBase();

    uint16_t serverPort() const;

private Q_SLOTS:
    void onNewConnection();
    void onConnectionClosed();
    void onServerError();
    void onClientError();
    void sendBuffer(uint16_t nodeId, const uint8_t* data, uint16_t length) override;
    uint16_t getBuffer(uint8_t* data, uint16_t maxLength, uint16_t* source) override;


    // void connectionCreated(Dashel::Stream* stream) override;
    // ;

    // void connectionClosed(Dashel::Stream* stream, bool abnormal) override;


private:
    void startServiceRegistration();
    QTcpServer* m_server;
    QTcpSocket* m_client;
    QZeroConf* m_zeroconf;
    QString m_robotType;
    QString m_robotName;
    uint16_t m_messageSize = 0;
    uint16_t m_messageSource = 0;
    QByteArray m_lastMessage;

protected:
    void clearBreakpoints();
    bool handleSingleMessageData();
};

template <typename Robot>
class SimpleConnection : public SimpleConnectionBase, public Robot {
public:
    SimpleConnection(const QString& type, const QString& name, unsigned& port, uint16_t nodeId)
        : SimpleConnectionBase(type, name, port), Robot(name.toStdString(), nodeId) {

        Aseba::vmStateToEnvironment[&this->vm] =
            std::make_pair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection*)this);
    }

public:
    void externalInputStep(double) {
        handleSingleMessageData();
    }
};


}  // namespace Aseba
