#include "BDEvEngineImagesOverview.h"
#include <QtGui>
#include <QtWidgets>
#include "ResultBufferAndImage.h"
#include "GlobalConst.h"
#include "qboxlayout.h"


BDevEngineImagesOverview::BDevEngineImagesOverview(MainAppBDevEngine *parent) : QWidget(NULL)
, m_CameraViewLayout(NULL)
{
	m_MainAppBDevEngine = parent;
	m_CameraViewLayout = new QGridLayout();
	QHBoxLayout *pBoxLayout = new QHBoxLayout();
	for (int i = 0; i < MAX_CAMERAS; i++)
		AppendNewCameraView(i);
	pBoxLayout->insertSpacing(0,120);
	pBoxLayout->insertLayout(1,m_CameraViewLayout);
	pBoxLayout->insertSpacing(2,120);
	setLayout(pBoxLayout);
}


BDevEngineImagesOverview::~BDevEngineImagesOverview()
{

}


void BDevEngineImagesOverview::AppendNewCameraView(int index)
{
	QHalconWindow *pQHalconWindow = new QHalconWindow(this);
	pQHalconWindow->setMinimumSize(128, 128);
	pQHalconWindow->setMaximumSize(640*0.9, 480*0.9);

	m_QHalconWindows.append(pQHalconWindow);
	
	switch (index)
	{
	    case 0:
			 m_CameraViewLayout->addWidget(pQHalconWindow, 0, 0);
		     break;
		case 1:
			 m_CameraViewLayout->addWidget(pQHalconWindow, 0, 1);
			 break;
		case 2:
			 m_CameraViewLayout->addWidget(pQHalconWindow, 1, 0);
			 break;
		case 3:
			 m_CameraViewLayout->addWidget(pQHalconWindow, 1, 1);
			 break;
		default:
			break;
	}
	
}


HalconCpp::HTuple BDevEngineImagesOverview::GetWindowID(int Index)
{
	HalconCpp::HTuple WinID=0;
	if (Index >= 0 && Index < m_QHalconWindows.count())
		return m_QHalconWindows.at(Index)->GetWindowID();
	return  WinID;
}


void BDevEngineImagesOverview::PaintTextCameraNotActive(int Index)
{
	if (Index >= 0 && Index < m_QHalconWindows.count())
		m_QHalconWindows.at(Index)->PaintTextCameraNotActive();
}

	