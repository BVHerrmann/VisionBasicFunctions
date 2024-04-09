#pragma once
#include "logic.h"
#include <interfaces.h>
#include <QtCore>

class ImageData;
class MainAppBDevEngine;
class MainLogic :public Logic
{
	Q_OBJECT
public:
	explicit MainLogic(MainAppBDevEngine *pMainAppBDevEngine);
	MainAppBDevEngine *GetMainAppBDevEngine() { return m_MainAppBDevEngine; }

	//void ReadInputControlParameterFromOPCUA(ImageData *pImageData);

	
private:
	
	MainAppBDevEngine *m_MainAppBDevEngine;
	virtual void doWork();

};

