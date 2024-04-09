#ifndef BDEVENGINEPLUGIN_H
#define BDEVENGINEPLUGIN_H


#include <plugin.h>
#include <interfaces.h>
#include <QtCore>
#include <QtWidgets>
#include "GlobalConst.h"


class MainAppBDevEngine;
class BDevEngineSettingsWidget;
class BDevEngineImagesOverview;
class BDevEngineImageOverview;
class BDevEnginePlugin : public Plugin, MainWindowInterface, PluginInterface, CommunicationInterface, ImageConsumerInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.BDevEnginePlugin")
    Q_INTERFACES(MainWindowInterface)
    Q_INTERFACES(PluginInterface)
	Q_INTERFACES(CommunicationInterface)
	Q_INTERFACES(ImageConsumerInterface)

public:
    explicit BDevEnginePlugin(QObject *parent = 0);
    virtual ~BDevEnginePlugin();
	 
	MainAppBDevEngine *GetMainAppBDevEngine() {return m_MainAppBDevEngine;}
	int GetCurrentNumberCameras() { return m_CurrentNumberCameras; }
    // PluginWindowInterface
    const QString identifier() const override { return "BDevEngine"; }
    const QString name() const override { return tr("BDevEngine"); }
    const MachineState machineState() const override { return _current_state; }
    QThread::Priority priority() const override { return QThread::IdlePriority; }
	void SetMessage(const QString &message, QtMsgType MsgType);

    // MainWindowInterface
	const WidgetType widgetType(const int idx) const
	{
		if (idx == TAB_INDEX_MAIN_WIDGET_SETTINGS)
			return Settings;
		else
			return Application;
	}
	
	//const WidgetType widgetType(const int idx) const override { (void)idx; return Application; }
	const QString title(const int idx) const;// override { (void)idx; return name(); }
	QWidget * mainWidget(const int idx) const;// override { (void)idx; return (QWidget *)_widget; }
	virtual OptionPanel *optionPanel(const int idx) const override { (void)idx; return nullptr; }
    int preferredTabIndex(const int idx) const override { (void)idx; return INT_MAX - 10; }
    int requiredWidgetAccessLevel(const int idx) const override { (void)idx; return kAccessLevelSysOp; }
    int mainWidgetCount() const override { return m_NumberMainWidget; }
	void SetPreference(const QString & preference, const QVariant & value);
	QVariant GetPreference(const QString & preference);
	void SoftwareTrigger(QString &CameraID);
	BDevEngineSettingsWidget *GetBDevEngineSettingsWidget() {return m_BDevEngineSettingsWidget;}
	void CameraNameChanged(QString &NewName, int Index);
	void ConfigureOPCUA(QString &CofigureOrSetup);
    
signals:
	// CommunicationInterface
	void valuesChanged(const QHash<QString, QVariant> &values);
	void valueChanged(const QString &name, const QVariant &value);
	void SignalStartupInitReady();

public slots:
    // PluginInterface
    void initialize() override;
    void uninitialize() override;
    void requestMachineState(const PluginInterface::MachineState state) override { _current_state = state; }

	// CommunicationInterface
	void setValue(const QString &name, const QVariant &value);
	void setValues(const QHash<QString, QVariant> &values);

	// ImageConsumerInterface
	void consumeImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image);

	void reset();
	

private:
    PluginInterface::MachineState _current_state = Production;
	MainAppBDevEngine        *m_MainAppBDevEngine;
	BDevEngineSettingsWidget *m_BDevEngineSettingsWidget;//Einstellwidget mit welcher Kamera was gemessen wird
	BDevEngineImagesOverview *m_BDevEngineImagesOverview;//Alle Kamerabilder auf eine Ansicht
	BDevEngineImageOverview  *m_BDevEngineImageOverview[MAX_CAMERAS];//Pro Ansicht ein Kamerabild
	int                       m_CurrentNumberCameras;
	int                       m_NumberMainWidget;
	QString  m_CameraNames[MAX_CAMERAS];
	QString  m_cameraId;
	QList<PluginInterface::Message> m_Messages;
	int                             m_MessageCounter;
	bool m_StartUnInit;
};

#endif // BDEVENGINEPLUGIN_H
