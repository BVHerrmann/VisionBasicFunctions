#include "audittrailoptionpanel.h"


AuditTrailOptionPanel::AuditTrailOptionPanel(AuditTrailDatabase *db, QWidget *parent) : OptionPanel(parent)
{
	_db = db;

	setupUi();
}

void AuditTrailOptionPanel::setupUi()
{
	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	layout->addSpacing(40);

	QPushButton* exportButton = new QPushButton(tr("Export..."));
	connect(exportButton, &QPushButton::clicked, this, &AuditTrailOptionPanel::exportToFile);
	layout->addWidget(exportButton);

	layout->addStretch();
}

void AuditTrailOptionPanel::exportToFile()
{
	QSettings settings;

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Audit Trail"), settings.value("AuditTrail/LastDir").toString() + "AuditTrail-" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss.csv"), tr("CSV Files (*.csv)"));

	if (fileName.isNull())
		return;

	// store last folder
	QFileInfo info(fileName);
	settings.setValue("AuditTrail/LastDir", info.dir().canonicalPath() + "/");

	// save log
	_db->saveAs(fileName);
}
