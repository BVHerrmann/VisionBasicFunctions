#include "MainAppBDevEngine.h"
#include "BDEvEngineSettingsWidget.h"
#include "BDEvEngineImagesOverview.h"
#include "BDevEngineImageOverview.h"
#include "bdevengineplugin.h"
#include "ImageData.h"
#include "VideoProducer.h"
#include "StreamingImage.h"
#include "MainLogic.h"


#include "halconcpp/HalconCpp.h"
#include "hdevengine/HDevEngineCpp.h"


MainAppBDevEngine::MainAppBDevEngine(BDevEnginePlugin *parent) : QObject()
, m_BDevEngineSettingsWidget(NULL)
, m_BDevEngineImagesOverview(NULL)
, m_BDevEnginePlugin(NULL)
, m_VideoProducer(NULL)
, m_StreamingImage(NULL)
, m_MaxImagesSaveOnDiskPerCamera(500)
{
	m_SettingsLocation = QCoreApplication::applicationDirPath() + QString("/") + "BDevEngine" + ".ini";//Allgemeine Programmeinstellungen werden Hier in Ini-Format gespeichert
	m_BDevEnginePlugin = parent;
	m_BDevEngineDataPath = "d://BDevEngineData";
	QDir().mkpath(m_BDevEngineDataPath);
	m_ImageDataPath = m_BDevEngineDataPath + "//Images";
	QDir().mkpath(m_BDevEngineDataPath);
	m_PathDXFFiles = m_BDevEngineDataPath + "//dxfFiles";
	QDir().mkpath(m_PathDXFFiles);
	m_MeasureProgramPath = GetBDevEnginePlugin()->GetPreference("BDevEnginePathMeasurePrograms").toString();//Speicherort für die Messprogramme von Halcon
	if (m_MeasureProgramPath.isEmpty())
	{
		m_MeasureProgramPath = m_BDevEngineDataPath + "//BDevEngineMeasurePrograms";
		GetBDevEnginePlugin()->SetPreference("BDevEnginePathMeasurePrograms", QVariant(m_MeasureProgramPath));
	}
	QDir().mkpath(m_MeasureProgramPath);
	m_BDevEngineImagesOverview = new BDevEngineImagesOverview(this);//Übersichtsbild zeigt alle Kamerabilder auf einem Widget, wenn anzahl Kameras größer als 1
	for (int i = 0; i < MAX_CAMERAS; i++)
		m_ImageDataSet.append(new ImageData(this, i));
    m_BDevEngineSettingsWidget = new BDevEngineSettingsWidget(this);//Einstellungen für die Zuordnung Messprogram zu Kamera
	//m_VideoProducer = new VideoProducer();
	m_StreamingImage = new StreamingImage();
	m_MainLogic = std::shared_ptr<MainLogic>(new MainLogic(this));

	// start CheckImageDiskSpace
	QTimer *pTimerCheckImageDiskSpace = new QTimer(this);
	pTimerCheckImageDiskSpace->setInterval(1000 * 60); 
	connect(pTimerCheckImageDiskSpace, &QTimer::timeout, this, &MainAppBDevEngine::SlotCheckImageDiskSpace);
	pTimerCheckImageDiskSpace->start();
}


MainAppBDevEngine::~MainAppBDevEngine()
{
	if (m_VideoProducer)
	{
		delete m_VideoProducer;
		m_VideoProducer = NULL;
	}
	if (m_StreamingImage)
	{
		delete m_StreamingImage;
		m_StreamingImage = NULL;
	}
	
	if(GetBDevEngineSettingsWidget())
	   GetBDevEngineSettingsWidget()->DisConnectAllSignalSlots();
	FreeAll();
}


void MainAppBDevEngine::SlotProcedureChangedByPLC(const QString &CameraName, const QString &ProcedureName)
{
	if (GetBDevEngineSettingsWidget())
		GetBDevEngineSettingsWidget()->SlotProcedureChangedByPLC(CameraName, ProcedureName);
}


int MainAppBDevEngine::LoadAndSetAllProcedures(QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		if (m_ImageDataSet.at(i))// && m_ImageDataSet.at(i)->GetCameraID() != DISABLED_NAME_CAMERA)
		{
			rv=m_ImageDataSet.at(i)->LoadAndSetAllProcedures(ErrorMsg);
		}
	}
	return rv;
}


void MainAppBDevEngine::ConfigureOPCUA(QString &ConfigureOrSetup)
{
	if (GetBDevEnginePlugin())
		GetBDevEnginePlugin()->ConfigureOPCUA(ConfigureOrSetup);
}


void MainAppBDevEngine::SetupControlParameterWidget()
{
	for (int i = 0; i < m_ImageDataSet.count(); i++)
		m_ImageDataSet.at(i)->SetupControlParameterWidget();
}


void MainAppBDevEngine::SlotStartupInitReady()
{
	SetupControlParameterWidget();
}


void MainAppBDevEngine::SlotCheckIsCameraDisabled(int Index)
{
	if (GetBDevEngineSettingsWidget())
	{
		if (GetBDevEngineSettingsWidget()->CheckIsCameraDisabled(Index))
		{
			if (Index >= 0 && Index < m_ImageDataSet.count())
			{
				if (m_ImageDataSet.at(Index)->GetBDevEngineImageOverview())
					m_ImageDataSet.at(Index)->GetBDevEngineImageOverview()->PaintTextCameraNotActive();
			}
			if(GetBDevEngineImagesOverview())
			   GetBDevEngineImagesOverview()->PaintTextCameraNotActive(Index);
		}
	}
}


void MainAppBDevEngine::SetCameraName(QString &NewName, int Index)
{
	SetImageDataName(NewName,Index);

	if (GetBDevEnginePlugin())
		GetBDevEnginePlugin()->CameraNameChanged(NewName,Index);
}


void MainAppBDevEngine::FreeAll()
{
	for (int i = 0; i < m_ImageDataSet.count(); i++)
	{
		delete m_ImageDataSet.at(i);
	}
	m_ImageDataSet.clear();
}


int MainAppBDevEngine::GetCurrentNumberCameras()
{
	if(GetBDevEnginePlugin())
		return GetBDevEnginePlugin()->GetCurrentNumberCameras();
	else
		return 0;
}

BDevEngineImageOverview *MainAppBDevEngine::GetBDevEngineImageOverview(int index)
{
	if (index >= 0 && index < m_ImageDataSet.count())
		return m_ImageDataSet.at(index)->GetBDevEngineImageOverview();
	else
		return NULL;
}


QString MainAppBDevEngine::GetImageDataName(int index)
{
	if (index >= 0 && index < m_ImageDataSet.count())
		return m_ImageDataSet.at(index)->GetName();
	else
		return QString("NoName");
}


void MainAppBDevEngine::SetImageDataName(QString &NewName,int index)
{
	if (index >= 0 && index < m_ImageDataSet.count())
		m_ImageDataSet.at(index)->SetName(NewName);
	
}


ImageData *MainAppBDevEngine::GetImageDataByCameraID(const QString &CameraID)
{
	for (int i = 0; i < m_ImageDataSet.count(); i++)
	{
		if (m_ImageDataSet.at(i)->GetCameraID() == CameraID)
			return m_ImageDataSet.at(i);
	}
	return NULL;
}


ImageData *MainAppBDevEngine::GetImageDataByName(const QString &Name)
{
	for (int i = 0; i < m_ImageDataSet.count(); i++)
	{
		if (m_ImageDataSet.at(i)->GetName() == Name)
			return m_ImageDataSet.at(i);
	}
	return NULL;
}

ImageData *MainAppBDevEngine::GetImageDataByIndex(int index)
{
	if (index >= 0 && index < m_ImageDataSet.count())
		return m_ImageDataSet.at(index);
	else
		return NULL;
	
}

void MainAppBDevEngine::SoftwareTrigger(QString &CameraID)
{
	if (GetBDevEnginePlugin())
	{
		GetBDevEnginePlugin()->SoftwareTrigger(CameraID);
	}
}

//Anzeigen unterschiedlicher Fehlertexte enum QtMsgType { QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtInfoMsg, QtSystemMsg = QtCriticalMsg };
void MainAppBDevEngine::SlotAddNewMessage(const QString &Msg, QtMsgType MsgType)
{
	if (GetBDevEnginePlugin())
		GetBDevEnginePlugin()->SetMessage(Msg, MsgType);//Info an die obergeordnete Instanz
}


void MainAppBDevEngine::SlotAddNewDebugInfo(const QString &Msg, int InfoCode)
{
	SlotAddNewMessage(Msg, QtMsgType::QtInfoMsg);
}


void MainAppBDevEngine::SlotCheckImageDiskSpace()
{
	QString ImageRootPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	QDir dir(ImageRootPath);
	QStorageInfo storageInfo(dir);
	QStringList ListImageDirsPerCamera;
	QStringList filters;
	filters IMAGE_FITERS;// << "*.bmp" << "*.tif" << "*.png";
	int MaxFilePerDir = 500;

	qint64 minSpace = (qint64)25 * 1024 * 1024 * 1024;

	//if (storageInfo.bytesAvailable() < minSpace) 
	{
		for (int i = 0; i < MAX_CAMERAS; i++)
		{
			if (m_ImageDataSet.at(i) && m_ImageDataSet.at(i)->GetCameraID() != DISABLED_NAME_CAMERA)
			{
				ListImageDirsPerCamera = m_ImageDataSet.at(i)->GetListImageDirs();
				for (int j = 0; j < ListImageDirsPerCamera.count(); j++)
				{
					QDir ImageDir(ListImageDirsPerCamera.at(j));
					QFileInfoList imageFiles = ImageDir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
					if (imageFiles.count() > MaxFilePerDir)
					{
						for (int k = MaxFilePerDir; k < imageFiles.count(); k++)
						{
							ImageDir.remove(imageFiles.at(k).absoluteFilePath());
						}
					}
				}
			}
		}


		// delete oldest image dir
		/*QStringList imageDirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
		if (imageDirs.count())
		{
			QDir subDir(dir.filePath(imageDirs.first()));
			qDebug() << "remove" << subDir << ":" << subDir.removeRecursively();
		}
		*/
	}
}



