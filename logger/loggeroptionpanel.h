#ifndef LOGGEROPTIONPANEL_H
#define LOGGEROPTIONPANEL_H

#include <QtWidgets>

#include <optionpanel.h>

#include "loggermodel.h"

class LoggerOptionPanel : public OptionPanel
{
    Q_OBJECT
public:
    explicit LoggerOptionPanel(LoggerModel *model, QWidget *parent = nullptr);

private slots:
	void exportToFile();

private:
	void setupUi();

	LoggerModel* _model;
};

#endif // LOGGEROPTIONPANEL_H
