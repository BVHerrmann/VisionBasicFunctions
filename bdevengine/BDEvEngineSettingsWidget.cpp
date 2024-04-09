#include "BDEvEngineSettingsWidget.h"
#include <QtGui>
#include <QtWidgets>
#include "GlobalConst.h"
#include "MainAppBDevEngine.h"
#include "ImageData.h"
#include "hdevengine/HDevEngineCpp.h"

const char * const kPropertyKey = "propertyKey";

BDevEngineSettingsWidget::BDevEngineSettingsWidget(MainAppBDevEngine *parent) : QWidget(NULL)
, m_MainAppBDevEngine(NULL)
, m_CameraIDModelComboBox(NULL)
{
	m_MainAppBDevEngine      = parent;
	m_CameraIDModelComboBox  = new QStandardItemModel(this);//Liste aller verfügbaren Kamera/KameraIDs
	QString SettingsLocation=GetMainAppBDevEngine()->GetSettingsLocation();//Speicherort der Konfiguration der verwendeten Messprogramm für jede Kamera
	QSettings settings(SettingsLocation, QSettings::IniFormat);

	registerCamera(QString(DISABLED_NAME_CAMERA));//erster eintrag mit der Möglichkeit die Kamera zu deaktivieren
	registerCamera(QString(SIMULATION_NAME_CAMERA));
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		m_ProcedureModelComboBox[i] = new QStandardItemModel(this);
		QString Path  = SECTION_CAMERA_ID + QString("%1").arg(i);
		QString Value = settings.value(Path, QString(DISABLED_NAME_CAMERA)).toString();
		registerCamera(Value);//Liste füllen der zuletzt gesetzen KameraIds für die ComboBoxen
	}
	//if (IsDebuggerPresent())
	//	registerCamera("Dummy");
	setupUi();
	
}


BDevEngineSettingsWidget::~BDevEngineSettingsWidget()
{

}


void BDevEngineSettingsWidget::DisConnectAllSignalSlots()
{
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		disconnect(m_LineEditCameraName[i], &QLineEdit::editingFinished, this, &BDevEngineSettingsWidget::SlotCameraNameChanged);
		disconnect(m_ComboBoxCameraID[i], static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BDevEngineSettingsWidget::SlotChangeCamera);
		disconnect(m_PushButtonSelectProgram[i], &QPushButton::clicked, this, &BDevEngineSettingsWidget::SlotButtonClickedSelectProgram);
		disconnect(m_ComboBoxProcedureName[i], static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BDevEngineSettingsWidget::SlotProcedureChanged);
	}
}


void BDevEngineSettingsWidget::setupUi()
{
	QString      SectionName,ProgramNameAndPath;
	QString      SettingsLocation = GetMainAppBDevEngine()->GetSettingsLocation();
	QSettings    settings(SettingsLocation, QSettings::IniFormat);
	QGridLayout *pGridLayout = new QGridLayout();
	QHBoxLayout *pProrgamNameLayout[MAX_CAMERAS];
	QFormLayout *pFormLayoutCamera[MAX_CAMERAS];
	QGroupBox   *pCameraGroup[MAX_CAMERAS];
	
	
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		pFormLayoutCamera[i]         = new QFormLayout();
		pCameraGroup[i]              = new QGroupBox(tr("Camera %1").arg(i + 1));
		m_LineEditCameraName[i]      = new QLineEdit();
		m_ComboBoxCameraID[i]        = new QComboBox();
		m_LineEditProgramName[i]     = new QLineEdit();
		m_ComboBoxProcedureName[i]   = new QComboBox();
		pProrgamNameLayout[i]        = new QHBoxLayout();
		m_PushButtonSelectProgram[i] = new QPushButton("w");
		m_PushButtonSelectProgram[i]->setStyleSheet("background-color: rgb(64, 77, 83);border-radius: 4px;font-family: Bertram; min-height: 38px; max-height: 38px; min-width: 25px; max-width: 25px;qproperty-flat: true;");
		pCameraGroup[i]->setLayout(pFormLayoutCamera[i]);
	}
	for (int i = 0; i < MAX_CAMERAS; i++)
	{   //Camera Name
		SectionName = SECTION_CAMERA_NAME + QString("%1").arg(i);
		m_LineEditCameraName[i]->setProperty(kPropertyKey, SectionName);
		m_LineEditCameraName[i]->setText(settings.value(SectionName, QVariant(tr("Camera%1").arg(i + 1))).toString());
		connect(m_LineEditCameraName[i], &QLineEdit::editingFinished, this, &BDevEngineSettingsWidget::SlotCameraNameChanged);
		pFormLayoutCamera[i]->addRow(tr("Name:"), m_LineEditCameraName[i]);
		//Camera ID
		SectionName = SECTION_CAMERA_ID + QString("%1").arg(i);
		m_ComboBoxCameraID[i]->setProperty(kPropertyKey, SectionName);
		m_ComboBoxCameraID[i]->setModel(m_CameraIDModelComboBox);
		m_ComboBoxCameraID[i]->setCurrentIndex(indexForCameraId(settings.value(SectionName).toString()));
		connect(m_ComboBoxCameraID[i], static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BDevEngineSettingsWidget::SlotChangeCamera);
		pFormLayoutCamera[i]->addRow(tr("ID:"), m_ComboBoxCameraID[i]);
		//Measureprorgam
		SectionName = SECTION_MEASURE_PROGRAM + QString("%1").arg(i);
		m_PushButtonSelectProgram[i]->setProperty(kPropertyKey, SectionName);
		ProgramNameAndPath=GetMainAppBDevEngine()->GetMeasureProgramPath() + QString("\\") + "HalconTest.hdev";//Default Progname
		ProgramNameAndPath=settings.value(SectionName, QVariant(ProgramNameAndPath)).toString();
		QFileInfo fi(ProgramNameAndPath);
		m_LineEditProgramName[i]->setText(fi.fileName());
		pProrgamNameLayout[i]->addWidget(m_LineEditProgramName[i]);
		pProrgamNameLayout[i]->addWidget(m_PushButtonSelectProgram[i]);
		connect(m_PushButtonSelectProgram[i], &QPushButton::clicked, this, &BDevEngineSettingsWidget::SlotButtonClickedSelectProgram);
		pFormLayoutCamera[i]->addRow(tr("Program:"), pProrgamNameLayout[i]);
		//ProcedureName
		SectionName = SECTION_MEASURE_PROCEDURE + QString("%1").arg(i);
		m_ComboBoxProcedureName[i]->setProperty(kPropertyKey, SectionName);
		FillStandardItemModelProcedure(ProgramNameAndPath,i);
		m_ComboBoxProcedureName[i]->setModel(m_ProcedureModelComboBox[i]);
		m_ComboBoxProcedureName[i]->setCurrentIndex(indexForProcedureName(settings.value(SectionName).toString(),i));
		connect(m_ComboBoxProcedureName[i], static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BDevEngineSettingsWidget::SlotProcedureChanged);
		pFormLayoutCamera[i]->addRow(tr("Procedure:"), m_ComboBoxProcedureName[i]);
		if(i==0)
			pGridLayout->addWidget(pCameraGroup[i], 1, 0);
		if (i == 1)
			pGridLayout->addWidget(pCameraGroup[i], 1, 1);
		if (i == 2)
			pGridLayout->addWidget(pCameraGroup[i], 2, 0);
		if (i == 3)
			pGridLayout->addWidget(pCameraGroup[i], 2, 1);
			
	}
	setLayout(pGridLayout);
	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		if (GetMainAppBDevEngine()->GetImageDataByIndex(i))
		{
			GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetName(m_LineEditCameraName[i]->text());
			GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetMeasureProgramName(m_LineEditProgramName[i]->text());
			GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetProcedureName(m_ComboBoxProcedureName[i]->currentText());
			GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetupControlParameterWidget();
			GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetCameraID(m_ComboBoxCameraID[i]->currentText());
		}
	}
	//UpdateImageData();
}


void BDevEngineSettingsWidget::SlotProcedureChangedByPLC(const QString &CameraName, const QString &ProcedureName)
{
	bool IsSet = false;
	for (int i = 0; i < MAX_CAMERAS; i++)
	{   //Camera Name
		if (m_LineEditCameraName[i]->text() == CameraName)
		{
			for (int r = 0; r < m_ProcedureModelComboBox[i]->rowCount(); r++)
			{
				if (m_ProcedureModelComboBox[i]->item(r)->text() == ProcedureName)
				{
					m_ComboBoxProcedureName[i]->setCurrentIndex(r);
					IsSet = true;
					break;
				}
			}
			break;
		}
	}
}


bool BDevEngineSettingsWidget::CheckIsCameraDisabled(int index)
{
	bool rv = false;
	if (index >= 0 || MAX_CAMERAS < index)
	{
		if (m_ComboBoxCameraID[index]->currentText() == DISABLED_NAME_CAMERA)
			rv = true;
	}
	return rv;
}


void BDevEngineSettingsWidget::FillStandardItemModelProcedure(QString &ProgramNameAndPath,int index)
{
	HDevEngineCpp::HDevProgram DevProgram;
	HalconCpp::HTuple ProcedureNames;
	
	m_ProcedureModelComboBox[index]->clear();
	try
	{
		DevProgram.LoadProgram(ProgramNameAndPath.toStdString().c_str());
		ProcedureNames = DevProgram.GetLocalProcedureNames();
		for (int i = 0; i < ProcedureNames.Length(); i++)
		{
			QString ProcedureName = QString(QByteArray::fromStdString((std::string)(ProcedureNames[i].S())));
			QList<QStandardItem *> existingItems = m_ProcedureModelComboBox[index]->findItems(ProcedureName, Qt::MatchExactly, 0);
			if (existingItems.isEmpty())
				m_ProcedureModelComboBox[index]->setItem(m_ProcedureModelComboBox[index]->rowCount(), new QStandardItem(ProcedureName));
		}
	}
	catch (HDevEngineCpp::HDevEngineException &hdev_exception)
	{   //DispMessage(hdev_exception.Message());
		if (GetMainAppBDevEngine())
		{
			QString ErrorMsg = hdev_exception.Message();
			GetMainAppBDevEngine()->SlotAddNewMessage(ErrorMsg, QtMsgType::QtFatalMsg);
		}
	}
}


void BDevEngineSettingsWidget::registerCamera(QString cameraId, bool FromCameraInterface)
{
	QList<QStandardItem *> existingItems = m_CameraIDModelComboBox->findItems(cameraId, Qt::MatchExactly, 0);
	if (existingItems.isEmpty())
		m_CameraIDModelComboBox->setItem(m_CameraIDModelComboBox->rowCount(), new QStandardItem(cameraId));
	if (FromCameraInterface)
		m_ListUsedCameraIDs.append(cameraId);
}


void BDevEngineSettingsWidget::showEvent(QShowEvent *event)
{
	bool NewCameraTypeIsIn = false;

	for (int i = 0; i < m_CameraIDModelComboBox->rowCount(); i++)
	{
		if (m_CameraIDModelComboBox->item(i)->text() != QString(DISABLED_NAME_CAMERA) && m_CameraIDModelComboBox->item(i)->text() != QString(SIMULATION_NAME_CAMERA))
		{
			if (!m_ListUsedCameraIDs.contains(m_CameraIDModelComboBox->item(i)->text()))
			{
				m_CameraIDModelComboBox->removeRow(i);
				i = 0;
				NewCameraTypeIsIn = true;
			}
		}
	}
	if (NewCameraTypeIsIn)
	{
		for (int i = 0; i < MAX_CAMERAS; i++)
		{
			m_ComboBoxCameraID[i]->setModel(m_CameraIDModelComboBox);
			m_ComboBoxCameraID[i]->setCurrentIndex(0);
		}
	}
}


void BDevEngineSettingsWidget::UpdateImageData()
{
	//QString ErrorMsg;
	//QString ConfigureOrSetup = "setup";
	//bool UpdateUPCUAServer = false;

	for (int i = 0; i < MAX_CAMERAS; i++)
	{
			if (GetMainAppBDevEngine()->GetImageDataByIndex(i))
			{
				bool MeasureProgramChanged = false;
				GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetCameraID(m_ComboBoxCameraID[i]->currentText());
				if (m_LineEditCameraName[i]->text() != GetMainAppBDevEngine()->GetImageDataByIndex(i)->GetName())
				{
					GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetName(m_LineEditCameraName[i]->text());
					MeasureProgramChanged = true;
				}
				if (m_LineEditProgramName[i]->text() != GetMainAppBDevEngine()->GetImageDataByIndex(i)->GetMeasureProgramName())
				{
					GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetMeasureProgramName(m_LineEditProgramName[i]->text());
					GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetReloadMeasureProgram(true);//beim nächsten Messaufruf wird das Programm neu geladen
					MeasureProgramChanged = true;
				}
				if (m_ComboBoxProcedureName[i]->currentText() != GetMainAppBDevEngine()->GetImageDataByIndex(i)->GetProcedureName())
				{
					GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetProcedureName(m_ComboBoxProcedureName[i]->currentText());
					GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetReloadMeasureProgram(true);//beim nächsten Messaufruf wird das Programm neu geladen
					MeasureProgramChanged = true;
				}
				if (MeasureProgramChanged)
				{
					GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetupControlParameterWidget();//Paramter haben sich geändert Anzeige updaten
					//GetMainAppBDevEngine()->GetImageDataByIndex(i)->LoadAndSetAllProcedures(ErrorMsg);//Alle möglichen Proceduren neu laden
					//UpdateUPCUAServer = true;
				}
			}
	}
	//if (MeasureProgramChanged)
	//{
	//	for (int i = 0; i < MAX_CAMERAS; i++)
	//		GetMainAppBDevEngine()->GetImageDataByIndex(i)->SetupControlParameterWidget();
	//}
	//if(UpdateUPCUAServer)
	 // GetMainAppBDevEngine()->ConfigureOPCUA(ConfigureOrSetup);//der OPCUA Server muss neu konfiguriert werden
	
}


void BDevEngineSettingsWidget::ConfigureOPCUANew(QString &ConfigureOrSetup)
{
	if(GetMainAppBDevEngine())
	   GetMainAppBDevEngine()->ConfigureOPCUA(ConfigureOrSetup);
}


bool BDevEngineSettingsWidget::CheckIsCameraIDInUse(QString &CameraID)
{
	bool rv = false;
	QString CurrentCameraID;

	for (int i = 0; i < MAX_CAMERAS; i++)
	{
		if (GetMainAppBDevEngine() && GetMainAppBDevEngine()->GetImageDataByIndex(i))
		{
			CurrentCameraID=GetMainAppBDevEngine()->GetImageDataByIndex(i)->GetCameraID();
			if (CameraID == CurrentCameraID)
			{
				rv = true;
				break;
			}
		}
	}
	return rv;
}


void BDevEngineSettingsWidget::SlotButtonClickedSelectProgram()
{
	QVariant propertyKey = QObject::sender()->property(kPropertyKey);
	if (propertyKey.isValid())
	{
		QString SettingsLocation = GetMainAppBDevEngine()->GetSettingsLocation();
		QSettings settings(SettingsLocation, QSettings::IniFormat);
		QString  ProgramNameAndPath = QFileDialog::getOpenFileName(this, tr("Select Program"), GetMainAppBDevEngine()->GetMeasureProgramPath(), tr("*.hdev"));
		if (!ProgramNameAndPath.isEmpty())
		{
			QFileInfo fi(ProgramNameAndPath);
			QString ProgramName = fi.fileName();
			QString Section=propertyKey.toString();
			QString StringIndex = Section.right(1);
			settings.setValue(Section, ProgramNameAndPath);
			m_LineEditProgramName[StringIndex.toInt()]->setText(ProgramName);
			FillStandardItemModelProcedure(ProgramNameAndPath, StringIndex.toInt());
			UpdateImageData();
		}
	}
}

int BDevEngineSettingsWidget::indexForProcedureName(QString ProcedureName,int index)
{
	QList<QStandardItem *> existingItems = m_ProcedureModelComboBox[index]->findItems(ProcedureName, Qt::MatchExactly, 0);

	if (existingItems.isEmpty())
		return -1;
	else
		return existingItems.first()->row();
}


int BDevEngineSettingsWidget::indexForCameraId(QString cameraId)
{
	QList<QStandardItem *> existingItems = m_CameraIDModelComboBox->findItems(cameraId, Qt::MatchExactly, 0);
	
	if (existingItems.isEmpty())
		return -1;
	else
    	return existingItems.first()->row();
}


void BDevEngineSettingsWidget::SlotCameraNameChanged()
{
	QVariant propertyKey = QObject::sender()->property(kPropertyKey);
	if (propertyKey.isValid() && GetMainAppBDevEngine())
	{
		QString   SettingsLocation = GetMainAppBDevEngine()->GetSettingsLocation();
		QSettings settings(SettingsLocation, QSettings::IniFormat);
		QString   value=((QLineEdit*)QObject::sender())->text();
		QString   SectionName = propertyKey.toString();
		
		settings.setValue(SectionName, value);
		UpdateImageData();//Info to ImageData Class
	}
}


void BDevEngineSettingsWidget::SlotChangeCamera(int index)
{
	if (index < 0 || m_CameraIDModelComboBox->rowCount() <= index)
		return;
	else
	{
		QString CameraID = m_CameraIDModelComboBox->item(index, 0)->text();
		if (!CheckIsCameraIDInUse(CameraID) || CameraID == SIMULATION_NAME_CAMERA || CameraID == DISABLED_NAME_CAMERA)
		{
			changeValue(QObject::sender(), CameraID);//Save into Settings location
			UpdateImageData();//Info to ImageData Class
		}
		else
		{
			QVariant propertyKey = QObject::sender()->property(kPropertyKey);
			if (propertyKey.isValid())
			{
				QString Section = propertyKey.toString();
				QString StringIndex = Section.right(1);
				m_ComboBoxCameraID[StringIndex.toInt()]->setCurrentIndex(0);
				if (CameraID != DISABLED_NAME_CAMERA && CameraID != SIMULATION_NAME_CAMERA)
				{
					if(GetMainAppBDevEngine())
					   GetMainAppBDevEngine()->SlotAddNewMessage(tr("Camera %1 Is In Use").arg(CameraID), QtMsgType::QtWarningMsg);
				}
				else
				  UpdateImageData();
			}
		}
    }
}


void BDevEngineSettingsWidget::SlotProcedureChanged(int index)
{
	QStandardItemModel *pItemModel = (QStandardItemModel*)((QComboBox*)(QObject::sender()))->model();
	if (index < 0 || pItemModel->rowCount() <= index)
		return;
	else
	{
		QString ProcedureName = pItemModel->item(index, 0)->text();
		changeValue(QObject::sender(), ProcedureName);
		UpdateImageData();//Procedurename hat sich geändert, an Instanz weitergeben
	}
}


void BDevEngineSettingsWidget::changeValue(QObject *sender, QVariant value)
{
	if (!sender)
	{
		qWarning() << "No sender for changeValue(" << value << ")!";
		return;
	}

	QVariant propertyKey = sender->property(kPropertyKey);
	if (propertyKey.isValid() && GetMainAppBDevEngine())
	{
		QString SettingsLocation = GetMainAppBDevEngine()->GetSettingsLocation();
		QSettings settings(SettingsLocation, QSettings::IniFormat);
		
		if (value.isValid() && !value.isNull())
		{
			settings.setValue(propertyKey.toString(), value);
		}
		else
		{
			settings.remove(propertyKey.toString());
		}
	}
	else
	{
		qWarning() << "No property \"" << kPropertyKey << "\" for sender" << sender << "value" << value;
		return;
	}
}


	