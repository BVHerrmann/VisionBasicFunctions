#pragma once
#include <qwidget.h>
#include "qhalconwindow.h"

class ControlParameterWidget;
class ImageData;
class BDevEngineImageOverview : public QWidget
{
	Q_OBJECT
public:
	BDevEngineImageOverview(ImageData *parent);
	~BDevEngineImageOverview();
	ImageData *GetImageData() {return m_ImageData;}
	ControlParameterWidget *GetControlParameterWidget() { return m_ControlParameterWidget; }
	QHalconWindow *GetQHalconWindow() { return m_QHalconWindow; }
	void setupUi();
	HalconCpp::HTuple GetWindowID();
	QImage GetWindowImage();
	void PaintTextCameraNotActive();
	void SetupControlParameterWidget();
	void ViewNewOutputControlParameter();
	void ViewNewInputControlParameter();
	double GetImageAspectRatio();
	
public slots:
	void SlotTriggerCamera();
	void SlotSetAspectRatioNew(double AspectRatio);
private:
	ImageData *m_ImageData;
	QHalconWindow *m_QHalconWindow;
	ControlParameterWidget *m_ControlParameterWidget;
};

