#include "audittrailmodel.h"

#include <QtCore>
#include <QtGui>


AuditTrailModel::AuditTrailModel(QObject *parent, QSqlDatabase db) : QSqlTableModel(parent, db)
{
    setTable("audit_trail");
    
    setHeaderData(0, Qt::Horizontal, "ID");
    setHeaderData(1, Qt::Horizontal, tr("Date & Time"));
    setHeaderData(2, Qt::Horizontal, tr("User"));
    setHeaderData(3, Qt::Horizontal, tr("Message"));
    
    setSort(0, Qt::DescendingOrder);
}

AuditTrailModel::~AuditTrailModel()
{

}

QVariant AuditTrailModel::data(const QModelIndex &idx, int role) const
{
    if (role == Qt::DisplayRole && idx.column() == 1) { // start
        QDateTime ts = QDateTime::fromString(QSqlTableModel::data(idx, role).toString(), Qt::ISODateWithMs);
        return QLocale::system().toString(ts.date(), QLocale::ShortFormat) + " " + ts.time().toString(Qt::ISODateWithMs);
    }
    
    return QSqlTableModel::data(idx, role);
}
