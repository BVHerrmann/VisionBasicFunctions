#include "BDEvEngineImageOverview.h"
#include <QtGui>
#include <QtWidgets>
#include "ImageData.h"
#include "ControlParameterWidget.h"



BDevEngineImageOverview::BDevEngineImageOverview(ImageData *pImageData) : QWidget(NULL)
,m_QHalconWindow(NULL)
,m_ControlParameterWidget(NULL)
{
	m_ImageData = pImageData;
	setupUi();
}


BDevEngineImageOverview::~BDevEngineImageOverview()
{

}


void BDevEngineImageOverview::SetupControlParameterWidget()
{
	if (GetControlParameterWidget())
		GetControlParameterWidget()->SetupControlParameterWidget();
}


void BDevEngineImageOverview::ViewNewOutputControlParameter()
{
	if (GetControlParameterWidget())
		GetControlParameterWidget()->ViewNewOutputControlParameter();
}


void BDevEngineImageOverview::ViewNewInputControlParameter()
{
	if (GetControlParameterWidget())
		GetControlParameterWidget()->ViewNewInputControlParameter();
}


double BDevEngineImageOverview::GetImageAspectRatio()
{
	return (double)(m_QHalconWindow->width()) / m_QHalconWindow->height();
}

void BDevEngineImageOverview::SlotSetAspectRatioNew(double AspectRatio)
{
	int newHeight = 640 / AspectRatio;
	int MaxWidth = 640;// m_ImageData->GetImageWidth();
	int MaxHeight = newHeight;// m_ImageData->GetImageHeight();
	int MinWidth = MaxWidth / 2.0;
	int MinHeight = MaxHeight / 2.0;
	m_QHalconWindow->setMinimumSize(MinWidth, MinHeight);
	m_QHalconWindow->setMaximumSize(MaxWidth, MaxHeight);
	m_QHalconWindow->resize(MaxWidth, MaxHeight);
	m_QHalconWindow->SetWindowID();
	m_ControlParameterWidget->setMinimumSize(MinWidth, MinHeight);
	m_ControlParameterWidget->setMaximumSize(MaxWidth, MaxHeight);
}


void BDevEngineImageOverview::setupUi()
{
	int MaxWidth  = m_ImageData->GetImageWidth();
	int MaxHeight = m_ImageData->GetImageHeight();
	int MinWidth  = MaxWidth/2.0;
	int MinHeight = MaxHeight/2.0;


	m_QHalconWindow = new QHalconWindow(this);
	m_ControlParameterWidget = new ControlParameterWidget(this);


	m_QHalconWindow->setMinimumSize(MinWidth, MinHeight);
	m_QHalconWindow->setMaximumSize(MaxWidth, MaxHeight);
	m_QHalconWindow->resize(MaxWidth, MaxHeight);
	m_QHalconWindow->SetWindowID();
	m_ControlParameterWidget->setMinimumSize(MinWidth, MinHeight);
	m_ControlParameterWidget->setMaximumSize(MaxWidth, MaxHeight);

	QGridLayout *pGridLayout = new QGridLayout();
	QHBoxLayout *pBoxLayout = new QHBoxLayout();
	QGridLayout *LayoutImageAndControlParameter = new QGridLayout();
	QPushButton *ButtonTriggerCamera = new QPushButton(tr("Trigger Camera"));
		
	connect(ButtonTriggerCamera, &QPushButton::clicked, this, &BDevEngineImageOverview::SlotTriggerCamera);

	LayoutImageAndControlParameter->addWidget(m_QHalconWindow, 0, 0);
	LayoutImageAndControlParameter->addWidget(m_ControlParameterWidget, 0, 1);
	
	pGridLayout->addLayout(LayoutImageAndControlParameter, 0, 0);
	pGridLayout->addWidget(ButtonTriggerCamera, 1, 0);
	
	pBoxLayout->insertSpacing(0, 120);
	pBoxLayout->insertLayout(1, pGridLayout);
	pBoxLayout->insertSpacing(2, 120);
		
	setLayout(pBoxLayout);
	//HalconCpp::HImage Image;
	//HalconCpp::HString PathAndName = "d:\\temp\\fin1.png";
	//Image.ReadImage(PathAndName);
	m_QHalconWindow->resize(MaxWidth, MaxHeight);
}


HalconCpp::HTuple BDevEngineImageOverview::GetWindowID()
{
	if (GetQHalconWindow())
		return GetQHalconWindow()->GetWindowID();
	else
		return 0;
}


QImage BDevEngineImageOverview::GetWindowImage()
{
	return qApp->primaryScreen()->grabWindow(m_QHalconWindow->winId()).toImage();
}


void BDevEngineImageOverview::PaintTextCameraNotActive()
{
	if (GetQHalconWindow())
	    GetQHalconWindow()->PaintTextCameraNotActive();
}



void BDevEngineImageOverview::SlotTriggerCamera()
{
	if (GetImageData())
	{
		GetImageData()->SoftwareTrigger();
		if (GetImageData()->GetCameraID() == SIMULATION_NAME_CAMERA)
		{
			GetImageData()->StepOneImageInSimulation();
		}
	}
}

	