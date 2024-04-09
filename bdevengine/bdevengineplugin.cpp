#include "bdevengineplugin.h"
#include "MainAppBDevEngine.h"
#include "ImageData.h"
#include "BDEvEngineSettingsWidget.h"
#include "BDEvEngineImagesOverview.h"
#include "opcvalue.h"
#include "MainLogic.h"


BDevEnginePlugin::BDevEnginePlugin(QObject *parent) : Plugin(parent)
, m_MainAppBDevEngine(NULL)
, m_CurrentNumberCameras(0)
, m_NumberMainWidget(2)
, m_BDevEngineImagesOverview(NULL)
, m_MessageCounter(1)
, m_StartUnInit(false)
{
	m_CurrentNumberCameras = 4;
	m_NumberMainWidget = 6;
	for (int i = 0; i < MAX_CAMERAS; i++)
		m_BDevEngineImageOverview[i] = NULL;
	// initialize views
	m_MainAppBDevEngine = new MainAppBDevEngine(this);
	m_BDevEngineSettingsWidget = GetMainAppBDevEngine()->GetBDevEngineSettingsWidget();
	m_BDevEngineImagesOverview = GetMainAppBDevEngine()->GetBDevEngineImagesOverview();
	connect(this, &BDevEnginePlugin::SignalStartupInitReady, m_MainAppBDevEngine, &MainAppBDevEngine::SlotStartupInitReady);
	for (int index = 0; index < MAX_CAMERAS; index++)
	{
		m_BDevEngineImageOverview[index] = GetMainAppBDevEngine()->GetBDevEngineImageOverview(index);
		m_CameraNames[index]             = GetMainAppBDevEngine()->GetImageDataName(index);
	}
}


BDevEnginePlugin::~BDevEnginePlugin()
{
	
}


void BDevEnginePlugin::initialize()
{
	if (GetMainAppBDevEngine())
	{
		QString ErrorMsg;
		if (GetMainAppBDevEngine()->LoadAndSetAllProcedures(ErrorMsg) == ERROR_CODE_NO_ERROR)
		{
			QString ConfigureOrSetup = "configure";
			ConfigureOPCUA(ConfigureOrSetup);
			emit SignalStartupInitReady();
		}
		else
		  SetMessage(ErrorMsg, QtMsgType::QtFatalMsg);
    }
}


void BDevEnginePlugin::ConfigureOPCUA(QString &ConfigureOrSetup)
{
	QHash<QString, QVariant> config;
	QVector<std::shared_ptr<OPCInputValue>>  in_config;
	QVector<std::shared_ptr<OPCOutputValue>> out_config;
	QVector<std::shared_ptr<OPCMethod>>      method_config;
	QString Description;
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		ImageData *pImagedata = GetMainAppBDevEngine()->GetImageDataByIndex(i);
		if (pImagedata)
		{
			if (pImagedata->GetCameraID() != DISABLED_NAME_CAMERA)
			{
				QString  Path, InputPathAndParameterName, OutputPathAndParameterName;
				QHash<QString, ProcedureIOControlParameter> hashProcedureParameter = pImagedata->GetListProcedureIOParameter();
				QHashIterator<QString, ProcedureIOControlParameter> i(hashProcedureParameter);
				while (i.hasNext())
				{
					i.next();
					ProcedureIOControlParameter pProcedureIOControlParameter = i.value();
					Path = pImagedata->GetName() + QString("/") + pProcedureIOControlParameter.m_ProcedureName + QString("/");
					for (int in = 0; in < pProcedureIOControlParameter.m_ListInputControlParameter.count(); in++)
					{//inputs
						Description = pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Description;
						InputPathAndParameterName = Path + pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Name;
						if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Type == HALCON_DATA_TYPE_NAME_REAL)
						{
							if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Description != "DisableForPLC")
							  in_config << std::make_shared<OPCInputValue>(*(pImagedata->m_ListInputDoubleValues.at(pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Name, InputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
						}
						else
						{
							if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
							{
								if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Description != "DisableForPLC")
								{
									if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_IsBool)
										in_config << std::make_shared<OPCInputValue>(*(pImagedata->m_ListInputBoolValues.at(pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Name, InputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
									else
										in_config << std::make_shared<OPCInputValue>(*(pImagedata->m_ListInputInt32Values.at(pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Name, InputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
								}
							}
							else
							{
								if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Type == HALCON_DATA_TYPE_NAME_STRING)
								{
									if (pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Description != "DisableForPLC")
									{
										in_config << std::make_shared<OPCInputValue>(*(pImagedata->m_ListInputStringValues.at(pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListInputControlParameter.at(in).m_Name, InputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
									}
								}
							}
						}
					}
					for (int out = 0; out < pProcedureIOControlParameter.m_ListOutputControlParameter.count(); out++)
					{//outputs
						Description = pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Description;
						OutputPathAndParameterName = Path + pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name;
						if (pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Type == HALCON_DATA_TYPE_NAME_REAL)
						{
							out_config << std::make_shared<OPCOutputValue>(*(pImagedata->m_ListOutputDoubleValues.at(pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
						}
						else
						{
							if (pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
							{
								if (pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_IsBool)
								{
									if(pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name != OPCUA_OUTPUT_NAME_JOP_PASS && pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name != OPCUA_OUTPUT_NAME_MODEL_FREE)
									   out_config << std::make_shared<OPCOutputValue>(*(pImagedata->m_ListOutputBoolValues.at(pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
								}
								else
									out_config << std::make_shared<OPCOutputValue>(*(pImagedata->m_ListOutputInt32Values.at(pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
							}
							else
							{
								if (pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Type == HALCON_DATA_TYPE_NAME_STRING)
								{
									out_config << std::make_shared<OPCOutputValue>(*(pImagedata->m_ListOutputStringValues.at(pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_TableIndexOPCUA)), pProcedureIOControlParameter.m_ListOutputControlParameter.at(out).m_Name, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
								}
							}
						}
					}
				}
				//input methods
				InputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_METHOD_NAME_TRIGGER_CAMERA;
				Description = tr("Setting this bit triggers an image acquisition");
				method_config << std::make_shared<OPCMethod>(OPCUA_METHOD_NAME_TRIGGER_CAMERA, InputPathAndParameterName, std::function<bool()>([=]()->bool { return pImagedata->TriggerAcquisition(); }), OPCUA_METHOD_NAME_TRIGGER_CAMERA, Description);
				InputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_METHOD_NAME_LOAD_JOB;
				Description = tr("Loads the specified job name on to the vision system. The procedure name specified must exist system.");
				method_config << std::make_shared<OPCMethod>(OPCUA_METHOD_NAME_LOAD_JOB, InputPathAndParameterName, [=](std::string value)->bool { return pImagedata->LoadJob(value); }, OPCUA_METHOD_NAME_LOAD_JOB, OPCUA_METHOD_NAME_LOAD_JOB_INPUT,Description);
				//input data
				InputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_INPUT_NAME_RECORD_IMAGES;
				Description = tr("Save Images-> 'none' : Do not record images ; 'all' : All images ; 'nio' : Only when JobPass == 0");
				in_config << std::make_shared<OPCInputValue>(pImagedata->m_RecordImages, OPCUA_INPUT_NAME_RECORD_IMAGES, InputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				InputPathAndParameterName = pImagedata->GetName() + QString("/") + "test";
				in_config << std::make_shared<OPCInputValue>(pImagedata->m_Test,QString("test"), InputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				//output data
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_RESULTS_VALID;
				Description = tr("Set when measuring finished. If set on false measuring is runnig");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_ResultsValid, OPCUA_OUTPUT_NAME_RESULTS_VALID, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_INSPECTION_COMPLETE;
				Description = tr("This bit is toggled upon the completion of an inspection");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_InspCompleted, OPCUA_OUTPUT_NAME_INSPECTION_COMPLETE, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_SYSTEM_BUSY;
				Description = tr("Set when measuring is running. If set on false measuring is finished");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_SystemBusy, OPCUA_OUTPUT_NAME_SYSTEM_BUSY, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_TRIGGER_READY;
				Description = tr("This field is true when the vision system is Online, and the vision system is not currently acquiringan image");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_TriggerReady, OPCUA_OUTPUT_NAME_TRIGGER_READY, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_INSPECTION_ID;
				Description = tr("Frame number from camera");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_InspectionID, OPCUA_OUTPUT_NAME_INSPECTION_ID, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_CURRENT_JOB_NAME;
				Description = tr("Returns the name of the current active procedure");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_CurrentJobName, OPCUA_OUTPUT_NAME_CURRENT_JOB_NAME, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_JOP_PASS;
				Description = tr("Running of procedure passed without error.If the first result from procedure is bool, this result is included.");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_JobPass, OPCUA_OUTPUT_NAME_JOP_PASS, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				OutputPathAndParameterName = pImagedata->GetName() + QString("/") + OPCUA_OUTPUT_NAME_MODEL_FREE;
				Description = tr("Set when space around the model is free");
				out_config << std::make_shared<OPCOutputValue>(pImagedata->m_ModelFree, OPCUA_OUTPUT_NAME_MODEL_FREE, OutputPathAndParameterName, QCoreApplication::organizationDomain(), Description);
				pImagedata->SetTriggerReady();
			}
		}//end ImageData
	}
	config["type"]          = "server";
	config["logic"]         = QVariant::fromValue(std::dynamic_pointer_cast<Logic>(GetMainAppBDevEngine()->GetMainLogic()));
	config["in_config"]     = QVariant::fromValue(in_config);
	config["out_config"]    = QVariant::fromValue(out_config);
	config["method_config"] = QVariant::fromValue(method_config);
	emit valueChanged(QString("OPCUA/%1").arg(ConfigureOrSetup), config);
}


void BDevEnginePlugin::uninitialize()
{
	m_StartUnInit = true;
	if (GetMainAppBDevEngine())
	{
		delete m_MainAppBDevEngine;
		m_MainAppBDevEngine = NULL;
	}
}


//Wird von der obergeordnetet Instanz aufgerufen
QWidget *BDevEnginePlugin::mainWidget(const int idx) const
{
	switch (idx)
	{
	case TAB_INDEX_MAIN_WIDGET_SETTINGS:
		return (QWidget *)m_BDevEngineSettingsWidget;
		break;
	case TAB_INDEX_IMAGES_OVERVIEW:
		if(m_CurrentNumberCameras==1)
		  return (QWidget *)m_BDevEngineImageOverview[0];// m_BDevEngineImagesOverview;
		else
		{
			if (m_CurrentNumberCameras > 1)
				return (QWidget *)m_BDevEngineImagesOverview;
			else
				return NULL;
		}
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_0:
		return (QWidget *)m_BDevEngineImageOverview[0];
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_1:
		return (QWidget *)m_BDevEngineImageOverview[1];
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_2:
		return (QWidget *)m_BDevEngineImageOverview[2];
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_3:
		return (QWidget *)m_BDevEngineImageOverview[3];
		break;
	default:
		return NULL;
		break;
	}
}


//Legt den Titel f r die verschiedenen GUI-Elemnte fest
const QString BDevEnginePlugin::title(const int idx) const
{
	QString Name;
	switch (idx)
	{
	case TAB_INDEX_MAIN_WIDGET_SETTINGS:
		Name = tr("Settings");
		break;
	case TAB_INDEX_IMAGES_OVERVIEW:
		Name = tr("Overview");
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_0:
		Name = m_CameraNames[0];
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_1:
		Name = m_CameraNames[1];
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_2:
		Name = m_CameraNames[2];
		break;
	case TAB_INDEX_IMAGE_OVERVIEW_3:
		Name = m_CameraNames[3];
		break;
	default:
		Name = tr("NoName");
		break;
	}
	return Name;
}


void BDevEnginePlugin::CameraNameChanged(QString &NewName,int Index)
{
	if (Index >= 0 && Index < MAX_CAMERAS)
	{
		m_CameraNames[Index] = NewName;
		emit valueChanged("updateUi", QVariant());
	}
}

//Aktuelle Kameraparameter direkt an die Kamera
void BDevEnginePlugin::setValue(const QString &name, const QVariant &value)
{
	    QString ErrorMsg;
		if (name.endsWith("/CameraName"))
		{
			m_cameraId = name.split("/").first();
			//emit valueChanged(QString("%1/TriggerMode").arg(m_cameraId), "Off");
			//emit valueChanged(QString("%1/AcquisitionFrameRateEnable").arg(m_cameraId), 1);
			if (GetBDevEngineSettingsWidget())
			{
				GetBDevEngineSettingsWidget()->registerCamera(m_cameraId, true);// , value.toString());
				/*int XOffset = GetPreference(QString("%1/XOffset").arg(m_cameraId)).toInt();
				emit valueChanged(QString("%1/XOffset").arg(m_cameraId), XOffset);
				int YOffset = GetPreference(QString("%1/YOffset").arg(m_cameraId)).toInt();
				emit valueChanged(QString("%1/YOffset").arg(m_cameraId), YOffset);
				int CameraWidth = GetPreference(QString("%1/CameraWidthInPixel").arg(m_cameraId)).toInt();
				if (CameraWidth == 0)
				{
					CameraWidth = 640;
					SetPreference(QString("%1/CameraWidthInPixel").arg(m_cameraId), QVariant(CameraWidth));
				}
				emit valueChanged(QString("%1/Width").arg(m_cameraId), CameraWidth);
				int CameraHeight = GetPreference(QString("%1/CameraHeightInPixel").arg(m_cameraId)).toInt();
				if (CameraHeight == 0)
				{
					CameraHeight = 480;
					SetPreference(QString("%1/CameraHeightInPixel").arg(m_cameraId), QVariant(CameraHeight));
				}
				emit valueChanged(QString("%1/Height").arg(m_cameraId), CameraHeight);
				int ExposureTime = GetPreference(QString("%1/ExposureTime").arg(m_cameraId)).toInt();
				if (ExposureTime == 0)
				{
					ExposureTime = 4000;
				}
				emit valueChanged(QString("%1/ExposureTime").arg(m_cameraId), ExposureTime);
				*/
			}
		}
		
}


void BDevEnginePlugin::setValues(const QHash<QString, QVariant> &values)
{
#ifdef DEBUG
	qDebug() << this << "ignored setValues" << values;
#else
	Q_UNUSED(values);
#endif
}


void BDevEnginePlugin::SetPreference(const QString & preference, const QVariant & value)
{
	setPreference(preference, value);
}


QVariant BDevEnginePlugin::GetPreference(const QString & preference)
{
	return getPreference(preference);
}

//Bild von den Kameras zur Anwendung
void BDevEnginePlugin::consumeImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image)
{
	if (!m_StartUnInit)
	{
		if (GetMainAppBDevEngine())
		{
			ImageData *pImagedata = GetMainAppBDevEngine()->GetImageDataByCameraID(cameraId);
			if (pImagedata)
			{
				pImagedata->SetInspectionID(frameNumber);
				pImagedata->NewIncommingImage(image);
			}
			else
			{
				QString ErrorMsg = tr("No Measure Program Loaded For This Camera:%1").arg(cameraId);
				SetMessage(ErrorMsg, QtMsgType::QtWarningMsg);
			}
		}
	}
}

//Anzeigen unterschiedlicher Fehlertexte enum QtMsgType { QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtInfoMsg, QtSystemMsg = QtCriticalMsg };
void BDevEnginePlugin::SetMessage(const QString &message, QtMsgType MsgType)
{
	bool found = false;
	for (int i = 0; i < m_Messages.count(); i++)
	{
		if (message == m_Messages.at(i).message)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		m_Messages << PluginInterface::Message(m_MessageCounter, message, MsgType);
		PluginInterface::updateMessages(m_Messages);
		m_MessageCounter++;
	}
	if (m_MessageCounter >= 500)
		reset();
}

//Entfernt alle Textnachrichen aus dem Fenster
void BDevEnginePlugin::reset()
{
	clearMessages();
	m_Messages.clear();
	m_MessageCounter = 0;
}


void BDevEnginePlugin::SoftwareTrigger(QString &CameraID)
{
	emit valueChanged(QString("%1/TriggerSoftware").arg(CameraID), "Execute");
}


