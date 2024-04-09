#ifndef AUDITTRAILMODEL_H
#define AUDITTRAILMODEL_H

#include <QtSql>
#include <QList>
#include <QMutex>

class AuditTrailItem;


class AuditTrailModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit AuditTrailModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    virtual ~AuditTrailModel();
    
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const;
};

#endif // AUDITTRAILMODEL_H
