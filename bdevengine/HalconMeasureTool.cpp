#include "HalconMeasureTool.h"
#include "my_error_output.h"
#include "ImageData.h"
#include "GlobalConst.h"
#include "ProcedureIOControlParameter.h"
#include "qvariant.h"


HalconMeasureTool::HalconMeasureTool(ImageData *pImageData, QObject *parent) : QObject(parent)
, m_Procedure(NULL)
, m_ProcCall(NULL)
{
	m_ImageData = pImageData;
}


HalconMeasureTool::~HalconMeasureTool()
{
	if (m_ProcCall)
	{
		delete m_ProcCall;
		m_ProcCall = NULL;
	}
	if (m_Procedure)
	{
		delete m_Procedure;
		m_Procedure = NULL;
	}
}


int HalconMeasureTool::LoadMeasureProgram(QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	QString ProgramPathAndName = GetImageData()->GetMeasureProgramPath() + QString("\\") + GetImageData()->GetMeasureProgramName();
	QString ProcedureName      = GetImageData()->GetProcedureName();
	HalconCpp::HTuple ProcedureNames;
	bool isIn = false;
	QStringList ListAllProcdeures;

	try
	{
		m_DevProgram.LoadProgram(ProgramPathAndName.toStdString().c_str());
		ProcedureNames = m_DevProgram.GetLocalProcedureNames();//Alle Proceduren im Messprogramm laden
		for (int i = 0; i < ProcedureNames.Length(); i++)
			ListAllProcdeures.append(QString(QByteArray::fromStdString((std::string)(ProcedureNames[i].S()))));
		//check is Procedure in Measureprogram
		for (int i = 0; i < ListAllProcdeures.count(); i++)
		{
			if (ListAllProcdeures.at(i) == ProcedureName)
			{
				isIn = true;
				break;
			}
		}
		if (isIn)
		{//die Procedure ist im Messprogramm enthalten
			if (m_Procedure)
			{
				delete m_Procedure;
				m_Procedure = NULL;
			}
			if (m_ProcCall)
			{
				delete m_ProcCall;
				m_ProcCall = NULL;
			}
			try
			{
				m_Procedure = new HDevEngineCpp::HDevProcedure(m_DevProgram, ProcedureName.toLatin1().data());
				m_ProcCall = new HDevEngineCpp::HDevProcedureCall(*m_Procedure);
			}
			catch (HDevEngineCpp::HDevEngineException &hdev_exception)
			{	//DispMessage(hdev_exception.Message());
				ErrorMsg = hdev_exception.Message();
				rv = ERROR_CODE_ANY_ERROR;
			}
		}
		else
		{
			QFileInfo fi(ProgramPathAndName);
			ErrorMsg = tr("Procedure %1 Is Not In Measure Programm: %2").arg(ProcedureName).arg(fi.fileName());
			rv = ERROR_CODE_ANY_ERROR;
		}
	}
	catch (HDevEngineCpp::HDevEngineException &hdev_exception)
	{	//DispMessage(hdev_exception.Message());
		ErrorMsg = hdev_exception.Message();
		rv = ERROR_CODE_ANY_ERROR;
	}
	
	return rv;
}


int HalconMeasureTool::ExecuteMeasureTool(HalconCpp::HImage Image,QString &ErrorMsg)
{
	int rv = ERROR_CODE_NO_ERROR;
	if (GetImageData() && GetImageData()->ProceduresLoaded())
	{
		if (m_ProcCall)
		{
			try
			{
				HalconCpp::HTuple WinID = GetImageData()->GetWindowID();//WinID Bildanzeigenummer
				if (WinID != 0)
				{
					QString ParameterName;
					QString ParameterAsString;
					HalconCpp::HTuple InputParameter, OutputParameter;
					ProcedureIOControlParameter *procedureIOControlParameter = GetImageData()->GetIOControlParameter();
					if (procedureIOControlParameter)
					{
						m_ProcCall->SetInputIconicParamObject(1, Image);//first Iconic Object (Halcon def.)
						m_ProcCall->SetInputCtrlParamTuple(1, WinID);//First input control parameter
						//Set input control parameter
						for (int i = 0; i < procedureIOControlParameter->m_ListInputControlParameter.count(); i++)
						{
							ParameterName = procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Name;
							if (procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_STRING)
							{
								QString temp = (procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Value).toString();
								InputParameter = ((procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Value).toString().toLatin1().data());
							}
							else
							{
								if (procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_REAL)
									InputParameter = (procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Value).toDouble();
								else
								{
									if (procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
										InputParameter = (procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Value).toInt();
								}
							}
							m_ProcCall->SetInputCtrlParamTuple(ParameterName.toLatin1().data(), InputParameter);
							/*if (GetImageData()->GetProcedureName() == "CalibrateCamera")//Recording Calibrationdata input
							{
							    QString LocationAndName = GetImageData()->GetCameraCalibrationDataPathRecording() + QString("RecordData%1.txt").arg(GetImageData()->GetImageCounter());
								QString ParameterAsString=(procedureIOControlParameter->m_ListInputControlParameter.at(i).m_Value).toString();
								ParameterAsString = ParameterName + QString(": ") + ParameterAsString;
								if(i==0)
								   WriteParameter(LocationAndName, QString("Input Data"), false);
								WriteParameter(LocationAndName, ParameterAsString, false);
					    	}
							*/
						}
						/*if (GetImageData()->GetProcedureName() == "CalibrateCamera")//Recording Images
						{
							QString LocationAndName = GetImageData()->GetCameraCalibrationDataPathRecording() + QString("RecordImage%1.jpg").arg(GetImageData()->GetImageCounter());
							Image.WriteImage("jpg", 0, LocationAndName.toLatin1().data());
						}
						*/
						m_ProcCall->Execute();//Starten der Messung
						//Set output control parameter (Results)
						for (int i = 0; i < procedureIOControlParameter->m_ListOutputControlParameter.count(); i++)
						{
							ParameterName = procedureIOControlParameter->m_ListOutputControlParameter.at(i).m_Name;
							OutputParameter = m_ProcCall->GetOutputCtrlParamTuple(ParameterName.toLatin1().data());//Ergebnis
							if (OutputParameter.Length() > 0)//ist ein Ergebnis überhaupt vorhanden
							{   //Datentype des Ausgangsparameter abfragen
								if (procedureIOControlParameter->m_ListOutputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_STRING)
								{
									procedureIOControlParameter->m_ListOutputControlParameter[i].m_Value = QString(OutputParameter.S());
								}
								else
								{
									if (procedureIOControlParameter->m_ListOutputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_REAL)
									{
										procedureIOControlParameter->m_ListOutputControlParameter[i].m_Value = OutputParameter.D();
									}
									else
									{
										if (procedureIOControlParameter->m_ListOutputControlParameter.at(i).m_Type == HALCON_DATA_TYPE_NAME_INTEGER)
										{
											procedureIOControlParameter->m_ListOutputControlParameter[i].m_Value = OutputParameter.L();
										}
									}
								}
							}
							/*if (GetImageData()->GetProcedureName() == "CalibrateCamera")//Recording Calibrationdata output
							{
								QString LocationAndName = GetImageData()->GetCameraCalibrationDataPathRecording() + QString("RecordData%1.txt").arg(GetImageData()->GetImageCounter());
								QString ParameterAsString = (procedureIOControlParameter->m_ListOutputControlParameter[i].m_Value).toString();
								ParameterAsString = ParameterName + QString(": ") + ParameterAsString;
								if (i == 0)
									WriteParameter(LocationAndName, QString("\nOutput Data"), false);
								WriteParameter(LocationAndName, ParameterAsString, false);
							}
							*/
						}
						GetImageData()->SetSignalViewOutputControlParameter();//to the GUI
					}
     			}
				else
				{
					ErrorMsg = tr("Image window not visisble(No WinID)");
					rv = ERROR_CODE_ANY_ERROR;
				}
			}
			catch (HDevEngineCpp::HDevEngineException &hdev_exception)
			{	//DispMessage(hdev_exception.Message());
				ErrorMsg = hdev_exception.Message();
				rv = ERROR_CODE_ANY_ERROR;
    		}
		}
		else
		{
			ErrorMsg = tr("No Measureprogram Loaded");
			rv = ERROR_CODE_ANY_ERROR;			
		}
	}
	return rv;
}

int HalconMeasureTool::WriteParameter(QString &FileName, QString &data, bool WithDateAndTime)
{
	int retVal = ERROR_CODE_NO_ERROR;
	QString Data;
	QFile CurrentFile;
	QString Seperator = "|";

	CurrentFile.setFileName(FileName);
	if (WithDateAndTime)
		Data = QDateTime::currentDateTime().date().toString("dd.MM.yy") + Seperator + QDateTime::currentDateTime().time().toString("hh:mm:ss.zzz") + Seperator + data;
	else
		Data = data;
	if (CurrentFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
	{//write data
		QTextStream os(&CurrentFile);
		os << Data << "\n";
		CurrentFile.close();
	}
	else
		retVal = ERROR_CODE_ANY_ERROR;

	return retVal;
}

