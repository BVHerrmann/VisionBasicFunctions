#include "loggeroptionpanel.h"


LoggerOptionPanel::LoggerOptionPanel(LoggerModel *model, QWidget *parent) : OptionPanel(parent)
{
	_model = model;

	setupUi();
}

void LoggerOptionPanel::setupUi()
{
	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	layout->addSpacing(40);

	QPushButton* exportButton = new QPushButton(tr("Export..."));
	connect(exportButton, &QPushButton::clicked, this, &LoggerOptionPanel::exportToFile);
	layout->addWidget(exportButton);

	layout->addStretch();
}

void LoggerOptionPanel::exportToFile()
{
	QSettings settings;

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Log File"), settings.value("Logger/LastDir").toString() + "Inspector-" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss.log"), tr("Log Files (*.log)"));

	if (fileName.isNull())
		return;

	// store last folder
	QFileInfo info(fileName);
	settings.setValue("Logger/LastDir", info.dir().canonicalPath() + "/");

	// save log
	_model->saveAs(fileName);
}
