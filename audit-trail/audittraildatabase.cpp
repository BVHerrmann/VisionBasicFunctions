#include "audittraildatabase.h"

#include <QtCore>
#include <QtGui>

#include <audittrail.h>
#include <colors.h>

#include "audittrailmodel.h"


AuditTrailDatabase::AuditTrailDatabase(QObject *parent) : QObject(parent)
{
    _database = QSqlDatabase::database();
    if (!_database.open()) {
        qCritical() << "Failed to open Audit Trail Database:" << _database.lastError();
    }
    
    createAuditTrailTable();
    cleanupDatabase();
}

AuditTrailDatabase::~AuditTrailDatabase()
{
    
}

QString AuditTrailDatabase::databasePath() const
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    if (!dir.exists()) {
        bool result = dir.mkpath(dir.path());
        if (!result) {
            qWarning() << "Failed to create directory at:" << dir.path();
        }
    }
    
    return dir.filePath("audit_trail.sqlite");
}

AuditTrailModel *AuditTrailDatabase::tableModel()
{
    return new AuditTrailModel(this, _database);
}

void AuditTrailDatabase::log(const QDateTime &timestamp, const QString &username, const QString &message)
{
    QSqlQuery query;
    query.prepare("INSERT INTO audit_trail (timestamp, username, message) VALUES (:timestamp, :username, :message)");
    query.bindValue(":timestamp", timestamp);
    query.bindValue(":username", username);
    query.bindValue(":message", message);

    if (!query.exec()) {
        qWarning() << "Failed to store alarm:" << query.lastError();
    }
}

void AuditTrailDatabase::saveAs(QString &fileName)
{
    assert(QThread::currentThread() == qApp->thread());
    
    QFile csvFile(fileName);
    if (!csvFile.open(QIODevice::Append | QIODevice::Text)) {
        qCritical() << "Failed to open log file:" << csvFile.fileName();
    } else {
        AuditTrail::message("Audit Trail exported");
        
        auto model = tableModel();
        model->select();
        
        for (int c=0; c < model->columnCount(); ++c) {
            csvFile.write(model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString().toUtf8());
            if (c == model->columnCount() - 1) {
                csvFile.write("\n");
            } else {
                csvFile.write(",");
            }
        }
        
        for (int r=0; r < model->rowCount(); ++r) {
            for (int c=0; c < model->columnCount(); ++c) {
                csvFile.write(model->data(model->index(r,c), Qt::DisplayRole).toString().toUtf8());
                if (c == model->columnCount() - 1) {
                    csvFile.write("\n");
                } else {
                    csvFile.write(",");
                }
            }
        }
        
        csvFile.close();
    }
}

void AuditTrailDatabase::createAuditTrailTable()
{
    QSqlQuery query("CREATE TABLE IF NOT EXISTS audit_trail (id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp DATETIME NOT NULL, username TEXT, message TEXTNOT NULL)", _database);
    if (!query.isActive()) {
        qWarning() << "Failed to create Table: " << query.lastError();
    }
}

void AuditTrailDatabase::cleanupDatabase()
{
    QSqlQuery query("VACUUM", _database);
    if (!query.isActive()) {
        qWarning() << "Failed to create Table: " << query.lastError();
    }
}
