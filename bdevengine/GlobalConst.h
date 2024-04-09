#pragma once
const int TAB_INDEX_MAIN_WIDGET_SETTINGS = 0;
const int TAB_INDEX_IMAGES_OVERVIEW = 1;
const int TAB_INDEX_IMAGE_OVERVIEW_0 = 2;
const int TAB_INDEX_IMAGE_OVERVIEW_1 = 3;
const int TAB_INDEX_IMAGE_OVERVIEW_2 = 4;
const int TAB_INDEX_IMAGE_OVERVIEW_3 = 5;

const int MAX_CAMERAS = 4;
const int CAMERA_1_ID = 1;
const int CAMERA_2_ID = 2;
const int CAMERA_3_ID = 3;
const int CAMERA_4_ID = 4;


const int ERROR_CODE_NO_ERROR = 0;
const int ERROR_CODE_ANY_ERROR = 1;

const QString SECTION_CAMERA_ID                     = "BDevEngine/Camera";
const QString SECTION_CAMERA_NAME                   = "BDevEngine/Name";
const QString SECTION_MEASURE_PROGRAM               = "BDevEngine/MeasureProgram";
const QString SECTION_MEASURE_PROCEDURE             = "BDevEngine/MeasureProcedure";

const QString SECTION_NAME_INPUT_PARAMETER          = "InputParameter/";
const QString SECTION_NAME_OUTPUT_PARAMETER         = "OutputParameter/";

const QString DISABLED_NAME_CAMERA                  = "Disabled";
const QString SIMULATION_NAME_CAMERA                = "Simulation";

const QString HALCON_DATA_TYPE_NAME_STRING          = "string";
const QString HALCON_DATA_TYPE_NAME_REAL            = "real";
const QString HALCON_DATA_TYPE_NAME_INTEGER         = "integer";

//Input data names
const QString OPCUA_INPUT_NAME_RECORD_IMAGES        = "RecordImages";

//Output data names
const QString OPCUA_OUTPUT_NAME_JOP_PASS            = "JobPass";
const QString OPCUA_OUTPUT_NAME_MODEL_FREE          = "ModelFree";
const QString OPCUA_OUTPUT_NAME_RESULTS_VALID       = "ResultsValid";
const QString OPCUA_OUTPUT_NAME_INSPECTION_COMPLETE = "InspectionComplete";
const QString OPCUA_OUTPUT_NAME_SYSTEM_BUSY         = "SystemBusy";
const QString OPCUA_OUTPUT_NAME_TRIGGER_READY       = "TriggerReady";
const QString OPCUA_OUTPUT_NAME_INSPECTION_ID       = "InspectionID";
const QString OPCUA_OUTPUT_NAME_CURRENT_JOB_NAME    = "CurrentJobName";

const QString OPCUA_METHOD_NAME_TRIGGER_CAMERA      = "TriggerAcquisition";
const QString OPCUA_METHOD_NAME_LOAD_JOB            = "LoadJob";
const QString OPCUA_METHOD_NAME_LOAD_JOB_INPUT      = "JobName";

#define IMAGE_FITERS <<"*.bmp"<<"*.tif"<<"*.png"
#define IMAGE_LOCATION_ARUMENTS(CameraIndexName,ProcedureName,SubPath)                              QString("%1_%2_ImageLocation/Path_%3").arg(CameraIndexName).arg(ProcedureName).arg(SubPath)
#define IMAGE_PROCEDURE_PARAMETER_ARUMENTS_VALUE(CameraIndexName,ProcedureName,SectionName,index)   QString("%1_%2/%3Index%4/Value").arg(CameraIndexName).arg(ProcedureName).arg(SectionName).arg(index)
#define IMAGE_PROCEDURE_PARAMETER_ARUMENTS_NAME(CameraIndexName,ProcedureName,SectionName,index)    QString("%1_%2/%3Index%4/Name").arg(CameraIndexName).arg(ProcedureName).arg(SectionName).arg(index)
#define IMAGE_PROCEDURE_PARAMETER_ARUMENTS_TYPE(CameraIndexName,ProcedureName,SectionName,index)    QString("%1_%2/%3Index%4/Type").arg(CameraIndexName).arg(ProcedureName).arg(SectionName).arg(index)

const QString IMGAGE_SAVE_CONDITION_ALL_IMAGES      = "all";
const QString IMGAGE_SAVE_CONDITION_ONLY_BAD_IMAGES = "nio";

const int MAX_NUMBER_IO_PARAMETER = 32;




