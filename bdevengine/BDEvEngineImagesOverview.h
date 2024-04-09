#pragma once
#include <qwidget.h>
#include "qhalconwindow.h"


class QHalconWindow;
class QGridLayout;
class MainAppBDevEngine;
class BDevEngineImagesOverview : public QWidget
{
	Q_OBJECT
public:
	BDevEngineImagesOverview(MainAppBDevEngine *parent);
	~BDevEngineImagesOverview();
	MainAppBDevEngine *GetMainAppBDevEngine() {return m_MainAppBDevEngine;}
	void AppendNewCameraView(int index);
	void PaintTextCameraNotActive(int index);
	HalconCpp::HTuple GetWindowID(int Index);
private:
	MainAppBDevEngine *m_MainAppBDevEngine;
	QGridLayout *m_CameraViewLayout;
	QList<QHalconWindow *> m_QHalconWindows;
};

