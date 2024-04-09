#pragma once
#include <qwidget.h>
#include "GlobalConst.h"
class QStandardItemModel;
class MainAppBDevEngine;
class QLineEdit;
class QPushButton;
class QComboBox;
class BDevEngineSettingsWidget : public QWidget
{
	Q_OBJECT
public:
	BDevEngineSettingsWidget(MainAppBDevEngine *parent);
	~BDevEngineSettingsWidget();
	MainAppBDevEngine *GetMainAppBDevEngine() {return m_MainAppBDevEngine;}
	void showEvent(QShowEvent *event);
	void registerCamera(QString cameraId,bool FromCamera=false);
	int indexForCameraId(QString CameraId);
	int indexForProcedureName(QString ProcedureName, int index);
	void setupUi();
	void changeValue(QObject *sender, QVariant value);
	void UpdateImageData();
	void FillStandardItemModelProcedure(QString &ProgramNameAndPath, int index);
	void DisConnectAllSignalSlots();
	bool CheckIsCameraIDInUse(QString &CameraID);
	bool CheckIsCameraDisabled(int index);
	void ConfigureOPCUANew(QString &ConfigureOrSetup);

public slots:
	void SlotChangeCamera(int index);
	void SlotButtonClickedSelectProgram();
	void SlotProcedureChanged(int index);
	void SlotCameraNameChanged();
	void SlotProcedureChangedByPLC(const QString &CameraName, const QString &ProcdeureName);

private:
	MainAppBDevEngine *m_MainAppBDevEngine;
	QStandardItemModel *m_CameraIDModelComboBox;
	QStringList m_ListUsedCameraIDs;
	QStringList m_ListRegisteredCameraIDs;
	QLineEdit   *m_LineEditProgramName[MAX_CAMERAS];
	QPushButton *m_PushButtonSelectProgram[MAX_CAMERAS];
	QComboBox   *m_ComboBoxProcedureName[MAX_CAMERAS];
	QLineEdit   *m_LineEditCameraName[MAX_CAMERAS];
	QComboBox   *m_ComboBoxCameraID[MAX_CAMERAS];
	QStandardItemModel *m_ProcedureModelComboBox[MAX_CAMERAS];
	
};

