#include "loggeritem.h"

#include <QtCore>
#include <QtGui>

#include <colors.h>

#include "loggermodel.h"


LoggerItem::LoggerItem(QtMsgType type, QString msg, QDateTime timestamp, QString source, QObject *parent) :
    QObject(parent)
{
    _timestamp = timestamp;
    _type = type;
    _msg = msg;
    _threadName = source;
}

QVariant LoggerItem::data(int column, int role) const
{
    switch (role) {
    case Qt::BackgroundRole:
        {
            switch(_type) {
            case QtWarningMsg:
                return HMIColor::WarningLowTranslucent;
                break;
            case QtCriticalMsg:
                return HMIColor::WarningHighTranslucent;
                break;
            case QtFatalMsg:
                return HMIColor::AlarmTranslucent;
                break;
            default:
                return QVariant();
            }
        }
    case Qt::DisplayRole:
        {
            switch (column) {
            case 0:
                return _type;
            case 1:
                return _timestamp.toString("dd.MM.yy hh:mm:ss.zzz");
            case 2:
                return _threadName;
            case 3:
                return _msg;
            default:
                return QVariant();
            }
        }
    default:
        return QVariant();
    }
}

QString LoggerItem::toString() const
{
    QString complete = "[" + _timestamp.toString("yyyy-MM-ddThh:mm:ss.zzz") + "/" + _threadName + "] ";
    
    switch (_type) {
        case QtDebugMsg:
            complete += _msg + "\n";
            break;
        case QtWarningMsg:
            complete += "WARNING: " + _msg + "\n";
            break;
        case QtCriticalMsg:
            complete += "CRITICAL: " + _msg + "\n";
            break;
        case QtFatalMsg:
            complete += "FATAL: " + _msg + "\n";
            break;
        default:
            break;
    }
    
    return complete;
}
