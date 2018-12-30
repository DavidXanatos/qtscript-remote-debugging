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

#ifndef QSCRIPTREMOTETARGETDEBUGGER_H
#define QSCRIPTREMOTETARGETDEBUGGER_H

#include <QtCore/qobject.h>
#include <QtNetwork/qabstractsocket.h>
#include <QtNetwork/qhostaddress.h>

class QScriptDebugger;
class QScriptRemoteTargetDebuggerFrontend;
class QAction;
class QWidget;
class QMainWindow;
class QMenu;
class QToolBar;

class QScriptRemoteTargetDebugger
    : public QObject
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

    enum DebuggerWidget {
        ConsoleWidget,
        StackWidget,
        ScriptsWidget,
        LocalsWidget,
        CodeWidget,
        CodeFinderWidget,
        BreakpointsWidget,
        DebugOutputWidget,
        ErrorLogWidget
    };

    enum DebuggerAction {
        InterruptAction,
        ContinueAction,
        StepIntoAction,
        StepOverAction,
        StepOutAction,
        RunToCursorAction,
        RunToNewScriptAction,
        ToggleBreakpointAction,
        ClearDebugOutputAction,
        ClearErrorLogAction,
        ClearConsoleAction,
        FindInScriptAction,
        FindNextInScriptAction,
        FindPreviousInScriptAction,
        GoToLineAction
    };

    QScriptRemoteTargetDebugger(QObject *parent = 0);
    ~QScriptRemoteTargetDebugger();

    void attachTo(const QHostAddress &address, quint16 port);
    void detach();

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);

    bool autoShowStandardWindow() const;
    void setAutoShowStandardWindow(bool autoShow);

    QMainWindow *standardWindow() const;
    QToolBar *createStandardToolBar(QWidget *parent = 0);
    QMenu *createStandardMenu(QWidget *parent = 0);

    QWidget *widget(DebuggerWidget widget) const;
    QAction *action(DebuggerAction action) const;

Q_SIGNALS:
    void attached();
    void detached();
    void error(QScriptRemoteTargetDebugger::Error error);

    void evaluationSuspended();
    void evaluationResumed();

private Q_SLOTS:
    void showStandardWindow();

private:
    void createDebugger();
    void createFrontend();

private:
    QScriptRemoteTargetDebuggerFrontend *m_frontend;
    QScriptDebugger *m_debugger;
    bool m_autoShow;
    QMainWindow *m_standardWindow;

    Q_DISABLE_COPY(QScriptRemoteTargetDebugger)
};

#endif
