#pragma once
#include <qobject.h>
#include "GlobalConst.h"
#include "ProcedureIOControlParameter.h"

class MainLogic;
class StreamingImage;
class VideoProducer;
class BDevEnginePlugin;
class BDevEngineSettingsWidget;
class BDevEngineImagesOverview;
class BDevEngineImageOverview;
class ImageData;
class MainAppBDevEngine : public QObject
{
	Q_OBJECT
public:
	MainAppBDevEngine(BDevEnginePlugin *parent);
	~MainAppBDevEngine();
	BDevEngineSettingsWidget *GetBDevEngineSettingsWidget() { return m_BDevEngineSettingsWidget; }
	BDevEngineImagesOverview *GetBDevEngineImagesOverview() { return m_BDevEngineImagesOverview; }
	BDevEnginePlugin *GetBDevEnginePlugin() {return m_BDevEnginePlugin;}

	VideoProducer *GetVideoProducer() { return m_VideoProducer; }
	StreamingImage *GetStreamingImage() { return m_StreamingImage; }
	
	BDevEngineImageOverview *GetBDevEngineImageOverview(int index);
	QString GetImageDataName(int index);
	void SetImageDataName(QString &NewName, int index);

	ImageData *GetImageDataByCameraID(const QString &CameraID);
	ImageData *GetImageDataByIndex(int index);
	ImageData *GetImageDataByName(const QString &Name);

	QString GetMeasureProgramPath() {return m_MeasureProgramPath;}
	void SoftwareTrigger(QString &CameraID);
	void FreeAll();
	QString GetSettingsLocation() {return m_SettingsLocation;}

	int GetCurrentNumberCameras();

	void SetCameraName(QString &NewName, int Index);

	QString GetBDevEngineDataPath() { return m_BDevEngineDataPath; }
	QString GetImageDataPath() { return m_ImageDataPath; }
	QString GetPathDXFFiles() { return m_PathDXFFiles; }

	int GetMaxImagesSaveOnDiskPerCamera() { return m_MaxImagesSaveOnDiskPerCamera; }
	void SetMaxImagesSaveOnDiskPerCamera(int set) { m_MaxImagesSaveOnDiskPerCamera = set; }
	int LoadAndSetAllProcedures(QString &ErroMsg);
	void SetupControlParameterWidget();

	void ConfigureOPCUA(QString &ConfigureOrSetup);
	std::shared_ptr<MainLogic>  GetMainLogic() { return m_MainLogic; }
	
public slots:
	void SlotAddNewMessage(const QString &Msg, QtMsgType MsgType);
	void SlotAddNewDebugInfo(const QString &Msg, int InfoCode);
	void SlotCheckIsCameraDisabled(int Index);
	void SlotStartupInitReady();
	void SlotProcedureChangedByPLC(const QString &CameraName, const QString &ProcedureName);
	void SlotCheckImageDiskSpace();

private:
	BDevEngineSettingsWidget *m_BDevEngineSettingsWidget;
	BDevEngineImagesOverview *m_BDevEngineImagesOverview;
	BDevEnginePlugin *m_BDevEnginePlugin;
	QList<ImageData   *>          m_ImageDataSet;
	QString m_MeasureProgramPath;
	QString m_SettingsLocation;
	QString m_BDevEngineDataPath;
	QString m_ImageDataPath;
	QString m_PathDXFFiles;
	int m_MaxImagesSaveOnDiskPerCamera;
	VideoProducer *m_VideoProducer;
	StreamingImage *m_StreamingImage;
	std::shared_ptr<MainLogic>  m_MainLogic;
};

