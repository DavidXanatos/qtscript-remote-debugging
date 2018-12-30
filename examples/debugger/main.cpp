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

#include <QtGui>
#include <QtScript>
#include <QtNetwork>
#include "qscriptremotetargetdebugger.h"

void qScriptDebugRegisterMetaTypes();

class Runner : public QObject
{
    Q_OBJECT
public:
    Runner(const QHostAddress &addr, quint16 port, bool listen, QObject *parent = 0);
private slots:
    void onAttached();
    void onDetached();
    void onError(QScriptRemoteTargetDebugger::Error error);
private:
    QScriptRemoteTargetDebugger *m_debugger;
};

Runner::Runner(const QHostAddress &addr, quint16 port, bool listen, QObject *parent)
    : QObject(parent)
{
    m_debugger = new QScriptRemoteTargetDebugger(this);
    QObject::connect(m_debugger, SIGNAL(attached()), this, SLOT(onAttached()));
    QObject::connect(m_debugger, SIGNAL(detached()), this, SLOT(onDetached()));
    QObject::connect(m_debugger, SIGNAL(error(QScriptRemoteTargetDebugger::Error)),
                     this, SLOT(onError(QScriptRemoteTargetDebugger::Error)));
    if (listen) {
        if (m_debugger->listen(addr, port))
            qDebug("listening for debuggee connection at %s:%d", qPrintable(addr.toString()), port);
        else {
            qWarning("Failed to listen!");
            QCoreApplication::quit();
        }
    } else {
        qDebug("attaching to %s:%d", qPrintable(addr.toString()), port);
        m_debugger->attachTo(addr, port);
    }
}

void Runner::onAttached()
{
}

void Runner::onDetached()
{
    QApplication::quit();
}

void Runner::onError(QScriptRemoteTargetDebugger::Error /*error*/)
{
    QApplication::quit();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QHostAddress addr(QHostAddress::LocalHost);
    quint16 port = 2000;
    bool listen = false;
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
            else if (opt == QLatin1String("listen"))
                listen = true;
            else if (opt == QLatin1String("help")) {
                fprintf(stdout, "Usage: remote --address=ADDR --port=NUM [--listen]\n");
                return(-1);
            }
        }
    }

    qScriptDebugRegisterMetaTypes();
    Runner runner(addr, port, listen);
    return app.exec();
}

#include "main.moc"
