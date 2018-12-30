/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QSCRIPTDEBUGGERENGINE_H
#define QSCRIPTDEBUGGERENGINE_H

#include <QtCore/qobject.h>

#include <QtNetwork/qhostaddress.h>
//#include <QtNetwork/qabstractsocket.h>

class QScriptEngine;
class QScriptRemoteTargetDebuggerBackend;

class QScriptDebuggerEngine : public QObject
{
    Q_OBJECT
public:
    enum Error {
        NoError,
        HostNotFoundError,
        ConnectionRefusedError,
        HandshakeError,
        SocketError
    };

    QScriptDebuggerEngine(QObject *parent = 0);
    ~QScriptDebuggerEngine();

    void setTarget(QScriptEngine *engine);
    QScriptEngine *target() const;

    void connectToDebugger(const QHostAddress &address, quint16 port);
    void disconnectFromDebugger();

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);

signals:
    void connected();
    void disconnected();
    void error(QScriptDebuggerEngine::Error error);

private:
    QScriptRemoteTargetDebuggerBackend *m_backend;

    Q_DISABLE_COPY(QScriptDebuggerEngine)
};

#endif
