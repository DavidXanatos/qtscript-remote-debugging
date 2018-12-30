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

#include <QtScript>
#include <qscriptdebuggerengine.h>

void qScriptDebugRegisterMetaTypes();

class Runner : public QObject
{
    Q_OBJECT
public:
    Runner(const QHostAddress &addr, quint16 port, bool connect, QObject *parent = 0);
private slots:
    void onConnected();
    void onDisconnected();
private:
    QScriptEngine *m_scriptEngine;
    QScriptDebuggerEngine *m_debuggerEngine;
};

Runner::Runner(const QHostAddress &addr, quint16 port, bool connect, QObject *parent)
    : QObject(parent)
{
    m_scriptEngine = new QScriptEngine(this);
    m_debuggerEngine = new QScriptDebuggerEngine(this);
    QObject::connect(m_debuggerEngine, SIGNAL(connected()), this, SLOT(onConnected()), Qt::QueuedConnection);
    QObject::connect(m_debuggerEngine, SIGNAL(disconnected()), this, SLOT(onDisconnected()), Qt::QueuedConnection);
    m_debuggerEngine->setTarget(m_scriptEngine);
    if (connect) {
        qDebug("attempting to connect to debugger at %s:%d", qPrintable(addr.toString()), port);
        m_debuggerEngine->connectToDebugger(addr, port);
    } else {
        if (m_debuggerEngine->listen(addr, port))
            qDebug("waiting for debugger to connect at %s:%d", qPrintable(addr.toString()), port);
        else {
            qWarning("Failed to listen!");
            QCoreApplication::quit();
        }
    }
}

void Runner::onConnected()
{
    qDebug("connected to debugger");
    QStringList fileNames;
    QString path = QDir::currentPath();
    fileNames << (path + QDir::separator() + "foo.qs")
              << (path + QDir::separator() + "example.qs");
    for (int i = 0; i < fileNames.size(); ++i) {
        QString fn = fileNames.at(i);
        QFile file(fn);
        file.open(QIODevice::ReadOnly);
        QString program = file.readAll();
        qDebug("calling evaluate");
        m_scriptEngine->evaluate(program, fn);
        qDebug("evaluate done");
    }
    m_debuggerEngine->disconnectFromDebugger();
}

void Runner::onDisconnected()
{
    qDebug("disconnected");
    QCoreApplication::quit();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QHostAddress addr(QHostAddress::LocalHost);
    quint16 port = 2000;
    bool connect = false;
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        arg = arg.trimmed();
        if(arg.startsWith("--")) {
            QString opt;
            QString val;
            int split = arg.indexOf("=");
            if(split > 0) {
                opt = arg.mid(2).left(split-2);
                val = arg.mid(split + 1).trimmed();
            } else {
                opt = arg.mid(2);
            }
            if (opt == QLatin1String("address"))
                addr.setAddress(val);
            else if (opt == QLatin1String("port"))
                port = val.toUShort();
            else if (opt == QLatin1String("connect"))
                connect = true;
            else if (opt == QLatin1String("help")) {
                fprintf(stdout, "Usage: debuggee --address=ADDR --port=NUM [--connect]\n");
                return(0);
            }
        }
    }

    qScriptDebugRegisterMetaTypes();
    Runner runner(addr, port, connect);
    return app.exec();
}

#include "main.moc"
