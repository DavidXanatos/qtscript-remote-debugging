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

#include "qscriptdebuggerengine.h"
#include <QtCore/qeventloop.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtScript/qscriptengine.h>
#include <private/qscriptdebuggerbackend_p.h>
#include <private/qscriptdebuggercommand_p.h>
#include <private/qscriptdebuggerevent_p.h>
#include <private/qscriptdebuggerresponse_p.h>
#include <private/qscriptdebuggercommandexecutor_p.h>
#include <private/qscriptbreakpointdata_p.h>
#include <private/qscriptdebuggerobjectsnapshotdelta_p.h>

// #define DEBUGGERENGINE_DEBUG

class QScriptRemoteTargetDebuggerBackend : public QObject,
                                           public QScriptDebuggerBackend
{
    Q_OBJECT
public:
    QScriptRemoteTargetDebuggerBackend();
    ~QScriptRemoteTargetDebuggerBackend();

    void connectToDebugger(const QHostAddress &address, quint16 port);
    void disconnectFromDebugger();

    bool listen(const QHostAddress &address, quint16 port);

    void resume();

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(QScriptDebuggerEngine::Error error);

protected:
    void event(const QScriptDebuggerEvent &event);

private Q_SLOTS:
    void onSocketStateChanged(QAbstractSocket::SocketState);
    void onSocketError(QAbstractSocket::SocketError);
    void onReadyRead();
    void onNewConnection();

private:
    enum State {
        UnconnectedState,
        HandshakingState,
        ConnectedState
    };

private:
    State m_state;
    QTcpSocket *m_socket;
    int m_blockSize;
    QTcpServer *m_server;
    QList<QEventLoop*> m_eventLoopPool;
    QList<QEventLoop*> m_eventLoopStack;

private:
    Q_DISABLE_COPY(QScriptRemoteTargetDebuggerBackend)
};

QScriptRemoteTargetDebuggerBackend::QScriptRemoteTargetDebuggerBackend()
    : m_state(UnconnectedState), m_socket(0), m_blockSize(0), m_server(0)
{
}

QScriptRemoteTargetDebuggerBackend::~QScriptRemoteTargetDebuggerBackend()
{
}

void QScriptRemoteTargetDebuggerBackend::connectToDebugger(const QHostAddress &address, quint16 port)
{
    Q_ASSERT(m_state == UnconnectedState);
    if (!m_socket) {
        m_socket = new QTcpSocket(this);
        QObject::connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                         this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));
        QObject::connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(onSocketError(QAbstractSocket::SocketError)));
        QObject::connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    }
    m_socket->connectToHost(address, port);
}

void QScriptRemoteTargetDebuggerBackend::disconnectFromDebugger()
{
    if (!m_socket)
        return;
    m_socket->disconnectFromHost();
}

bool QScriptRemoteTargetDebuggerBackend::listen(const QHostAddress &address, quint16 port)
{
    if (m_socket)
        return false;
    if (!m_server) {
        m_server = new QTcpServer(this);
        QObject::connect(m_server, SIGNAL(newConnection()),
                         this, SLOT(onNewConnection()));
    }
    return m_server->listen(address, port);
}

void QScriptRemoteTargetDebuggerBackend::onSocketStateChanged(QAbstractSocket::SocketState s)
{
    if (s == QAbstractSocket::ConnectedState) {
        m_state = HandshakingState;
    } else if (s == QAbstractSocket::UnconnectedState) {
        engine()->setAgent(0);
        m_state = UnconnectedState;
        emit disconnected();
    }
}

void QScriptRemoteTargetDebuggerBackend::onSocketError(QAbstractSocket::SocketError err)
{
    if (err != QAbstractSocket::RemoteHostClosedError) {
        qDebug("%s", qPrintable(m_socket->errorString()));
        emit error(QScriptDebuggerEngine::SocketError);
    }
}

void QScriptRemoteTargetDebuggerBackend::onNewConnection()
{
    m_socket = m_server->nextPendingConnection();
    m_server->close();
    QObject::connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                     this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));
    QObject::connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                     this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    QObject::connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    // the handshake is initiated by the debugger side, so wait for it
    m_state = HandshakingState;
}

void QScriptRemoteTargetDebuggerBackend::onReadyRead()
{
    switch (m_state) {
    case UnconnectedState:
        Q_ASSERT(0);
        break;

    case HandshakingState: {
        QByteArray handshakeData("QtScriptDebug-Handshake");
        if (m_socket->bytesAvailable() == handshakeData.size()) {
            QByteArray ba = m_socket->read(handshakeData.size());
            if (ba == handshakeData) {
#ifdef DEBUGGERENGINE_DEBUG
                qDebug() << "sending handshake reply (" << handshakeData.size() << "bytes )";
#endif
                m_socket->write(handshakeData);
                // handshaking complete
                // ### a way to specify if a break should be triggered immediately,
                // or only if an uncaught exception is triggered
                interruptEvaluation();
                m_state = ConnectedState;
                emit connected();
            } else {
//                d->error = QScriptDebuggerEngine::HandshakeError;
//                d->errorString = QString::fromLatin1("Incorrect handshake data received");
                m_state = UnconnectedState;
                emit error(QScriptDebuggerEngine::HandshakeError);
                m_socket->close();
            }
        }
    }   break;

    case ConnectedState: {
#ifdef DEBUGGERENGINE_DEBUG
        qDebug() << "received data. bytesAvailable:" << m_socket->bytesAvailable();
#endif
        QDataStream in(m_socket);
        in.setVersion(QDataStream::Qt_4_5);
        if (m_blockSize == 0) {
            if (m_socket->bytesAvailable() < (int)sizeof(quint32))
                return;
            in >> m_blockSize;
#ifdef DEBUGGERENGINE_DEBUG
            qDebug() << "  blockSize:" << m_blockSize;
#endif
        }
        if (m_socket->bytesAvailable() < m_blockSize)
            return;

#ifdef DEBUGGERENGINE_DEBUG
        qDebug() << "deserializing command";
#endif
        int wasAvailable = m_socket->bytesAvailable();
        qint32 id;
        in >> id;
        QScriptDebuggerCommand command(QScriptDebuggerCommand::None);
        in >> command;
        Q_ASSERT(m_socket->bytesAvailable() == wasAvailable - m_blockSize);

#ifdef DEBUGGERENGINE_DEBUG
        qDebug("executing command (id=%d, type=%d)", id, command.type());
#endif
        QScriptDebuggerResponse response = commandExecutor()->execute(this, command);

#ifdef DEBUGGERENGINE_DEBUG
        qDebug() << "serializing response";
#endif
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_5);
        out << (quint32)0; // reserve 4 bytes for block size
        out << (quint8)1;  // type = command response
        out << id;
        out << response;
        out.device()->seek(0);
        out << (quint32)(block.size() - sizeof(quint32));
#ifdef DEBUGGERENGINE_DEBUG
        qDebug() << "writing response (" << block.size() << "bytes )" << block.toHex();
#endif
        m_socket->write(block);
        m_blockSize = 0;

#ifdef DEBUGGERENGINE_DEBUG
        qDebug() << "bytes available is now" << m_socket->bytesAvailable();
#endif
        if (m_socket->bytesAvailable() != 0)
            QMetaObject::invokeMethod(this, "onReadyRead", Qt::QueuedConnection);
    }   break;

    }
}

/*!
  \reimp
*/
void QScriptRemoteTargetDebuggerBackend::event(const QScriptDebuggerEvent &event)
{
    if (m_state != ConnectedState)
        return;
    if (m_eventLoopPool.isEmpty())
        m_eventLoopPool.append(new QEventLoop());
    QEventLoop *eventLoop = m_eventLoopPool.takeFirst();
    Q_ASSERT(!eventLoop->isRunning());
    m_eventLoopStack.prepend(eventLoop);

#ifdef DEBUGGERENGINE_DEBUG
    qDebug() << "serializing event of type" << event.type();
#endif
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_5);
    out << (quint32)0; // reserve 4 bytes for block size
    out << (quint8)0;  // type = event
    out << event;
    out.device()->seek(0);
    out << (quint32)(block.size() - sizeof(quint32));

#ifdef DEBUGGERENGINE_DEBUG
    qDebug() << "writing event (" << block.size() << " bytes )";
#endif
    m_socket->write(block);

    // run an event loop until the debugger triggers a resume
#ifdef DEBUGGERENGINE_DEBUG
    qDebug("entering event loop");
#endif
    eventLoop->exec();
#ifdef DEBUGGERENGINE_DEBUG
    qDebug("returned from event loop");
#endif

    if (!m_eventLoopStack.isEmpty()) {
        // the event loop was quit directly (i.e. not via resume())
        m_eventLoopStack.takeFirst();
    }
    m_eventLoopPool.append(eventLoop);
    doPendingEvaluate(/*postEvent=*/false);
}

/*!
  \reimp
*/
void QScriptRemoteTargetDebuggerBackend::resume()
{
    // quitting the event loops will cause event() to return (see above)
    while (!m_eventLoopStack.isEmpty()) {
        QEventLoop *eventLoop = m_eventLoopStack.takeFirst();
        if (eventLoop->isRunning())
            eventLoop->quit();
    }
}

/*!
  Constructs a new QScriptDebuggerEngine object with the given \a
  parent.
*/
QScriptDebuggerEngine::QScriptDebuggerEngine(QObject *parent)
    : QObject(parent), m_backend(0)
{
}

/*!
  Destroys this QScriptDebuggerEngine.
*/
QScriptDebuggerEngine::~QScriptDebuggerEngine()
{
    delete m_backend;
}

/*!
  Sets the \a target engine that this debugger engine will manage.
*/
void QScriptDebuggerEngine::setTarget(QScriptEngine *target)
{
    if (m_backend) {
        m_backend->detach();
    } else {
        m_backend = new QScriptRemoteTargetDebuggerBackend();
        QObject::connect(m_backend, SIGNAL(connected()),
                         this, SIGNAL(connected()));
        QObject::connect(m_backend, SIGNAL(disconnected()),
                         this, SIGNAL(disconnected()));
        QObject::connect(m_backend, SIGNAL(error(QScriptDebuggerEngine::Error)),
                         this, SIGNAL(error(QScriptDebuggerEngine::Error)));
    }
    m_backend->attachTo(target);
}

/*!
  Returns the script engine that this debugger engine manages a connection to,
  or 0 if no engine has been set.
*/
QScriptEngine *QScriptDebuggerEngine::target() const
{
    if (!m_backend)
        return 0;
    return m_backend->engine();
}

/*!
  Attempts to make a connection to the given \a address on the given
  \a port.

  The connected() signal is emitted when the connection has been
  established.

  \sa disconnectFromDebugger(), listen()
*/
void QScriptDebuggerEngine::connectToDebugger(const QHostAddress &address, quint16 port)
{
    if (!m_backend) {
        qWarning("QScriptDebuggerEngine::connectToDebugger(): no engine has been set (call setTarget() first)");
        return;
    }
    m_backend->connectToDebugger(address, port);
}

/*!
  Attempts to close the connection.

  The disconnected() signal is emitted when the connection has been
  closed.

  \sa connectToDebugger()
*/
void QScriptDebuggerEngine::disconnectFromDebugger()
{
    if (m_backend)
        m_backend->disconnectFromDebugger();
}

/*!
  Listens for an incoming connection on the given \a address and \a
  port.

  Returns true on success; otherwise returns false.

  The connected() signal is emitted when a connection has been
  established.

  \sa connectToDebugger()
*/
bool QScriptDebuggerEngine::listen(const QHostAddress &address, quint16 port)
{
    if (!m_backend) {
        qWarning("QScriptDebuggerEngine::listen(): no script engine has been set (call setTarget() first)");
        return false;
    }
    return m_backend->listen(address, port);
}

#include "qscriptdebuggerengine.moc"
