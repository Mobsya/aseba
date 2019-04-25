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

#include "DashelAsebaGlue.h"
#include "EnkiGlue.h"
#include <QDataStream>
#include <QCoreApplication>
#include "transport/buffer/vm-buffer.h"

namespace Aseba {

SimpleConnectionBase::SimpleConnectionBase(const QString & type, const QString & name, unsigned port):
    m_server(new QTcpServer(this)), m_client(nullptr), m_zeroconf(new QZeroConf(this)),
    m_robotType(type),
    m_robotName(name) {
    connect(m_server, &QTcpServer::newConnection, this, &SimpleConnectionBase::onNewConnection);
    m_server->listen(QHostAddress::LocalHost, port);

    m_server->setMaxPendingConnections(1);
    startServiceRegistration();

}
void SimpleConnectionBase::onNewConnection() {
    if(m_client) {
        qWarning() << "This Virtual Robot is already connected to a peer";
        SEND_NOTIFICATION(LOG_WARNING, "client already connected", m_robotName.toStdString());
        return;
    }
    SEND_NOTIFICATION(LOG_INFO, "client connected", "");
    m_client = m_server->nextPendingConnection();
    m_client->open(QIODevice::ReadWrite);
}

void SimpleConnectionBase::startServiceRegistration() {
    m_zeroconf->addServiceTxtRecord("type", m_robotType);
    m_zeroconf->addServiceTxtRecord("protovers", QString::number(9));
    std::string name = QStringLiteral("%1 - %2")
                           .arg(m_robotName)
                           .arg(QCoreApplication::applicationPid()).toStdString();

    m_zeroconf->startServicePublish(
        name.c_str(),
        "_aseba._tcp",
        "local",
        m_server->serverPort(),
        QZeroConf::service_option::localhost_only);
}

void SimpleConnectionBase::sendBuffer(uint16_t nodeId, const uint8_t* data, uint16_t length) {
    if(!m_client)
        return;
    QDataStream stream(m_client);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << uint16_t(length - 2);
    stream << nodeId;
    stream.writeRawData((char*)(data), length);
}

bool SimpleConnectionBase::handleSingleMessageData() {
    if(m_messageSize == 0) {
        if(!m_client || m_client->bytesAvailable() < 2)
            return false;
        QDataStream stream(m_client);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> m_messageSize;
    }
    if(!m_lastMessage.isEmpty())
        return false;

    if(m_messageSize != 0 && m_client->bytesAvailable() < m_messageSize + 2)
        return false;

    QDataStream stream(m_client);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> m_messageSource;
    QByteArray data(m_messageSize + 2, Qt::Uninitialized);
    stream.readRawData(data.data(), data.size());
    m_lastMessage =  data;
    for(auto vmStateToEnvironmentKV : vmStateToEnvironment) {
        if(vmStateToEnvironmentKV.second.second == this) {
            AsebaProcessIncomingEvents(vmStateToEnvironmentKV.first);
            AsebaVMRun(vmStateToEnvironmentKV.first, 1000);
        }
    }
    return true;
}

uint16_t SimpleConnectionBase::getBuffer(uint8_t* data, uint16_t maxLength, uint16_t* source) {
    *source = m_messageSource;
    uint16_t s = qMin(uint16_t(m_lastMessage.size()), maxLength);
    memcpy(data, m_lastMessage.data(), s);
    m_lastMessage = m_lastMessage.mid(s);
    if(m_lastMessage.isEmpty())
        m_messageSize = 0;
    return s;
}

/*void SimpleConnectionBase::connectionClosed(Dashel::Stream* stream, bool abnormal) {
    // if the stream being closed is the current one (not old), clear breakpoints and reset current
    if(stream == this->stream) {
        clearBreakpoints();
        this->stream = nullptr;
    }
    if(abnormal) {
        SEND_NOTIFICATION(LOG_WARNING, "client disconnected abnormally", stream->getTargetName());
    } else {
        SEND_NOTIFICATION(LOG_INFO, "client disconnected properly", stream->getTargetName());
    }
}*/

//! Clear breakpoints on all VM that are linked to this connection
void SimpleConnectionBase::clearBreakpoints() {
    for(auto vmStateToEnvironmentKV : vmStateToEnvironment) {
        if(vmStateToEnvironmentKV.second.second == this)
            vmStateToEnvironmentKV.first->breakpointsCount = 0;
    }
}

}  // namespace Aseba
