#include "loggermodel.h"

#include <QtCore>
#include <QtGui>

#include <audittrail.h>

#include "loggerdefines.h"
#include "loggeritem.h"


LoggerModel::LoggerModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    _removeItems = true;
    
    // initialize column headers
    _columns << tr("Level") << tr("Date & Time") << tr("Sender") << tr("Message");

    // read settings
    loadPreferences();
}

LoggerModel::~LoggerModel()
{
    while (!_items.isEmpty()) {
        delete _items.takeFirst();
    }
}

void LoggerModel::loadPreferences()
{
    QSettings settings;
    _maxResults = settings.value(kMaxResults, kDefaultMaxResults).toUInt();
}

void LoggerModel::log(LoggerItem *item)
{
    assert(QThread::currentThread() == qApp->thread());
    
    int pos = _items.count();
    beginInsertRows(QModelIndex(), pos, pos);
    _items << item;
    endInsertRows();
    
    if (_removeItems && pos >= _maxResults) {
        beginRemoveRows(QModelIndex(), 0, pos - _maxResults);
        while (_items.count() > _maxResults) {
            delete _items.takeFirst();
        }
        endRemoveRows();
    }
}

QVariant LoggerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            return _columns.value(section);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant LoggerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= _items.count())
        return QVariant();

    LoggerItem *item = _items.value(index.row());
    if (item)
        return item->data(index.column(), role);

    return QVariant();
}

int LoggerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _items.count();
}

int LoggerModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _columns.count();
}

void LoggerModel::saveAs(QString &fileName) const
{
    assert(QThread::currentThread() == qApp->thread());
    
    QFile logFile(fileName);
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qCritical() << "Failed to open log file:" << logFile.fileName();
    } else {
        for (const LoggerItem *item : _items) {
            logFile.write(item->toString().toUtf8());
        }
        
        logFile.close();
        
        AuditTrail::message("Log files exported");
    }
}
