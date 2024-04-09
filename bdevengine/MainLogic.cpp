#include "MainLogic.h"
#include "MainAppBDevEngine.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include "ImageData.h"

MainLogic::MainLogic(MainAppBDevEngine *pMainAppBDevEngine) : Logic()
, m_MainAppBDevEngine(NULL)
{
	m_MainAppBDevEngine = pMainAppBDevEngine;
}


void MainLogic::doWork()
{
	QFuture<void> Cam[MAX_CAMERAS];

	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		ImageData *pImageData = GetMainAppBDevEngine()->GetImageDataByIndex(i);
		if (pImageData && pImageData->GetCameraID() != DISABLED_NAME_CAMERA)
		{
			Cam[i] = QtConcurrent::run(pImageData, &ImageData::ReadInputControlParameterFromOPCUA);// , pImageData);
		}
	}
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		if (Cam[i].isRunning())
		{
			Cam[i].waitForFinished();
		}
	}
}



	
