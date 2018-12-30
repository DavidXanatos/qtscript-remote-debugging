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

#include "qscriptremotetargetdebugger.h"
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtGui>

#include <private/qscriptdebuggerfrontend_p.h>
#include <private/qscriptdebugger_p.h>
#include <private/qscriptdebuggercommand_p.h>
#include <private/qscriptdebuggerevent_p.h>
#include <private/qscriptdebuggerresponse_p.h>
#include <private/qscriptdebuggerstandardwidgetfactory_p.h>

// #define DEBUG_DEBUGGER

class QScriptRemoteTargetDebuggerFrontend
    : public QObject, public QScriptDebuggerFrontend
{
    Q_OBJECT
public:
    enum State {
        UnattachedState,
        ConnectingState,
        HandshakingState,
        AttachedState,
        DetachingState
    };

    QScriptRemoteTargetDebuggerFrontend();
    ~QScriptRemoteTargetDebuggerFrontend();

    void attachTo(const QHostAddress &address, quint16 port);
    void detach();
    bool listen(const QHostAddress &address, quint16 port);

Q_SIGNALS:
    void attached();
    void detached();
    void error(QScriptRemoteTargetDebugger::Error error);

protected:
    void processCommand(int id, const QScriptDebuggerCommand &command);

private Q_SLOTS:
    void onSocketStateChanged(QAbstractSocket::SocketState);
    void onSocketError(QAbstractSocket::SocketError);
    void onReadyRead();
    void onNewConnection();

private:
    void initiateHandshake();

private:
    State m_state;
    QTcpServer *m_server;
    QTcpSocket *m_socket;
    int m_blockSize;

    Q_DISABLE_COPY(QScriptRemoteTargetDebuggerFrontend)
};

QScriptRemoteTargetDebuggerFrontend::QScriptRemoteTargetDebuggerFrontend()
    : m_state(UnattachedState), m_server(0), m_socket(0), m_blockSize(0)
{
}

QScriptRemoteTargetDebuggerFrontend::~QScriptRemoteTargetDebuggerFrontend()
{
}

void QScriptRemoteTargetDebuggerFrontend::attachTo(const QHostAddress &address, quint16 port)
{
    Q_ASSERT(m_state == UnattachedState);
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

void QScriptRemoteTargetDebuggerFrontend::detach()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "implement me");
}

bool QScriptRemoteTargetDebuggerFrontend::listen(const QHostAddress &address, quint16 port)
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

void QScriptRemoteTargetDebuggerFrontend::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        m_state = UnattachedState;
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
        m_state = ConnectingState;
        break;
    case QAbstractSocket::ConnectedState:
        initiateHandshake();
        break;
    case QAbstractSocket::BoundState:
        break;
    case QAbstractSocket::ClosingState:
        emit detached();
        break;
    case QAbstractSocket::ListeningState:
        break;
    }
}

void QScriptRemoteTargetDebuggerFrontend::onSocketError(QAbstractSocket::SocketError err)
{
    if (err != QAbstractSocket::RemoteHostClosedError) {
        qDebug("%s", qPrintable(m_socket->errorString()));
        emit error(QScriptRemoteTargetDebugger::SocketError);
    }
}

void QScriptRemoteTargetDebuggerFrontend::onReadyRead()
{
    switch (m_state) {
    case UnattachedState:
    case ConnectingState:
    case DetachingState:
        Q_ASSERT(0);
        break;

    case HandshakingState: {
        QByteArray handshakeData("QtScriptDebug-Handshake");
        if (m_socket->bytesAvailable() >= handshakeData.size()) {
            QByteArray ba = m_socket->read(handshakeData.size());
            if (ba == handshakeData) {
#ifdef DEBUG_DEBUGGER
                qDebug("handshake ok!");
#endif
                m_state = AttachedState;
                emit attached();
                if (m_socket->bytesAvailable() > 0)
                    QMetaObject::invokeMethod(this, "onReadyRead", Qt::QueuedConnection);
            } else {
//                d->error = HandshakeError;
//                d->errorString = QString::fromLatin1("Incorrect handshake data received");
                m_state = DetachingState;
                emit error(QScriptRemoteTargetDebugger::HandshakeError);
                m_socket->close();
            }
        }
    }   break;

    case AttachedState: {
#ifdef DEBUG_DEBUGGER
        qDebug() << "got something! bytes available:" << m_socket->bytesAvailable();
#endif
        QDataStream in(m_socket);
        in.setVersion(QDataStream::Qt_4_5);
        if (m_blockSize == 0) {
            if (m_socket->bytesAvailable() < (int)sizeof(quint32))
                return;
            in >> m_blockSize;
#ifdef DEBUG_DEBUGGER
            qDebug() << "blockSize:" << m_blockSize;
#endif
        }
        if (m_socket->bytesAvailable() < m_blockSize) {
#ifdef DEBUG_DEBUGGER
            qDebug("waiting for %lld more bytes...", m_blockSize - m_socket->bytesAvailable());
#endif
            return;
        }

        int wasAvailable = m_socket->bytesAvailable();
        quint8 type;
        in >> type;
        if (type == 0) {
            // event
#ifdef DEBUG_DEBUGGER
            qDebug("deserializing event");
#endif
            QScriptDebuggerEvent event(QScriptDebuggerEvent::None);
            in >> event;
#ifdef DEBUG_DEBUGGER
            qDebug("notifying event of type %d", event.type());
#endif
            bool handled = notifyEvent(event);
            if (handled) {
                scheduleCommand(QScriptDebuggerCommand::resumeCommand(),
                                /*responseHandler=*/0);
            }
        } else {
            // command response
#ifdef DEBUG_DEBUGGER
            qDebug("deserializing command response");
#endif
            qint32 id;
            in >> id;
            QScriptDebuggerResponse response;
            in >> response;
#ifdef DEBUG_DEBUGGER
            qDebug("notifying command %d finished", id);
#endif
            notifyCommandFinished((int)id, response);
        }
        Q_ASSERT(m_socket->bytesAvailable() == wasAvailable - m_blockSize);
        m_blockSize = 0;
#ifdef DEBUG_DEBUGGER
        qDebug("bytes available is now %lld", m_socket->bytesAvailable());
#endif
        if (m_socket->bytesAvailable() != 0)
            QMetaObject::invokeMethod(this, "onReadyRead", Qt::QueuedConnection);
    }   break;
    }
}

void QScriptRemoteTargetDebuggerFrontend::onNewConnection()
{
    qDebug("received connection");
    m_socket = m_server->nextPendingConnection();
    m_server->close();
    QObject::connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                     this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));
    QObject::connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                     this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    QObject::connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    initiateHandshake();
}

/*!
  \reimp
*/
void QScriptRemoteTargetDebuggerFrontend::processCommand(int id, const QScriptDebuggerCommand &command)
{
    Q_ASSERT(m_state == AttachedState);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_5);
    out << (quint32)0; // reserve 4 bytes for block size
    out << (qint32)id;
    out << command;
    out.device()->seek(0);
    out << (quint32)(block.size() - sizeof(quint32));
#ifdef DEBUG_DEBUGGER
    qDebug("writing command (id=%d, %d bytes)", id, block.size());
#endif
    m_socket->write(block);
}

void QScriptRemoteTargetDebuggerFrontend::initiateHandshake()
{
    m_state = HandshakingState;
    QByteArray handshakeData("QtScriptDebug-Handshake");
#ifdef DEBUG_DEBUGGER
    qDebug("writing handshake data");
#endif
    m_socket->write(handshakeData);
}

QScriptRemoteTargetDebugger::QScriptRemoteTargetDebugger(QObject *parent)
    : QObject(parent), m_frontend(0), m_debugger(0), m_autoShow(true),
      m_standardWindow(0)
{
}

QScriptRemoteTargetDebugger::~QScriptRemoteTargetDebugger()
{
    delete m_frontend;
    delete m_debugger;
}

void QScriptRemoteTargetDebugger::attachTo(const QHostAddress &address, quint16 port)
{
    createFrontend();
    m_frontend->attachTo(address, port);
}

void QScriptRemoteTargetDebugger::detach()
{
    if (m_frontend)
        m_frontend->detach();
}

bool QScriptRemoteTargetDebugger::listen(const QHostAddress &address, quint16 port)
{
    createFrontend();
    return m_frontend->listen(address, port);
}

void QScriptRemoteTargetDebugger::createDebugger()
{
    if (!m_debugger) {
        m_debugger = new QScriptDebugger();
        m_debugger->setWidgetFactory(new QScriptDebuggerStandardWidgetFactory(this));
        QObject::connect(m_debugger, SIGNAL(started()),
                         this, SIGNAL(evaluationResumed()));
        QObject::connect(m_debugger, SIGNAL(stopped()),
                         this, SIGNAL(evaluationSuspended()));
        if (m_autoShow) {
            QObject::connect(this, SIGNAL(evaluationSuspended()),
                             this, SLOT(showStandardWindow()));
        }
    }
}

void QScriptRemoteTargetDebugger::createFrontend()
{
    if (!m_frontend) {
        m_frontend = new QScriptRemoteTargetDebuggerFrontend();
        QObject::connect(m_frontend, SIGNAL(attached()),
                         this, SIGNAL(attached()), Qt::QueuedConnection);
        QObject::connect(m_frontend, SIGNAL(detached()),
                         this, SIGNAL(detached()), Qt::QueuedConnection);
        QObject::connect(m_frontend, SIGNAL(error(QScriptRemoteTargetDebugger::Error)),
                         this, SIGNAL(error(QScriptRemoteTargetDebugger::Error)));
        createDebugger();
        m_debugger->setFrontend(m_frontend);
    }
}

QMainWindow *QScriptRemoteTargetDebugger::standardWindow() const
{
    if (m_standardWindow)
        return m_standardWindow;
    if (!QApplication::instance())
        return 0;
    QScriptRemoteTargetDebugger *that = const_cast<QScriptRemoteTargetDebugger*>(this);

    QMainWindow *win = new QMainWindow();
    QDockWidget *scriptsDock = new QDockWidget(win);
    scriptsDock->setObjectName(QLatin1String("qtscriptdebugger_scriptsDockWidget"));
    scriptsDock->setWindowTitle(QObject::tr("Loaded Scripts"));
    scriptsDock->setWidget(widget(ScriptsWidget));
    win->addDockWidget(Qt::LeftDockWidgetArea, scriptsDock);

    QDockWidget *breakpointsDock = new QDockWidget(win);
    breakpointsDock->setObjectName(QLatin1String("qtscriptdebugger_breakpointsDockWidget"));
    breakpointsDock->setWindowTitle(QObject::tr("Breakpoints"));
    breakpointsDock->setWidget(widget(BreakpointsWidget));
    win->addDockWidget(Qt::LeftDockWidgetArea, breakpointsDock);

    QDockWidget *stackDock = new QDockWidget(win);
    stackDock->setObjectName(QLatin1String("qtscriptdebugger_stackDockWidget"));
    stackDock->setWindowTitle(QObject::tr("Stack"));
    stackDock->setWidget(widget(StackWidget));
    win->addDockWidget(Qt::RightDockWidgetArea, stackDock);

    QDockWidget *localsDock = new QDockWidget(win);
    localsDock->setObjectName(QLatin1String("qtscriptdebugger_localsDockWidget"));
    localsDock->setWindowTitle(QObject::tr("Locals"));
    localsDock->setWidget(widget(LocalsWidget));
    win->addDockWidget(Qt::RightDockWidgetArea, localsDock);

    QDockWidget *consoleDock = new QDockWidget(win);
    consoleDock->setObjectName(QLatin1String("qtscriptdebugger_consoleDockWidget"));
    consoleDock->setWindowTitle(QObject::tr("Console"));
    consoleDock->setWidget(widget(ConsoleWidget));
    win->addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    QDockWidget *debugOutputDock = new QDockWidget(win);
    debugOutputDock->setObjectName(QLatin1String("qtscriptdebugger_debugOutputDockWidget"));
    debugOutputDock->setWindowTitle(QObject::tr("Debug Output"));
    debugOutputDock->setWidget(widget(DebugOutputWidget));
    win->addDockWidget(Qt::BottomDockWidgetArea, debugOutputDock);

    QDockWidget *errorLogDock = new QDockWidget(win);
    errorLogDock->setObjectName(QLatin1String("qtscriptdebugger_errorLogDockWidget"));
    errorLogDock->setWindowTitle(QObject::tr("Error Log"));
    errorLogDock->setWidget(widget(ErrorLogWidget));
    win->addDockWidget(Qt::BottomDockWidgetArea, errorLogDock);

    win->tabifyDockWidget(errorLogDock, debugOutputDock);
    win->tabifyDockWidget(debugOutputDock, consoleDock);

    win->addToolBar(Qt::TopToolBarArea, that->createStandardToolBar());

#ifndef QT_NO_MENUBAR
    win->menuBar()->addMenu(that->createStandardMenu(win));

    QMenu *editMenu = win->menuBar()->addMenu(QObject::tr("Search"));
    editMenu->addAction(action(FindInScriptAction));
    editMenu->addAction(action(FindNextInScriptAction));
    editMenu->addAction(action(FindPreviousInScriptAction));
    editMenu->addSeparator();
    editMenu->addAction(action(GoToLineAction));

    QMenu *viewMenu = win->menuBar()->addMenu(QObject::tr("View"));
    viewMenu->addAction(scriptsDock->toggleViewAction());
    viewMenu->addAction(breakpointsDock->toggleViewAction());
    viewMenu->addAction(stackDock->toggleViewAction());
    viewMenu->addAction(localsDock->toggleViewAction());
    viewMenu->addAction(consoleDock->toggleViewAction());
    viewMenu->addAction(debugOutputDock->toggleViewAction());
    viewMenu->addAction(errorLogDock->toggleViewAction());
#endif

    QWidget *central = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(central);
    vbox->addWidget(widget(CodeWidget));
    vbox->addWidget(widget(CodeFinderWidget));
    widget(CodeFinderWidget)->hide();
    win->setCentralWidget(central);

    win->setWindowTitle(QObject::tr("Qt Script Debugger"));

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    QVariant geometry = settings.value(QLatin1String("Qt/scripttools/debugging/mainWindowGeometry"));
    if (geometry.isValid())
        win->restoreGeometry(geometry.toByteArray());
    QVariant state = settings.value(QLatin1String("Qt/scripttools/debugging/mainWindowState"));
    if (state.isValid())
        win->restoreState(state.toByteArray());

    const_cast<QScriptRemoteTargetDebugger*>(this)->m_standardWindow = win;
    return win;
}

void QScriptRemoteTargetDebugger::showStandardWindow()
{
    (void)standardWindow(); // ensure it's created
    m_standardWindow->show();
}

QWidget *QScriptRemoteTargetDebugger::widget(DebuggerWidget widget) const
{
    const_cast<QScriptRemoteTargetDebugger*>(this)->createDebugger();
    return m_debugger->widget(static_cast<QScriptDebugger::DebuggerWidget>(widget));
}

QAction *QScriptRemoteTargetDebugger::action(DebuggerAction action) const
{
    QScriptRemoteTargetDebugger *that = const_cast<QScriptRemoteTargetDebugger*>(this);
    that->createDebugger();
    return m_debugger->action(static_cast<QScriptDebugger::DebuggerAction>(action), that);
}

QToolBar *QScriptRemoteTargetDebugger::createStandardToolBar(QWidget *parent)
{
    createDebugger();
    return m_debugger->createStandardToolBar(parent, this);
}

QMenu *QScriptRemoteTargetDebugger::createStandardMenu(QWidget *parent)
{
    createDebugger();
    return m_debugger->createStandardMenu(parent, this);
}

bool QScriptRemoteTargetDebugger::autoShowStandardWindow() const
{
    return m_autoShow;
}

void QScriptRemoteTargetDebugger::setAutoShowStandardWindow(bool autoShow)
{
    if (autoShow == m_autoShow)
        return;
    if (autoShow) {
        QObject::connect(this, SIGNAL(evaluationSuspended()),
                         this, SLOT(showStandardWindow()));
    } else {
        QObject::disconnect(this, SIGNAL(evaluationSuspended()),
                            this, SLOT(showStandardWindow()));
    }
    m_autoShow = autoShow;
}

#include "qscriptremotetargetdebugger.moc"
