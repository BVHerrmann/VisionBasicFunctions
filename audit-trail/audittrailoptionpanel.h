#ifndef AUDITTRAILOPTIONPANEL_H
#define AUDITTRAILOPTIONPANEL_H

#include <QtWidgets>

#include <optionpanel.h>

#include "audittraildatabase.h"

class AuditTrailOptionPanel : public OptionPanel
{
    Q_OBJECT
public:
    explicit AuditTrailOptionPanel(AuditTrailDatabase *db, QWidget *parent = nullptr);

private slots:
	void exportToFile();

private:
	void setupUi();

	AuditTrailDatabase* _db;
};

#endif // AUDITTRAILOPTIONPANEL_H
