#include "halconcpp/HalconCpp.h"
#include "hdevengine/HDevEngineCpp.h"
#include "ImageData.h"
#include "BDEvEngineImageOverview.h"
#include "BDEvEngineImagesOverview.h"
#include "GlobalConst.h"
#include "MainAppBDevEngine.h"
#include "HalconMeasureTool.h"
#include "MainAppBDevEngine.h"
#include "VideoProducer.h"
#include "StreamingImage.h"
#include "CameraSimulation.h"
#include "toojpeg.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include "BDEvEngineSettingsWidget.h"


QByteArray ImageData::m_JpegByteArray;

ImageData::ImageData(MainAppBDevEngine *parent, int Index) : QThread()
, m_MainAppBDevEngine(NULL)
, m_Index(0)
, m_BDevEngineImageOverview(NULL)
, m_TerminateInspection(false)
, m_HalconMeasureTool(NULL)
, m_ReloadMeasureProgram(true)
, m_ImageCounter(0)
, m_ViewRawCameraImage(false)
, m_CameraSimulation(NULL)
, m_FullInspectionTimeInms(0.0)
, m_InspectionCompleted(true)
, m_ReadyMeasuring(true)
, m_LastTriggerCamera(false)
, m_ImageWidth(640)
, m_ImageHeight(480)
, m_HalconBufferWindowID(0)
{
	m_MainAppBDevEngine          = parent;
	m_Index                      = Index;
	m_Name                       = GetCameraIndexName();
	m_MeasureProgramName         = "";
	m_ProcedureName              = "";
	m_CameraSimulationRecordType = IMGAGE_SAVE_CONDITION_ALL_IMAGES;
	//Kommunikation OPCUA
	m_ResultsValid.set(true);//an die SPS, Ergebnisse sind gültig
	m_SystemBusy.set(false);//Messung ist bereit
	m_InspCompleted.set(true);
	m_TriggerReady.set(false);
	m_InspectionID.set(0);//Bildnummer
	m_JobPass.set(false);
	m_ModelFree.set(false);

	m_CameraCalibrationDataPath = GetMainAppBDevEngine()->GetBDevEngineDataPath() + QString("//")+QString("CameraCalibrationDataCam%1/").arg(m_Index);
	QDir().mkpath(m_CameraCalibrationDataPath);
	m_CameraCalibrationDataPathRecording = m_CameraCalibrationDataPath  + QString("RecordingCalibrationData/");
	QDir().mkpath(m_CameraCalibrationDataPathRecording);
	QString   IniFileLocation = GetIniFileLocation();
	QSettings settings(IniFileLocation, QSettings::IniFormat);
	m_ImageWidth  = settings.value(QString("%1/%2").arg(GetCameraIndexName()).arg("ImageWidth"),  m_ImageWidth).toInt();
	m_ImageHeight = settings.value(QString("%1/%2").arg(GetCameraIndexName()).arg("ImageHeight"), m_ImageHeight).toInt();
	
	settings.setValue(QString("%1/%2").arg(GetCameraIndexName()).arg("ImageWidth"), m_ImageWidth);
	settings.setValue(QString("%1/%2").arg(GetCameraIndexName()).arg("ImageHeight"), m_ImageHeight);

	m_HalconMeasureTool = new HalconMeasureTool(this);
	connect(this, &ImageData::SignalShowMessage,               m_MainAppBDevEngine, &MainAppBDevEngine::SlotAddNewMessage);
	connect(this, &ImageData::SignalCheckIsCameraDisable,      m_MainAppBDevEngine, &MainAppBDevEngine::SlotCheckIsCameraDisabled);
	connect(this, &ImageData::SignalProcedureChangedByPLC,     m_MainAppBDevEngine, &MainAppBDevEngine::SlotProcedureChangedByPLC);
	connect(this, &ImageData::SignalViewOutputControlParameter,this,                &ImageData::SlotViewOutputControlParameter);
	connect(this, &ImageData::SignalViewInputControlParameter, this,                &ImageData::SlotViewInputControlParameter);


	qRegisterMetaType<HalconCpp::HImage>("HalconCpp::HImage");
	m_CameraSimulation = new CameraSimulation(this);
	m_BDevEngineImageOverview = new BDevEngineImageOverview(this);//Bildanzeige m_ProcedureName

	connect(this, &ImageData::SignalSetAspectratio, m_BDevEngineImageOverview, &BDevEngineImageOverview::SlotSetAspectRatioNew);
	start();
}


ImageData::~ImageData()
{
	WaitForFinshed();
	if (m_CameraSimulation)
	{
		delete m_CameraSimulation;
		m_CameraSimulation = NULL;
	}
	if (m_HalconMeasureTool)
	{
		delete m_HalconMeasureTool;
		m_HalconMeasureTool = NULL;
	}
	ClearAllProcedureParameter();
}


void ImageData::StepOneImageInSimulation()
{
	if (GetCameraSimulation())
	{
		GetCameraSimulation()->StepImage();
	}
}


void ImageData::SetTriggerReady()
{
	m_TriggerReady.set(true);
}


void ImageData::SetInspectionID(uint32_t value)
{
	m_InspectionID.set(value);
}


bool ImageData::LoadJob(std::string &newJob)
{
	QString JobName = QString().fromStdString(newJob);
	
	if (ExistProcedure(JobName))
	{
		QString CameraName = GetName();
		emit SignalProcedureChangedByPLC(CameraName, JobName);//setzt und speichert den neuen Procedurnamen
		return true;
	}
	else
		return false;
}


QString ImageData::GetCameraIndexName()
{
	return QString("Camera%1").arg(m_Index);
}


QString ImageData::GetIniFileLocation()
{
	return GetMeasureProgramPath() + QString("\\") + GetCameraIndexName() + QString(".ini");
}


void ImageData::SetProcedureName(QString &set)
{
	QString   IniFileLocation = GetIniFileLocation();
	QSettings settings(IniFileLocation, QSettings::IniFormat);
	QString   PathAllImages, PathNIOImages;

	std::string Jobname = set.toStdString();
	m_CurrentJobName.set(Jobname);
	m_ProcedureName = set;

	m_ListImageDirs.clear();
	PathAllImages=settings.value(IMAGE_LOCATION_ARUMENTS(GetCameraIndexName(), GetProcedureName(), IMGAGE_SAVE_CONDITION_ALL_IMAGES)).toString();
	if(!PathAllImages.isEmpty())
		m_ListImageDirs.append(PathAllImages);
	PathNIOImages=settings.value(IMAGE_LOCATION_ARUMENTS(GetCameraIndexName(), GetProcedureName(), IMGAGE_SAVE_CONDITION_ONLY_BAD_IMAGES)).toString();
	if (!PathNIOImages.isEmpty())
		m_ListImageDirs.append(PathNIOImages);
	
}


void ImageData::ClearAllProcedureParameter()
{
	m_ListProcedureIOParameter.clear();
	m_ListInputInt32Values.clear();
	m_ListInputDoubleValues.clear();
	m_ListOutputInt32Values.clear();
	m_ListOutputDoubleValues.clear();
	m_ListOutputBoolValues.clear();
}


int ImageData::LoadAndSetAllProcedures(QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	int ValueMin = 0;
	int ValueMax = 0;
	int IndexOffsetInputControlParameter = 2;//die ersten beiden Paramter Bild und WindowId werden übersprungen. Diese Parameter haben für die SPS keine Bedeutung 
	int IndexOffsetOutputControlParameter = 1;//der erste Paramter Bild wird übersprungen.

	m_MutexMeasuringIsRunning.lock();
	if (!m_MeasureProgramName.isEmpty())
	{
		try
		{
			QString CameraIndexName= GetCameraIndexName();
			HDevEngineCpp::HDevProgram HalconDevProgram;
			QString ProgramNameAndPath = GetMeasureProgramPath() + QString("\\") + GetMeasureProgramName();
			QString ProgramNameAndPathSettings = GetIniFileLocation();
			HalconCpp::HTuple ProcedureNames,Info;
			HDevEngineCpp::HDevProcedure     Procedure;
			QSettings settings(ProgramNameAndPathSettings, QSettings::IniFormat);
			int CountControlParameter;


			HalconDevProgram.LoadProgram(ProgramNameAndPath.toStdString().c_str());
			ProcedureNames = HalconDevProgram.GetLocalProcedureNames();
			ClearAllProcedureParameter();
			for (int i = 0; i < ProcedureNames.Length(); i++)
			{
				ProcedureIOControlParameter IOControlParameter;
				IOControlParameter.m_ProcedureName = QString(QByteArray::fromStdString((std::string)(ProcedureNames[i].S())));
				Procedure.LoadProcedure(HalconDevProgram, IOControlParameter.m_ProcedureName.toStdString().c_str());
				//QString text=Procedure.QueryParamInfo().S();
				CountControlParameter = Procedure.GetInputCtrlParamCount() - (IndexOffsetInputControlParameter-1);
				for (int k = 0; k < CountControlParameter; k++)
				{
					ProcedureInputControlParameter InputControlParameter;
					InputControlParameter.m_Name        = QString(Procedure.GetInputCtrlParamName(k + IndexOffsetInputControlParameter));
					InputControlParameter.m_Type        = Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "default_type").S();
					InputControlParameter.m_Description = Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "description").S();// "description").S();
					if (InputControlParameter.m_Type == HALCON_DATA_TYPE_NAME_STRING)
					{
						InputControlParameter.m_Value = QVariant(Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "default_value").S());
						InputControlParameter.m_Value = settings.value(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_INPUT_PARAMETER,k), InputControlParameter.m_Value);
						//InputControlParameter.m_Value = settings.value(QString("%1_%2/%3Index%4/Value").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(k), InputControlParameter.m_Value);
						m_ListInputStringValues.append(new input<std::string>(InputControlParameter.m_Value.toString().toStdString()));
						InputControlParameter.m_TableIndexOPCUA = m_ListInputStringValues.count() - 1;
					}
					else
					{
						if (InputControlParameter.m_Type == HALCON_DATA_TYPE_NAME_REAL)
						{
							HalconCpp::HTuple temp = Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "default_value");
							InputControlParameter.m_Value = QVariant(QString(temp.S()).toDouble());
							InputControlParameter.m_Value = settings.value(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, k), InputControlParameter.m_Value);
							//InputControlParameter.m_Value = settings.value(QString("%1_%2/%3Index%4/Value").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(k), InputControlParameter.m_Value);
							m_ListInputDoubleValues.append(new input<double>(InputControlParameter.m_Value.toDouble()));
							InputControlParameter.m_TableIndexOPCUA = m_ListInputDoubleValues.count() - 1;
						}
						else
						{
							if (InputControlParameter.m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
							{
								HalconCpp::HTuple HDefaultValue = Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "default_value");//Halconprogramm gibt Defaultwert vor
								HalconCpp::HTuple HValueMin = Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "valuemin");
								HalconCpp::HTuple HValueMax = Procedure.GetInputCtrlParamInfo(k + IndexOffsetInputControlParameter, "valuemax");
								ValueMin = QString(HValueMin.S()).toInt();
								ValueMax = QString(HValueMax.S()).toInt();
								
								InputControlParameter.m_Value = QVariant(QString(HDefaultValue.S()).toInt());
								InputControlParameter.m_Value = settings.value(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, k), InputControlParameter.m_Value);
								//InputControlParameter.m_Value = settings.value(QString("%1_%2/%3Index%4/Value").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(k), InputControlParameter.m_Value);//ist ein anderer als der Defaultwert schon gespeichert
								//In Halcon Datatype bool nicht verfügbar
								if(ValueMin==0 && ValueMax==1)
								{
									m_ListInputBoolValues.append(new input<bool>((bool)(InputControlParameter.m_Value.toInt())));//Instanz einer input Variable mit ensprechenden Startwert bzw. zuletzt benutzen Wert anlegen
									InputControlParameter.m_TableIndexOPCUA = m_ListInputBoolValues.count() - 1;//Speichern des Indexes um später drauf zuzugreifen
									InputControlParameter.m_IsBool = true;
     							}
								else
								{
								     m_ListInputInt32Values.append(new input<int32_t>(InputControlParameter.m_Value.toInt()));//Instanz einer input Variable mit ensprechenden Startwert bzw. zuletzt benutzen Wert anlegen
									 InputControlParameter.m_TableIndexOPCUA = m_ListInputInt32Values.count() - 1;//Speichern des Indexes um später drauf zuzugreifen
								}
							}
						}
					}
					//settings.setValue(QString("%1_%2/%3Index%4/Name").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(k), InputControlParameter.m_Name);
					settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_NAME(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, k), InputControlParameter.m_Name);
					//settings.setValue(QString("%1_%2/%3Index%4/Type").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(k), InputControlParameter.m_Type);
					settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_TYPE(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, k), InputControlParameter.m_Type);
					//settings.setValue(QString("%1_%2/%3Index%4/Value").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(k), InputControlParameter.m_Value);
					settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, k), InputControlParameter.m_Value);
					IOControlParameter.m_ListInputControlParameter.append(InputControlParameter);
				}
				CountControlParameter = Procedure.GetOutputCtrlParamCount() - (IndexOffsetOutputControlParameter -1);
				for (int j = 0; j < CountControlParameter; j++)
				{
					ProcedureOutputControlParameter OutputControlParameter;
					OutputControlParameter.m_Name        = QString(Procedure.GetOutputCtrlParamName(j + IndexOffsetOutputControlParameter));
					OutputControlParameter.m_Type        = Procedure.GetOutputCtrlParamInfo(j + IndexOffsetOutputControlParameter, "default_type").S();
					//std::string temp                     = Procedure.GetOutputCtrlParamInfo(j + IndexOffsetOutputControlParameter, "description").S();
					OutputControlParameter.m_Description = QString(Procedure.GetOutputCtrlParamInfo(j + IndexOffsetOutputControlParameter, "description").S());
					if (OutputControlParameter.m_Type == HALCON_DATA_TYPE_NAME_STRING)
					{
						m_ListOutputStringValues.append(new output<std::string>);
						OutputControlParameter.m_TableIndexOPCUA = m_ListOutputStringValues.count() - 1;
				    }
					else
					{
						if (OutputControlParameter.m_Type == HALCON_DATA_TYPE_NAME_REAL)
						{
							m_ListOutputDoubleValues.append(new output<double>);
							OutputControlParameter.m_TableIndexOPCUA = m_ListOutputDoubleValues.count() - 1;
						}
						else
						{
							if (OutputControlParameter.m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
							{
								HalconCpp::HTuple HValueMin = Procedure.GetOutputCtrlParamInfo(j + IndexOffsetOutputControlParameter, "valuemin");
								HalconCpp::HTuple HValueMax = Procedure.GetOutputCtrlParamInfo(j + IndexOffsetOutputControlParameter, "valuemax");
								ValueMin = QString(HValueMin.S()).toInt();
								ValueMax = QString(HValueMax.S()).toInt();
								if (ValueMin == 0 && ValueMax == 1)
								{//wird hier als bool benutzt
									m_ListOutputBoolValues.append(new output<bool>);
									OutputControlParameter.m_TableIndexOPCUA = m_ListOutputBoolValues.count() - 1;
									OutputControlParameter.m_IsBool = true;
								}
								else
								{
									m_ListOutputInt32Values.append(new output<int32_t>);
									OutputControlParameter.m_TableIndexOPCUA = m_ListOutputInt32Values.count() - 1;
								}
							}
						}
					}
					//settings.setValue(QString("%1_%2/%3Index%4/Name").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_OUTPUT_PARAMETER).arg(j), OutputControlParameter.m_Name);
					settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_NAME(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_OUTPUT_PARAMETER, j), OutputControlParameter.m_Name);
					//settings.setValue(QString("%1_%2/%3Index%4/Type").arg(CameraIndexName).arg(IOControlParameter.m_ProcedureName).arg(SECTION_NAME_OUTPUT_PARAMETER).arg(j), OutputControlParameter.m_Type);
					settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_TYPE(CameraIndexName, IOControlParameter.m_ProcedureName, SECTION_NAME_OUTPUT_PARAMETER, j), OutputControlParameter.m_Type);
					IOControlParameter.m_ListOutputControlParameter.append(OutputControlParameter);
				}
				m_ListProcedureIOParameter.insert(IOControlParameter.m_ProcedureName, IOControlParameter);
			}//end all procedures
			ProgramNameAndPath = "";
    	}
		catch (HDevEngineCpp::HDevEngineException &hdev_exception)
		{	
			ErrorMsg = hdev_exception.Message();
			rv = ERROR_CODE_ANY_ERROR;
		}
	}
	m_MutexMeasuringIsRunning.unlock();
	return rv;
}


bool ImageData::ProceduresLoaded()
{
	if (m_ListProcedureIOParameter.count() > 0)
		return true;
	else
		return false;
}


void ImageData::ReadInputControlParameterFromOPCUA()
{
	ProcedureIOControlParameter *pControlData = NULL;
	double DoubleInputValueFromPLC;
	int IntegerInputValueFromPLC;
	bool BoolValueFromPLC;
	std::string StringValueFromPLC;
	QString ProgramNameAndPathSettings = GetIniFileLocation();
	bool NewInputValue = false;

	
	pControlData = GetIOControlParameter();
	if (pControlData)
	{
			NewInputValue = false;
			for (int i = 0; i < pControlData->m_ListInputControlParameter.count(); i++)
			{
				if (pControlData->m_ListInputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_STRING)
				{
					StringValueFromPLC = m_ListInputStringValues.at(pControlData->m_ListInputControlParameter.at(i).m_TableIndexOPCUA)->get();
					if (pControlData->m_ListInputControlParameter[i].m_Value.toString().toStdString() != StringValueFromPLC)
					{
						QSettings settings(ProgramNameAndPathSettings, QSettings::IniFormat);
						QString CameraIndexName = GetCameraIndexName();
						pControlData->m_ListInputControlParameter[i].m_Value = QString::fromStdString(StringValueFromPLC);
						settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, pControlData->m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, i), pControlData->m_ListInputControlParameter[i].m_Value);
						//settings.setValue(QString("%1/%2/Index%3/Value").arg(pControlData->m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(i), pControlData->m_ListInputControlParameter[i].m_Value);
						NewInputValue = true;
					}
				}
				else
				{
					if (pControlData->m_ListInputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_REAL)
					{
						DoubleInputValueFromPLC = m_ListInputDoubleValues.at(pControlData->m_ListInputControlParameter.at(i).m_TableIndexOPCUA)->get();
						if (pControlData->m_ListInputControlParameter[i].m_Value.toDouble() != DoubleInputValueFromPLC)
						{//Wert ist von der SPS verändert worden. Neuen Wert übernehmen und speichern
							QSettings settings(ProgramNameAndPathSettings, QSettings::IniFormat);
							QString CameraIndexName = GetCameraIndexName();
							pControlData->m_ListInputControlParameter[i].m_Value = DoubleInputValueFromPLC;
							settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, pControlData->m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, i), pControlData->m_ListInputControlParameter[i].m_Value);
    						//settings.setValue(QString("%1/%2Index%3/Value").arg(pControlData->m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(i), pControlData->m_ListInputControlParameter[i].m_Value);
							NewInputValue = true;
						}
					}
					else
					{
						if (pControlData->m_ListInputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
						{
							if (!pControlData->m_ListInputControlParameter.at(i).m_IsBool)
							{
								IntegerInputValueFromPLC = m_ListInputInt32Values.at(pControlData->m_ListInputControlParameter.at(i).m_TableIndexOPCUA)->get();
								if (pControlData->m_ListInputControlParameter[i].m_Value.toInt() != IntegerInputValueFromPLC)
								{//Wert ist von der SPS verändert worden. Neuen Wert übernehmen und speichern
									QSettings settings(ProgramNameAndPathSettings, QSettings::IniFormat);
									QString CameraIndexName = GetCameraIndexName();
									pControlData->m_ListInputControlParameter[i].m_Value = IntegerInputValueFromPLC;
									settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, pControlData->m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, i), pControlData->m_ListInputControlParameter[i].m_Value);
									//settings.setValue(QString("%1/%2/Index%3/Value").arg(pControlData->m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(i), pControlData->m_ListInputControlParameter[i].m_Value);
									NewInputValue = true;
								}
							}
							else
							{
								BoolValueFromPLC = m_ListInputBoolValues.at(pControlData->m_ListInputControlParameter.at(i).m_TableIndexOPCUA)->get();
								if (pControlData->m_ListInputControlParameter[i].m_Value.toBool() != BoolValueFromPLC)
								{
									QSettings settings(ProgramNameAndPathSettings, QSettings::IniFormat);
									QString CameraIndexName = GetCameraIndexName();
									pControlData->m_ListInputControlParameter[i].m_Value = BoolValueFromPLC;
									settings.setValue(IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName, pControlData->m_ProcedureName, SECTION_NAME_INPUT_PARAMETER, i), pControlData->m_ListInputControlParameter[i].m_Value);
									//settings.setValue(QString("%1/%2/Index%3/Value").arg(pControlData->m_ProcedureName).arg(SECTION_NAME_INPUT_PARAMETER).arg(i), pControlData->m_ListInputControlParameter[i].m_Value);
									NewInputValue = true;
								}
							}
						}
					}
				}
			}
			if(NewInputValue)
			   SetSignalViewInputControlParameter();//Neuer Paramterwert in die Anzeige
	}
}


bool ImageData::TriggerAcquisition()
{
	bool rv=SoftwareTrigger();//Kommando an die Kamera starte Bildaufnahme
	if(rv)
		m_TriggerReady.set(false);
	return rv;
}


void ImageData::SetupControlParameterWidget()
{
	if (GetBDevEngineImageOverview())
		GetBDevEngineImageOverview()->SetupControlParameterWidget();
}


void ImageData::StartTimerMeasureFullInspectionTime()
{
	m_TimerMeasureFullInspectionTime.start();
	m_FullInspectionTimeInms = 0.0;
}


void ImageData::StopTimerMeasureFullInspectionTime()
{
	m_FullInspectionTimeInms = m_TimerMeasureFullInspectionTime.nsecsElapsed() / 1000000.0;
	emit SignalShowInspectionTime(m_FullInspectionTimeInms);
}


HalconCpp::HTuple ImageData::GetWindowID()
{
	/*if (GetBDevEngineImageOverview())
		return GetBDevEngineImageOverview()->GetWindowID();
	else
		return HalconCpp::HTuple();
*/

	return m_HalconBufferWindowID;
}


QString ImageData::GetMeasureProgramPath()
{
	if (GetMainAppBDevEngine())
		return GetMainAppBDevEngine()->GetMeasureProgramPath();
	else
		return QString();
}


void ImageData::NewIncommingImage(const cv::Mat &image)
{
	if (isRunning())
	{
		QString ErrorMsg;
		if (AddToHalconImage(image.rows, image.cols, image.data, ErrorMsg) != ERROR_CODE_NO_ERROR)
			emit SignalShowMessage(ErrorMsg, QtMsgType::QtFatalMsg);
	}
	else
	{//wird Bildeinzug unterbrochen oder Thread läuft nicht, dann werden die Bilder gelöscht
		m_MutexNewImage.lock();
		while (!m_QQueueIncommingImages.isEmpty())
			    m_QQueueIncommingImages.dequeue();
		m_MutexNewImage.unlock();
	}
}


int ImageData::AddToHalconImage(int rows, int cols, unsigned char *data,QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	try
	{//Bild nach Halcon copieren
		if (m_ReadyMeasuring)
		{
			m_ReadyMeasuring = false;
			m_MutexNewImage.lock();
			HalconCpp::HImage  HalconCameraImage;
			HalconCameraImage.GenImage1("byte", static_cast<Hlong>(cols), static_cast<Hlong>(rows), (void*)(data));
			m_QQueueIncommingImages.enqueue(HalconCameraImage);
			m_MutexNewImage.unlock();
			m_WaitConditionNewImage.wakeAll();//Signal an den Thread neues Bild von der Kamera starte Messung
		}
	}
	catch (HalconCpp::HException &exception)
	{
		ErrorMsg =tr("Error/License Error Input Image. In Funktion:%1 %2 %3").arg(exception.ErrorCode()).arg((const char *)exception.ProcName()).arg((const char *)exception.ErrorMessage());
		rv = ERROR_CODE_ANY_ERROR;
	}
	return rv;
}

//Thread der die eingehenden Bilder von der Kamera verarbeitet. 
void ImageData::run()
{
	QMutex Mutex;
	QString ErrorMsg;
	int rv= ERROR_CODE_NO_ERROR;
	HalconCpp::HImage  HalconRawCameraImage;
	HalconCpp::HImage  HalconDisplayImage;
	Hlong  ImageWidth = 0;
	Hlong  ImageHeight = 0;
	
	while (!m_TerminateInspection)
	{
		if (m_QQueueIncommingImages.isEmpty())
		{//wenn kein Bild in der Queue dann warten
			Mutex.lock();
			if (!m_WaitConditionNewImage.wait(&Mutex, 5000))
				emit SignalCheckIsCameraDisable(m_Index);
			Mutex.unlock();
		}
		if(!m_QQueueIncommingImages.isEmpty())
		{
			m_ResultsValid.set(false);//an die SPS Ergebnisse sind nicht mehr gültig
			m_SystemBusy.set(true);//Messung wird gestartet
			m_JobPass.set(false);
			m_ModelFree.set(false);
			if (m_MutexNewImage.tryLock())
			{
				StartTimerMeasureFullInspectionTime();
				HalconRawCameraImage = m_QQueueIncommingImages.dequeue();
				HalconRawCameraImage.GetImageSize(&ImageWidth, &ImageHeight);
				if (m_HalconBufferWindowID == 0)
				{
					HalconCpp::OpenWindow(0, 0, ImageWidth, ImageHeight, 0, "invisible", "", &m_HalconBufferWindowID);
					HalconCpp::SetWindowParam(m_HalconBufferWindowID, "flush", "false");
					HalconCpp::SetPart(m_HalconBufferWindowID, 0, 0, ImageHeight - 1, ImageWidth - 1);
				    HalconCpp::SetWindowExtents(m_HalconBufferWindowID, 0, 0, ImageWidth, ImageHeight);
				}
				HalconCpp::DispImage(HalconRawCameraImage, m_HalconBufferWindowID);
				m_MutexMeasuringIsRunning.lock();
				if (m_ReloadMeasureProgram)
				{
						rv = m_HalconMeasureTool->LoadMeasureProgram(ErrorMsg);
						if (rv == ERROR_CODE_NO_ERROR)
							m_ReloadMeasureProgram = false;
						else
							emit SignalShowMessage(ErrorMsg, QtMsgType::QtFatalMsg);
				}
				if (rv == ERROR_CODE_NO_ERROR)
				{//Starten des Messprogramms und Anzeige von Zwischenergebnissen
					    if (GetProcedureName() == "CalibrateCamera")
					    {
							QString LocationAndName = GetCameraCalibrationDataPathRecording() + QString("RecordImage%1").arg(GetImageCounter());
							HalconRawCameraImage.WriteImage("jpg", 0, LocationAndName.toLatin1().data());
						}
						rv = m_HalconMeasureTool->ExecuteMeasureTool(HalconRawCameraImage, ErrorMsg);
						if (rv == ERROR_CODE_NO_ERROR)
						{
							m_ImageCounter++;
							StopTimerMeasureFullInspectionTime();
							//rv=DrawAdditinalResultsAndStreamImage(HalconDisplayImage, ErrorMsg);
							if (rv != ERROR_CODE_NO_ERROR)
								emit SignalShowMessage(ErrorMsg, QtMsgType::QtFatalMsg);
							ResultsToPLC(HalconRawCameraImage);
						}
						else
							emit SignalShowMessage(ErrorMsg, QtMsgType::QtFatalMsg);
				}
				m_MutexMeasuringIsRunning.unlock();
				HalconCpp::FlushBuffer(m_HalconBufferWindowID);
				HalconCpp::DumpWindowImage(&HalconDisplayImage, m_HalconBufferWindowID);
				//HalconCpp::ZoomImageFactor(HalconDisplayImage, &HalconDisplayImage, ScaleWidth, ScaleHeight, "constant");
				//QString LocationAndName = "d:/Temp/DumpImage.jpg";
				//(HalconDisplayImage.WriteImage("jpg", 0, LocationAndName.toLatin1().data());
				DisplayImageAndOverviewImage(HalconDisplayImage, ErrorMsg);
				
				if (rv != ERROR_CODE_NO_ERROR)
				    m_JobPass.set(false);
		    	m_MutexNewImage.unlock();
			}
			m_ReadyMeasuring = true;
			m_SystemBusy.set(false);//Messung zu ende
			m_TriggerReady.set(true);//Trigger ist freigegeben
     	}
    }
	m_WaitConditionLiveViewIsDisable.wakeAll();
}


void ImageData::ResultsToPLC(HalconCpp::HImage &CameraRawImage)
{
	if (GetMainAppBDevEngine())
	{
		QString ErrorMsg,Name, StringRecordImages;
		QString ProcedureName=GetProcedureName();
		ProcedureIOControlParameter *pResultData = GetIOControlParameter();
		double DoubleValue;
		int32_t IntegerValue;
		bool BoolValue;
		std::string StringValue;
		int rv = ERROR_CODE_NO_ERROR;

		if (pResultData)
		{
			for (int i = 0; i < pResultData->m_ListOutputControlParameter.count(); i++)
			{
				if (pResultData->m_ListOutputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_STRING)
				{
					StringValue = pResultData->m_ListOutputControlParameter.at(i).m_Value.toString().toStdString();
					m_ListOutputStringValues.at(pResultData->m_ListOutputControlParameter.at(i).m_TableIndexOPCUA)->set(StringValue);
				}
				else
				{
					if (pResultData->m_ListOutputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_REAL)
					{
						DoubleValue = pResultData->m_ListOutputControlParameter.at(i).m_Value.toDouble();
						m_ListOutputDoubleValues.at(pResultData->m_ListOutputControlParameter.at(i).m_TableIndexOPCUA)->set(DoubleValue);
	     			}
					else
					{
						if (pResultData->m_ListOutputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
						{
							if (pResultData->m_ListOutputControlParameter.at(i).m_IsBool)
							{
								BoolValue = pResultData->m_ListOutputControlParameter.at(i).m_Value.toBool();
								if (pResultData->m_ListOutputControlParameter.at(i).m_Name == OPCUA_OUTPUT_NAME_JOP_PASS)
									m_JobPass.set(BoolValue);
								else
								{
									if (pResultData->m_ListOutputControlParameter.at(i).m_Name == OPCUA_OUTPUT_NAME_MODEL_FREE)
										m_ModelFree.set(BoolValue);
								}
								m_ListOutputBoolValues.at(pResultData->m_ListOutputControlParameter.at(i).m_TableIndexOPCUA)->set(BoolValue);
							}
							else
							{
								IntegerValue = pResultData->m_ListOutputControlParameter.at(i).m_Value.toInt();
								m_ListOutputInt32Values.at(pResultData->m_ListOutputControlParameter.at(i).m_TableIndexOPCUA)->set(IntegerValue);
							}
							
						}
					}
				}
			}
		}
		/*  "none" : Do not record images
			"all"  : All images
			"nio"  : Only when JobPass == 0
		*/
		StringRecordImages = QString().fromStdString(m_RecordImages.get());
		if(StringRecordImages == IMGAGE_SAVE_CONDITION_ONLY_BAD_IMAGES)
		{
			if(!m_JobPass.get())
			   rv=SaveImage(CameraRawImage, StringRecordImages, ErrorMsg);
		}
		else
		{
			if (StringRecordImages == IMGAGE_SAVE_CONDITION_ALL_IMAGES)
    			rv=SaveImage(CameraRawImage, StringRecordImages, ErrorMsg);
		}
		if (rv != ERROR_CODE_NO_ERROR)
			emit SignalShowMessage(ErrorMsg, QtMsgType::QtFatalMsg);
		m_ResultsValid.set(true);//Info an die SPS Ergebnisse sind jetzt gültig
	    m_InspectionCompleted = !m_InspectionCompleted;
		m_InspCompleted.set(m_InspectionCompleted);
		
    }
}


bool ImageData::ExistProcedure(QString &ProcedureName)
{
	return m_ListProcedureIOParameter.contains(ProcedureName);
}


ProcedureIOControlParameter *ImageData::GetIOControlParameter()
{
	QString ProcedureName = GetProcedureName();
	if (m_ListProcedureIOParameter.contains(ProcedureName))
		return &(m_ListProcedureIOParameter[ProcedureName]);
	else
		return NULL;
}


void ImageData::SetSignalViewOutputControlParameter()
{
	emit SignalViewOutputControlParameter();
}


void ImageData::SlotViewOutputControlParameter()
{
	if (GetBDevEngineImageOverview())
		GetBDevEngineImageOverview()->ViewNewOutputControlParameter();//Anzeige der Ergebnisse in die GUI
}


void ImageData::SetSignalViewInputControlParameter()
{
	emit SignalViewInputControlParameter();
}


void ImageData::SlotViewInputControlParameter()
{
	if (GetBDevEngineImageOverview())
		GetBDevEngineImageOverview()->ViewNewInputControlParameter();//Anzeige der Parameter in die GUI
}


QString ImageData::GetPathDXFFiles()
{
	if (GetMainAppBDevEngine())
		return GetMainAppBDevEngine()->GetPathDXFFiles();
	else
		return QString();
}


int  ImageData::DisplayImageAndOverviewImage(HalconCpp::HImage Image,QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	try
	{
		HalconCpp::HImage DImage = Image.CopyImage();
		//Anzeige des Bildes
		if (GetBDevEngineImageOverview())
			rv=DisplayImage(DImage, GetBDevEngineImageOverview()->GetWindowID(), ErrorMsg);
		//Wenn mehr als eine Kamera angeschlossen dann Anzeige auch in das Übersichtsfenster
		if (GetMainAppBDevEngine())
		{
			if (GetMainAppBDevEngine()->GetCurrentNumberCameras() > 1 && GetMainAppBDevEngine()->GetBDevEngineImagesOverview())
			{
				HalconCpp::HTuple WindowID=GetMainAppBDevEngine()->GetBDevEngineImagesOverview()->GetWindowID(m_Index);
				rv=DisplayImage(DImage, WindowID, ErrorMsg);
			}
		}
    }
    catch (HDevEngineCpp::HDevEngineException &hdev_exception)
    {   //DispMessage(hdev_exception.Message());
	    QString ErrorMsg = hdev_exception.Message();
		rv=ERROR_CODE_ANY_ERROR;
    }
	return rv;
}


int ImageData::DisplayImage(HalconCpp::HImage Image, HalconCpp::HTuple WindowID,QString &ErrorMsg)
{
		Hlong  ImageWidth = 0;
		Hlong  ImageHeight = 0;
		int rv = ERROR_CODE_NO_ERROR;

		try
		{
			Image.GetImageSize(&ImageWidth, &ImageHeight);
			if (ImageWidth > 0 && ImageHeight > 0)
			{
				if (WindowID != 0)
				{
					HalconCpp::SetPart(WindowID, 0, 0, ImageHeight - 1, ImageWidth - 1);
					//HalconCpp::DispImage(Image, WindowID);
					HalconCpp::DispColor(Image, WindowID);
					HalconCpp::FlushBuffer(WindowID);
				}
			}
		}
		catch (HDevEngineCpp::HDevEngineException &hdev_exception)
		{   //DispMessage(hdev_exception.Message());
			QString ErrorMsg = hdev_exception.Message();
			rv = ERROR_CODE_ANY_ERROR;
		}
		return rv;
}


int ImageData::DrawAdditinalResultsAndStreamImage(HalconCpp::HImage CameraRawImage, QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	if(GetBDevEngineImageOverview())
	{
		rv=ForcePaintResultOnImageView(GetBDevEngineImageOverview()->GetWindowID(), ErrorMsg);
	}
	if (GetMainAppBDevEngine() && rv== ERROR_CODE_NO_ERROR)
	{
		if (GetMainAppBDevEngine()->GetCurrentNumberCameras() > 1 && GetMainAppBDevEngine()->GetBDevEngineImagesOverview())
		{
			HalconCpp::HTuple WindowID = GetMainAppBDevEngine()->GetBDevEngineImagesOverview()->GetWindowID(m_Index);
			rv=ForcePaintResultOnImageView(WindowID, ErrorMsg);
		}
    }
	if(rv == ERROR_CODE_NO_ERROR)
	   rv=StreamViewImage(GetBDevEngineImageOverview()->GetWindowID(), CameraRawImage,ErrorMsg);
	return rv;
}


int ImageData::ForcePaintResultOnImageView(HalconCpp::HTuple WindowID,QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	if (WindowID != 0)
	{
		try
		{
		    QString InspectTime = QString("Time:%1ms").arg(m_FullInspectionTimeInms,0,'f',2);
			HalconCpp::SetColor(WindowID, "blue");
			HalconCpp::SetTposition(WindowID, 0, 440);
			HalconCpp::WriteString(WindowID, InspectTime.toLatin1().data());
			if (GetCameraID() == SIMULATION_NAME_CAMERA)
			{
				if (GetCameraSimulation())
				{
					QString CurrentPathAndFilename=GetCameraSimulation()->GetCurrentFilename();
					QString CurrentFile = CurrentPathAndFilename.mid(CurrentPathAndFilename.lastIndexOf("/") + 1);
					HalconCpp::SetTposition(WindowID, 460, 0);
					HalconCpp::WriteString(WindowID, CurrentFile.toLatin1().data());
				}
			}
			HalconCpp::FlushBuffer(WindowID); //ForcePaint
		}
		catch (HDevEngineCpp::HDevEngineException &hdev_exception)
		{   //DispMessage(hdev_exception.Message());
			ErrorMsg = hdev_exception.Message();
			rv = ERROR_CODE_ANY_ERROR;
		}
	}
	else
	{
		ErrorMsg = tr("Can Not Paint Results! WinID Not Valid");
		//rv = ERROR_CODE_ANY_ERROR;
	}
	return rv;
}


int ImageData::StreamViewImage(HalconCpp::HTuple WindowID, HalconCpp::HImage CameraRawImage, QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	
	if (GetMainAppBDevEngine() && GetMainAppBDevEngine()->GetStreamingImage())
	{
		if (WindowID != 0)
		{
			try
			{
				int size, resolution,w,h,bytesPerPixel = 3;
				unsigned char *data = NULL;
				HalconCpp::HImage DumpImage;
				HalconCpp::HTuple Type, W, H, R, G, B;
				unsigned char *r, *g, *b;
				bool ok=true;
				//double Y, U, V;
				if (m_ViewRawCameraImage)
					HalconCpp::Compose3(CameraRawImage, CameraRawImage, CameraRawImage, &DumpImage);
				else
					HalconCpp::DumpWindowImage(&DumpImage, WindowID);
			    GetImagePointer3(DumpImage, &R, &G, &B, &Type, &W, &H);
		        w = (Hlong)(W);
		        h = (Hlong)(H);
				resolution = w * h;
		    	size = resolution * bytesPerPixel;
				data = new unsigned char[size];
				m_JpegByteArray.clear();
				if (resolution > 0)
				{
					auto offset = 0;
					r = (unsigned char *)(R.L());
					g = (unsigned char *)(G.L());
					b = (unsigned char *)(B.L());
					for (auto y = 0; y < h; y++)
						for (auto x = 0; x < w; x++)
						{	// memory location of current pixel
							offset = (y * w + x) * bytesPerPixel;
							data[offset] = *r;
							data[offset + 1] = *g;
							data[offset + 2] = *b;
							b++;
							g++;
							r++;
							//YUVfromRGB(Y, U, V, *r, *g, *b);
							m_JpegByteArray.append(data[offset]);
							m_JpegByteArray.append(data[offset + 1]);
							m_JpegByteArray.append(data[offset + 2]);
						}
				}
				
				//ok = TooJpeg::writeJpeg(CallBackJpegByte, data, w, h,true);// , isRGB, quality, downsample, comment);
				if (data)
					delete [] data;
				if (ok)
				{
					//if(GetMainAppBDevEngine()->GetVideoProducer())
					  // GetMainAppBDevEngine()->GetVideoProducer()->SetNewJpegRawData(m_JpegByteArray);

					
						GetMainAppBDevEngine()->GetStreamingImage()->SetNewJpegRawData(m_JpegByteArray);
				}
			}
			catch (HDevEngineCpp::HDevEngineException &hdev_exception)
			{   //DispMessage(hdev_exception.Message());
				ErrorMsg = hdev_exception.Message();
				rv = ERROR_CODE_ANY_ERROR;
			}
		}
		else
		{
			ErrorMsg = tr("Can Not Stream Image(Camera Name %1)! WinID Not Valid").arg(GetName());
			//rv = ERROR_CODE_ANY_ERROR;
		}
	}
	return rv;
}


void ImageData::WriteByteStram(QByteArray &ByteStream)
{
	QString Path = "d://temp//Iamges//";
	QDir().mkpath(Path);
	QString ImageName = QString("Image%1.bmp").arg(m_ImageCounter);

	Path = Path + ImageName;
	char* data = ByteStream.data();
	QImage img((uchar*)data, 640, 480, QImage::Format_RGB888);

	img.save(Path);
	
}

void ImageData::YUVfromRGB(double& Y, double& U, double& V, const double R, const double G, const double B)
{
	Y = 0.257 * R + 0.504 * G + 0.098 * B + 16;
	U = -0.148 * R - 0.291 * G + 0.439 * B + 128;
	V = 0.439 * R - 0.368 * G - 0.071 * B + 128;
}


void ImageData::WaitForFinshed()
{
	if (isRunning())
	{
		m_TerminateInspection = true;
		m_WaitConditionNewImage.wakeAll();
		m_WaitLiveImageViewIsDisable.lock();
		m_WaitConditionLiveViewIsDisable.wait(&m_WaitLiveImageViewIsDisable, 5000);//warte bis livebildanzeige beendet
		m_WaitLiveImageViewIsDisable.unlock();
		wait(100);
		if (isRunning())
			terminate();
	}
}


bool ImageData::SoftwareTrigger()
{
	QString CamID=GetCameraID();
	bool rv = false;
	if (CamID != DISABLED_NAME_CAMERA && CamID != SIMULATION_NAME_CAMERA)
	{
		if (GetMainAppBDevEngine())
		{
			GetMainAppBDevEngine()->SoftwareTrigger(CamID);
			rv = true;
		}
    }
	else
	{
		QString Msg = tr("Can Not Trigger Camera! Camera Is In %1 State").arg(CamID);
		emit SignalShowMessage(Msg, QtMsgType::QtWarningMsg);
	}
	return rv;
}


void ImageData::SetCameraID(QString &set)
{
	m_CameraID = set; 
	if (m_CameraID == SIMULATION_NAME_CAMERA)
	    StartCameraSimulation(true);
    else
		StartCameraSimulation(false);//stop simu
}


int ImageData::SaveImage(HalconCpp::HImage &CameraRawImage, QString &SubPath,QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	QString   IniFileLocation = GetIniFileLocation();
	QSettings settings(IniFileLocation, QSettings::IniFormat);
	QString   ImageRootPath=QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	QString   ImageCameraPath = ImageRootPath + QString("/") + GetName();
	QString   ImageCameraPathSubPath = ImageCameraPath + QString("/") + GetProcedureName() + QString("/") + SubPath;


	QString LocationAndName = QDir(ImageCameraPathSubPath).filePath(tr("Image") + " " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh.mm.ss") + ".bmp");
	QDir().mkpath(ImageCameraPathSubPath);


	if (!m_ListImageDirs.contains(ImageCameraPathSubPath))
	{
		QString CameraIndexName = QString("Camera%1").arg(m_Index);
		m_ListImageDirs.append(ImageCameraPathSubPath);//Is used for checking Diskspace
		settings.setValue(IMAGE_LOCATION_ARUMENTS(CameraIndexName,GetProcedureName(),SubPath), ImageCameraPathSubPath);
		//settings.setValue(QString("%1_%2_ImageLocation/Path_%3").arg(CameraIndexName).arg(GetProcedureName()).arg(SubPath), ImageCameraPathSubPath);
    }
	//CheckImageDir(ImageCameraPathSubPath);
	try
	{
		CameraRawImage.WriteImage("bmp", 0, LocationAndName.toLatin1().data());
	}
	catch (HDevEngineCpp::HDevEngineException &hdev_exception)
	{   //DispMessage(hdev_exception.Message());
		ErrorMsg = hdev_exception.Message();
		rv = ERROR_CODE_ANY_ERROR;
	}
	return rv;
}


void ImageData::CheckImageDir(QString &Dir)
{
	QDir dir(Dir);
	QStringList totalDirs;

	totalDirs = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::Time | QDir::Reversed);
	int diff = totalDirs.count() - GetMainAppBDevEngine()->GetMaxImagesSaveOnDiskPerCamera() + 1;

	if (diff >= 0)
	{
		for (int i = 0; i < diff; i++)
		{
			QDir dir(Dir + QString("/") + totalDirs[i]);
			dir.removeRecursively();//Lösche ältesten datensätze FIFO Prinzip
		}
	}
}

//Startet und beendet die Kamerasimulation
void ImageData::StartCameraSimulation(bool Start)
{
	if (GetCameraSimulation())
	{
		if (Start)
		{
			if (m_ListImageDirs.count() > 0)
			{
				bool found = false;
				for(int i = 0; i < m_ListImageDirs.count(); i++)
				{
					QString RecordType = m_ListImageDirs.at(i).mid(m_ListImageDirs.at(i).lastIndexOf("/")+1);
					if (RecordType == m_CameraSimulationRecordType)
					{
						QString FileLocation = m_ListImageDirs.at(i);
						if (!GetCameraSimulation()->isRunning())
							GetCameraSimulation()->StartSimulation(FileLocation);
						found = true;
						break;
					}
				}
				if (!found)
				{
					QString ErrorMsg = tr("Can not run camera simulation no files on disk! Camera:%1 Recordtype:%2").arg(GetName()).arg(m_CameraSimulationRecordType);
				}
			}
			else
			{
				QString ErrorMsg = tr("Can not run camera simulation no files on disk! Camera:%1").arg(GetName());
			}
		}
		else
		{
			if (GetCameraSimulation()->isRunning())
				GetCameraSimulation()->WaitForFinshed();
		}
	}
}


void ImageData::CallBackJpegByte(unsigned char byte)
{
	m_JpegByteArray.append(byte);
}





