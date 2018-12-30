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

#include <QtScript/qscriptcontextinfo.h>
#include <private/qscriptdebuggercommand_p.h>
#include <private/qscriptdebuggerevent_p.h>
#include <private/qscriptdebuggerresponse_p.h>
#include <private/qscriptdebuggerbackend_p.h>
#include <private/qscriptdebuggerobjectsnapshotdelta_p.h>

Q_DECLARE_METATYPE(QScriptDebuggerCommand)
Q_DECLARE_METATYPE(QScriptDebuggerResponse)
Q_DECLARE_METATYPE(QScriptDebuggerEvent)
Q_DECLARE_METATYPE(QScriptContextInfo)
Q_DECLARE_METATYPE(QScriptContextInfoList)
Q_DECLARE_METATYPE(QScriptDebuggerValue)
Q_DECLARE_METATYPE(QScriptDebuggerValueList)
Q_DECLARE_METATYPE(QScriptValue::PropertyFlags)
Q_DECLARE_METATYPE(QScriptBreakpointData)
Q_DECLARE_METATYPE(QScriptBreakpointMap)
Q_DECLARE_METATYPE(QScriptScriptData)
Q_DECLARE_METATYPE(QScriptScriptMap)
Q_DECLARE_METATYPE(QScriptScriptsDelta)
Q_DECLARE_METATYPE(QScriptDebuggerValueProperty)
Q_DECLARE_METATYPE(QScriptDebuggerValuePropertyList)
Q_DECLARE_METATYPE(QScriptDebuggerObjectSnapshotDelta)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<qint64>)

void qScriptDebugRegisterMetaTypes()
{
    qMetaTypeId<QScriptDebuggerCommand>();
    qMetaTypeId<QScriptDebuggerResponse>();
    qMetaTypeId<QScriptDebuggerEvent>();
    qMetaTypeId<QScriptContextInfo>();
    qMetaTypeId<QScriptContextInfoList>();
    qMetaTypeId<QScriptDebuggerValue>();
    qMetaTypeId<QScriptDebuggerValueList>();
    qMetaTypeId<QScriptValue::PropertyFlags>();
    qMetaTypeId<QScriptBreakpointData>();
    qMetaTypeId<QScriptBreakpointMap>();
    qMetaTypeId<QScriptScriptData>();
    qMetaTypeId<QScriptScriptMap>();
    qMetaTypeId<QScriptScriptsDelta>();
    qMetaTypeId<QScriptDebuggerValueProperty>();
    qMetaTypeId<QScriptDebuggerValuePropertyList>();
    qMetaTypeId<QScriptDebuggerObjectSnapshotDelta>();
    qMetaTypeId<QList<int> >();
    qMetaTypeId<QList<qint64> >();

    qRegisterMetaTypeStreamOperators<QScriptDebuggerCommand>("QScriptDebuggerCommand");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerResponse>("QScriptDebuggerResponse");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerEvent>("QScriptDebuggerEvent");
    qRegisterMetaTypeStreamOperators<QScriptContextInfo>("QScriptContextInfo");
    qRegisterMetaTypeStreamOperators<QScriptContextInfoList>("QScriptContextInfoList");
    qRegisterMetaTypeStreamOperators<QScriptBreakpointData>("QScriptBreakpointData");
    qRegisterMetaTypeStreamOperators<QScriptBreakpointMap>("QScriptBreakpointMap");
    qRegisterMetaTypeStreamOperators<QScriptScriptData>("QScriptScriptData");
    qRegisterMetaTypeStreamOperators<QScriptScriptMap>("QScriptScriptMap");
    qRegisterMetaTypeStreamOperators<QScriptScriptsDelta>("QScriptScriptsDelta");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerValue>("QScriptDebuggerValue");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerValueList>("QScriptDebuggerValueList");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerValueProperty>("QScriptDebuggerValueProperty");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerValuePropertyList>("QScriptDebuggerValuePropertyList");
    qRegisterMetaTypeStreamOperators<QScriptDebuggerObjectSnapshotDelta>("QScriptDebuggerObjectSnapshotDelta");
    qRegisterMetaTypeStreamOperators<QList<int> >("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<qint64> >("QList<qint64>");
}
