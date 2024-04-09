#pragma once

#include <QObject>
#include "halconcpp/HalconCpp.h"
#include "hdevengine/HDevEngineCpp.h"

class ImageData;
class HalconMeasureTool : public QObject
{
	Q_OBJECT
public:
	HalconMeasureTool(ImageData *pImageData,QObject *parent=NULL);
	~HalconMeasureTool();
	int ExecuteMeasureTool(HalconCpp::HImage Image,QString &ErrorMsg);
	QString GetMeasureToolName() { return m_MeasureToolName; }
	ImageData *GetImageData()    { return m_ImageData;}
	int LoadMeasureProgram(QString &ErrorMsg);
	int WriteParameter(QString &FileName, QString &data, bool WithDateAndTime);
	
private:
	QString m_MeasureToolName;
	HDevEngineCpp::HDevProcedure     *m_Procedure; // Fassen Sie eine Prozedur, die Nummer, den Namen und andere Informationen ihrer Eingabe- und Ausgabeparameter zusammen
	HDevEngineCpp::HDevProcedureCall *m_ProcCall; // Verwalten einer Prozedurinstanz, z. B. Ausführen und anderer Vorgänge
	ImageData *m_ImageData;
	HDevEngineCpp::HDevProgram m_DevProgram;
};
