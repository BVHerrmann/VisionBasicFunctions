#include "ControlParameterWidget.h"
#include <QtGui>
#include <QtWidgets>
#include "BDEvEngineImageOverview.h"
#include "ImageData.h"
#include "ProcedureIOControlParameter.h"


ControlParameterWidget::ControlParameterWidget(BDevEngineImageOverview *pBDevEngineImageOverview) :	QWidget()
,m_InLayout(NULL)
,m_OutLayout(NULL)
,m_GroupBoxInputParameter(NULL)
,m_GroupBoxOutputParameter(NULL)
{
	m_BDevEngineImageOverview = pBDevEngineImageOverview;
	setupUi();
}


ControlParameterWidget::~ControlParameterWidget()
{

}


void ControlParameterWidget::setupUi()
{
	QGridLayout *layout = new QGridLayout();

	layout->setContentsMargins(9, 0, 9, 0);
	setLayout(layout);

	m_GroupBoxInputParameter = new QGroupBox(tr("Inputs"));
	QPalette pal_in = m_GroupBoxInputParameter->palette();
	pal_in.setColor(QPalette::Window, Qt::transparent);
	m_GroupBoxInputParameter->setPalette(pal_in);
	layout->addWidget(m_GroupBoxInputParameter,0,0);

	QScrollArea *in_area = new QScrollArea();
	in_area->setWidgetResizable(true);
	in_area->setFrameStyle(QFrame::Plain);
	m_GroupBoxInputParameter->setLayout(new QVBoxLayout());
	m_GroupBoxInputParameter->layout()->setContentsMargins(0, 0, 0, 0);
	m_GroupBoxInputParameter->layout()->addWidget(in_area);

	QWidget *in_widget = new QWidget();
	in_area->setWidget(in_widget);

	m_InLayout = new QFormLayout();
	m_InLayout->setLabelAlignment(Qt::AlignVCenter);
	m_InLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	m_InLayout->setVerticalSpacing(0);
	in_widget->setLayout(m_InLayout);

	m_GroupBoxOutputParameter = new QGroupBox(tr("Outputs"));
	QPalette pal_out = m_GroupBoxOutputParameter->palette();
	pal_out.setColor(QPalette::Window, Qt::transparent);
	m_GroupBoxOutputParameter->setPalette(pal_out);
	layout->addWidget(m_GroupBoxOutputParameter,0,1);

	QScrollArea *out_area = new QScrollArea();
	out_area->setWidgetResizable(true);
	out_area->setFrameStyle(QFrame::Plain);
	m_GroupBoxOutputParameter->setLayout(new QVBoxLayout());
	m_GroupBoxOutputParameter->layout()->setContentsMargins(0, 0, 0, 0);
	m_GroupBoxOutputParameter->layout()->addWidget(out_area);

	QWidget *out_widget = new QWidget();
	out_area->setWidget(out_widget);

	m_OutLayout = new QFormLayout();
	m_OutLayout->setLabelAlignment(Qt::AlignVCenter);
	m_OutLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	m_OutLayout->setVerticalSpacing(0);
	out_widget->setLayout(m_OutLayout);
}


void ControlParameterWidget::SetupControlParameterWidget()
{
	QString ProcedureName = GetBDevEngineImageOverview()->GetImageData()->GetProcedureName();
	ProcedureIOControlParameter *procedureIOControlParameter = GetBDevEngineImageOverview()->GetImageData()->GetIOControlParameter();
	if (procedureIOControlParameter)
	{
		ViewInputControlParameter(m_InLayout, &(procedureIOControlParameter->m_ListInputControlParameter));
		ViewOutputControlParameter(m_OutLayout, &(procedureIOControlParameter->m_ListOutputControlParameter));

		m_GroupBoxInputParameter->setTitle(tr("Inputs(%1)").arg(ProcedureName));
	}
}


void ControlParameterWidget::ViewNewOutputControlParameter()
{
	ProcedureIOControlParameter *procedureIOControlParameter = GetBDevEngineImageOverview()->GetImageData()->GetIOControlParameter();
	if(procedureIOControlParameter)
	   ViewOutputControlParameter(m_OutLayout, &(procedureIOControlParameter->m_ListOutputControlParameter));
}


void ControlParameterWidget::ViewNewInputControlParameter()
{
	ProcedureIOControlParameter *procedureIOControlParameter = GetBDevEngineImageOverview()->GetImageData()->GetIOControlParameter();
	if (procedureIOControlParameter)
	    ViewInputControlParameter(m_InLayout, &(procedureIOControlParameter->m_ListInputControlParameter));
}



void ControlParameterWidget::ViewInputControlParameter(QFormLayout *pLayout, QList<ProcedureInputControlParameter> *ListControlParameter)
{
	for (int i = 0; i < pLayout->rowCount();)
	{
		pLayout->removeRow(i);
	}
	for (int i = 0; i < ListControlParameter->count(); i++)
	{
		if (ListControlParameter->at(i).m_Type == HALCON_DATA_TYPE_NAME_STRING)
		{
			QLineEdit *pLineEdit = new QLineEdit();
			pLineEdit->setReadOnly(true);
			pLineEdit->setText(ListControlParameter->at(i).m_Value.toString());
			pLineEdit->setStyleSheet("QLineEdit {min-width: 180px; max-width: 180px}");
			pLayout->addRow(ListControlParameter->at(i).m_Name, pLineEdit);
		}
		else
		{
			if (ListControlParameter->at(i).m_Type == HALCON_DATA_TYPE_NAME_REAL)
			{
				QDoubleSpinBox *pQDoubleSpinBox = new QDoubleSpinBox();
				pQDoubleSpinBox->setReadOnly(true);
				pQDoubleSpinBox->setMaximum(5000);
				pQDoubleSpinBox->setMinimum(-5000);
				pQDoubleSpinBox->setDecimals(6);
				pQDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
				pQDoubleSpinBox->setValue(ListControlParameter->at(i).m_Value.toDouble());
				pQDoubleSpinBox->setStyleSheet("QDoubleSpinBox {min-width: 80px; max-width: 80px}");
				pLayout->addRow(ListControlParameter->at(i).m_Name, pQDoubleSpinBox);
			}
			else
			{
				if (ListControlParameter->at(i).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
				{
					QDoubleSpinBox *pQDoubleSpinBox = new QDoubleSpinBox();
					pQDoubleSpinBox->setReadOnly(true);
					pQDoubleSpinBox->setMaximum(5000);
					pQDoubleSpinBox->setMinimum(-5000);
					pQDoubleSpinBox->setDecimals(0);
					pQDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
					pQDoubleSpinBox->setValue(ListControlParameter->at(i).m_Value.toInt());
					pQDoubleSpinBox->setStyleSheet("QDoubleSpinBox {min-width: 80px; max-width: 80px}");
					pLayout->addRow(ListControlParameter->at(i).m_Name, pQDoubleSpinBox);
				}
			}
		}

		QFrame *line = new QFrame();
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		line->setFixedHeight(20);
		line->setContentsMargins(0, 0, 0, 0);
		pLayout->addRow(line);
	}
}


void ControlParameterWidget::ViewOutputControlParameter(QFormLayout *pLayout, QList<ProcedureOutputControlParameter> *ListControlParameter)
{
	for (int i = 0; i < pLayout->rowCount();)
	{
		pLayout->removeRow(i);
	}
	for (int i = 0; i < ListControlParameter->count(); i++)
	{
		if (ListControlParameter->at(i).m_Type == HALCON_DATA_TYPE_NAME_STRING)
		{
			QLineEdit *pLineEdit = new QLineEdit();
			pLineEdit->setReadOnly(true);
			pLineEdit->setText(ListControlParameter->at(i).m_Value.toString());
			pLineEdit->setStyleSheet("QLineEdit {min-width: 180px; max-width: 180px}");
			pLayout->addRow(ListControlParameter->at(i).m_Name, pLineEdit);
		}
		else
		{
			if (ListControlParameter->at(i).m_Type == HALCON_DATA_TYPE_NAME_REAL)
			{
				QDoubleSpinBox *pQDoubleSpinBox = new QDoubleSpinBox();
				pQDoubleSpinBox->setReadOnly(true);
				pQDoubleSpinBox->setMaximum(5000);
				pQDoubleSpinBox->setMinimum(-5000);
				pQDoubleSpinBox->setDecimals(6);
				pQDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
				pQDoubleSpinBox->setValue(ListControlParameter->at(i).m_Value.toDouble());
				pQDoubleSpinBox->setStyleSheet("QDoubleSpinBox {min-width: 80px; max-width: 80px}");
				pLayout->addRow(ListControlParameter->at(i).m_Name, pQDoubleSpinBox);
			}
			else
			{
				if (ListControlParameter->at(i).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
				{
					QDoubleSpinBox *pQDoubleSpinBox = new QDoubleSpinBox();
					pQDoubleSpinBox->setReadOnly(true);
					pQDoubleSpinBox->setMaximum(5000);
					pQDoubleSpinBox->setMinimum(-5000);
					pQDoubleSpinBox->setDecimals(0);
					pQDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
					pQDoubleSpinBox->setValue(ListControlParameter->at(i).m_Value.toInt());
					pQDoubleSpinBox->setStyleSheet("QDoubleSpinBox {min-width: 80px; max-width: 80px}");
					pLayout->addRow(ListControlParameter->at(i).m_Name, pQDoubleSpinBox);
				}
			}
		}

		QFrame *line = new QFrame();
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		line->setFixedHeight(20);
		line->setContentsMargins(0, 0, 0, 0);
		pLayout->addRow(line);
	}
}
