#ifndef AUDITTRAILDATABASE_H
#define AUDITTRAILDATABASE_H

#include <QtSql>

#include "audittrailmodel.h"

class AuditTrailDatabase : public QObject
{
    Q_OBJECT
public:
    explicit AuditTrailDatabase(QObject *parent = nullptr);
    ~AuditTrailDatabase();
    
    AuditTrailModel *tableModel();
    
    void saveAs(QString &fileName);
    
signals:
    
public slots:
    void log(const QDateTime &timestamp, const QString &username, const QString &message);
    
private:
    QSqlDatabase _database;
    
    QString databasePath() const;
    
    void createAuditTrailTable();
    void cleanupDatabase();
};

#endif // AUDITTRAILDATABASE_H
