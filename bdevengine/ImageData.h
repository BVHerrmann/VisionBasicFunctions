#pragma once

#include "logic.h"

#include "QtCore"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"

#include "halconcpp/HalconCpp.h"
#include "GlobalConst.h"
class ProcedureControlParameter;
class ProcedureIOControlParameter;
class CameraSimulation;
class HalconMeasureTool;
class BDevEngineImageOverview;
class MainAppBDevEngine;
class ImageData : public QThread
{
	Q_OBJECT
public:
	ImageData(MainAppBDevEngine *pParent, int index);
	~ImageData();
	virtual void   run();
	MainAppBDevEngine *GetMainAppBDevEngine() { return m_MainAppBDevEngine; }
	CameraSimulation  *GetCameraSimulation()  { return m_CameraSimulation; }

 	int     GetIndex()              { return m_Index; }
	QString GetName()               { return m_Name; }
	QString GetCameraID()           { return m_CameraID; }
	QString GetMeasureProgramName() { return m_MeasureProgramName; }
	QString GetProcedureName()      { return m_ProcedureName; }

	QString GetPathDXFFiles();

	QString GetCameraIndexName();
	QString GetIniFileLocation();

	QString GetCameraCalibrationDataPath() { return m_CameraCalibrationDataPath; }
	QString GetCameraCalibrationDataPathRecording() { return m_CameraCalibrationDataPathRecording; }

	void SetIndex(int set) { m_Index = set; }
	void SetName(QString &set) { m_Name=set; }
	void SetCameraID(QString &set);// { m_CameraID = set; }
	void SetMeasureProgramName(QString &set) {m_MeasureProgramName = set; }
	void SetProcedureName(QString &set);

	BDevEngineImageOverview  *GetBDevEngineImageOverview() {return m_BDevEngineImageOverview;}

	void NewIncommingImage(const cv::Mat &image);
	int AddToHalconImage(int rows, int cols, unsigned char *data, QString &ErrorMsg);
	void WaitForFinshed();
	bool SoftwareTrigger();
	QString GetMeasureProgramPath();
	bool GetReloadMeasureProgram() { return m_ReloadMeasureProgram; }
	void SetReloadMeasureProgram(bool set) { m_ReloadMeasureProgram=set; }
	HalconCpp::HTuple GetWindowID();

	int DisplayImage(HalconCpp::HImage Image, HalconCpp::HTuple WindowID,QString &ErrorMsg);
	int ForcePaintResultOnImageView(HalconCpp::HTuple WindowID, QString &ErrorMsg);
	int StreamViewImage(HalconCpp::HTuple WindowID, HalconCpp::HImage CameraRawImage, QString &ErrorMsg);
	
	int  DrawAdditinalResultsAndStreamImage(HalconCpp::HImage CameraRawImage, QString &ErrorMsg);
	int  DisplayImageAndOverviewImage(HalconCpp::HImage Image, QString &ErrorMsg);
	static void CallBackJpegByte(unsigned char byte);

	void SetViewRawCameraImage(bool set) { m_ViewRawCameraImage = set; }
	bool GetViewRawCameraImage() { return m_ViewRawCameraImage; }

	void CheckImageDir(QString &Dir);
	int  SaveImage(HalconCpp::HImage &CameraRawImage, QString &StringValue, QString &ErrorMsg);
	void StartCameraSimulation(bool Start);

	//QString GetImagePathLocation() { return m_ImagePathLocation; }

	void YUVfromRGB(double& Y, double& U, double& V, const double R, const double G, const double B);
	void WriteByteStram(QByteArray &ByteStream);

	void StartTimerMeasureFullInspectionTime();
	void StopTimerMeasureFullInspectionTime();

	ProcedureIOControlParameter *GetIOControlParameter();
	void SetSignalViewOutputControlParameter();
	void SetSignalViewInputControlParameter();

	void SetupControlParameterWidget();
	void SetTriggerReady();

	int LoadAndSetAllProcedures(QString &ErrorMsg);
	void ResultsToPLC(HalconCpp::HImage &CameraRawImage);
	bool ExistProcedure(QString &ProcedureName);
	//bool SetInputControlParameter(QString &ProcedureName, QString &InputParameterName, const QVariant &Value,QString &ErrorMsg);
	void ReadInputControlParameterFromOPCUA();
	void ClearAllProcedureParameter();

	bool TriggerAcquisition();
	void SetInspectionID(uint32_t value);
	bool LoadJob(std::string &newJob);

	QHash<QString, ProcedureIOControlParameter> &GetListProcedureIOParameter() {return m_ListProcedureIOParameter;}

	QStringList GetListImageDirs() { return m_ListImageDirs; }
	bool ProceduresLoaded();
	void StepOneImageInSimulation();
	unsigned int GetImageWidth()  { return m_ImageWidth;}
	unsigned int GetImageHeight() { return m_ImageHeight; }

	unsigned int GetImageCounter() { return m_ImageCounter; }
	
signals:
	void SignalShowMessage(const QString &ErrorMsg, QtMsgType msgType);
	void SignalShowInspectionTime(double m_FullInspectionTimeInms);
	void SignalCheckIsCameraDisable(int Index);
	void SignalViewOutputControlParameter();
	void SignalViewInputControlParameter();
	void SignalProcedureChangedByPLC(const QString &CameraName, const QString &ProcedureName);
	void SignalSetAspectratio(double);
	

public slots:
	void SlotViewOutputControlParameter();
	void SlotViewInputControlParameter();
	

private:
	MainAppBDevEngine *m_MainAppBDevEngine;
	int     m_Index;
	unsigned int m_ImageCounter;
	unsigned int m_ImageWidth, m_ImageHeight;
	bool m_TerminateInspection;
	bool m_ReloadMeasureProgram;
	bool m_ViewRawCameraImage;
	bool m_InspectionCompleted;
	bool m_ReadyMeasuring;
	bool m_LastTriggerCamera;
	double  m_FullInspectionTimeInms;
	QString m_Name;
	QString m_CameraID;
	QString m_MeasureProgramName;
	QString m_ProcedureName;
	QString m_CameraSimulationRecordType;
	QString m_CameraCalibrationDataPath;
	QString m_CameraCalibrationDataPathRecording;
	//QString m_ImagePathLocation;
	BDevEngineImageOverview  *m_BDevEngineImageOverview;//Anzeige Bild
	QMutex                            m_MutexNewImage;
	QWaitCondition                    m_WaitConditionNewImage;
	QMutex                            m_WaitLiveImageViewIsDisable;
	QWaitCondition                    m_WaitConditionLiveViewIsDisable;
	QMutex                            m_MutexMeasuringIsRunning;
	QQueue<HalconCpp::HImage>  m_QQueueIncommingImages;
	HalconMeasureTool *m_HalconMeasureTool;
	static QByteArray m_JpegByteArray;
	CameraSimulation *m_CameraSimulation;
	QElapsedTimer                     m_TimerMeasureFullInspectionTime;
	QHash<QString, ProcedureIOControlParameter> m_ListProcedureIOParameter;
	QStringList m_ListImageDirs;
	HalconCpp::HTuple m_HalconBufferWindowID;
	

public:
	QList<output<int32_t>*>      m_ListOutputInt32Values;
	QList<output<double>*>       m_ListOutputDoubleValues;
	QList<output<bool>*>         m_ListOutputBoolValues;
	QList<output<std::string>*>  m_ListOutputStringValues;

	output<bool>        m_ResultsValid;
	output<bool>        m_InspCompleted;
	output<bool>        m_TriggerReady;
	output<bool>        m_SystemBusy;
	output<bool>        m_JobPass;
	output<bool>        m_ModelFree;
	output <uint32_t>   m_InspectionID;
	output<std::string> m_CurrentJobName;
	input<std::string>  m_RecordImages;
	input<double>  m_Test;
	
	QList<input<int32_t>*>     m_ListInputInt32Values;
	QList<input<double>*>      m_ListInputDoubleValues;
	QList<input<bool>*>        m_ListInputBoolValues;
	QList<input<std::string>*> m_ListInputStringValues;
};
Q_DECLARE_METATYPE(HalconCpp::HImage);


