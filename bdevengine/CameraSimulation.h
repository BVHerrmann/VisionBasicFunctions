#pragma once
#include <QtCore>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"


class ImageData;
class CameraSimulation : public QThread
{
	Q_OBJECT
public:
	CameraSimulation(ImageData *pImageData);
	~CameraSimulation();
	virtual void   run();
	ImageData *GetImageData() { return m_ImageData; }
	void WaitForFinshed();
	void StartSimulation(QString &FileLocation);
	void SetVideoState(int set) { m_VideoState = set; }
	int  SetSimulationFrameInterval(int set) { m_SimulationFrameInterval = set; }
	void StepImage();
	QString  GetCurrentFilename() { return m_CurrentFilename; }

signals:
	void SignalSetCameraStatus(const QString &Text,bool Simulation);
	
	void SignalShowMessage(const QString &ErrorMsg, QtMsgType msgType);

private:
	QString         m_FileLocation;
	QString         m_CurrentFilename;
	ImageData      *m_ImageData;
	bool            m_TerminateInspection;
	
	QMutex          m_WaitThreadIsFinished;
	QWaitCondition  m_WaitConditionThreadIsFinished;
	QWaitCondition  m_WaitConditionGetNextImage;
	int             m_SimulationFrameInterval;
	int             m_VideoState;
	
   
};

