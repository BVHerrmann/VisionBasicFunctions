#pragma once
#include <qwidget.h>


class QFormLayout;
class QGroupBox;
class ProcedureOutputControlParameter;
class ProcedureInputControlParameter;
class ProcedureIOControlParameter;
class BDevEngineImageOverview;
class ControlParameterWidget :	public QWidget
{
public:
	ControlParameterWidget(BDevEngineImageOverview *pBDevEngineImageOverview);
	~ControlParameterWidget();
	BDevEngineImageOverview *GetBDevEngineImageOverview() { return m_BDevEngineImageOverview; }
	void setupUi();
	void SetupControlParameterWidget();
	void ViewOutputControlParameter(QFormLayout *pLayout, QList<ProcedureOutputControlParameter> *ListControlParameter);
	void ViewInputControlParameter(QFormLayout *pLayout, QList<ProcedureInputControlParameter> *ListControlParameter);
	void ViewNewOutputControlParameter();
	void ViewNewInputControlParameter();
private:
	BDevEngineImageOverview *m_BDevEngineImageOverview;
	QFormLayout *m_InLayout;
	QFormLayout *m_OutLayout;
	QGroupBox *m_GroupBoxInputParameter;
	QGroupBox *m_GroupBoxOutputParameter;
};

