/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "WSServer.h"

WSServer::WSServer(QObject *parent):
    QObject(parent)
{
    wsServer = new QWebSocketServer(QStringLiteral("Emcp Deploy Tool Server"),
                                    QWebSocketServer::NonSecureMode,
                                    this);

    if (wsServer->listen(QHostAddress::Any, MOOLTICUTE_DAEMON_PORT))
    {
        qDebug() << "Todo server listening on port " << MOOLTICUTE_DAEMON_PORT;
    }
    else
    {
        qCritical() << "Failed to listen on port " << MOOLTICUTE_DAEMON_PORT;
        throw 1;
    }

    connect(MPManager::Instance(), SIGNAL(mpConnected(MPDevice*)), this, SLOT(mpAdded(MPDevice*)));
    connect(MPManager::Instance(), SIGNAL(mpDisconnected(MPDevice*)), this, SLOT(mpRemoved(MPDevice*)));

    if (MPManager::Instance()->getDeviceCount() > 0)
        mpAdded(MPManager::Instance()->getDevice(0));
}

WSServer::~WSServer()
{
    wsServer->close();
    qDeleteAll(wsClients.begin(), wsClients.end());
}

void WSServer::onNewConnection()
{
    QWebSocket *wsocket = wsServer->nextPendingConnection();

    connect(wsocket, &QWebSocket::disconnected, this, &WSServer::socketDisconnected);
    wsClients[wsocket] = new WSServerCon(wsocket);

    qDebug() << "New connection";
}

void WSServer::socketDisconnected()
{
    QWebSocket *wsocket = qobject_cast<QWebSocket *>(sender());
    if (wsocket && wsClients.contains(wsocket))
    {
        wsClients[wsocket]->deleteLater();
        wsClients.remove(wsocket);
    }

    qDebug() << "Connection closed";
}

void WSServer::notifyClients(const QJsonObject &obj)
{
    for (auto it = wsClients.begin();it != wsClients.end();it++)
    {
        it.value()->sendJsonMessage(obj);
    }
}

void WSServer::mpAdded(MPDevice *dev)
{
    //tell clients that current mp is not connected anymore
    //and use new connected mp instead
    if (device && device != dev)
        mpRemoved(device);

    qDebug() << "Mooltipass connected";
    device = dev;

//    connect(device, &MPDevice::statusChanged, [=]()
//    {
//        ui->plainTextEdit->appendPlainText(QString("Status: %1").arg(Common::MPStatusString[device->get_status()]));
//    });
}

void WSServer::mpRemoved(MPDevice *dev)
{
    if (device == dev)
    {
        qDebug() << "Mooltipass disconnected";
        device = nullptr;
    }
}