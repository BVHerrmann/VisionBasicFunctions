#ifndef LOGGERPLUGIN_H
#define LOGGERPLUGIN_H

#include <plugin.h>
#include <interfaces.h>

#include <QtCore>
#include <QtWidgets>

#include "loggerwidget.h"
#include "loggeroptionpanel.h"
#include "loggermodel.h"

class LoggerPlugin : public Plugin, MainWindowInterface, PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.LoggerPlugin")
    Q_INTERFACES(MainWindowInterface)
    Q_INTERFACES(PluginInterface)

public:
    explicit LoggerPlugin(QObject *parent = 0);
    virtual ~LoggerPlugin();

    // PluginWindowInterface
    const QString identifier() const override { return "Console"; }
    const QString name() const override { return tr("Console"); }
    const MachineState machineState() const override { return _current_state; }
    QThread::Priority priority() const override { return QThread::IdlePriority; }

    // MainWindowInterface
    const WidgetType widgetType(const int idx) const override { (void)idx; return Messages; }
    const QString title(const int idx) const override { (void)idx; return name(); }
    QWidget * mainWidget(const int idx) const override { (void)idx; return (QWidget *)_widget; }
	virtual OptionPanel *optionPanel(const int idx) const override { (void)idx; return currentAccessLevel() >= kAccessLevelService ? _optionPanel : nullptr; }
    int preferredTabIndex(const int idx) const override { (void)idx; return INT_MAX - 10; }
    int requiredWidgetAccessLevel(const int idx) const override { (void)idx; return kAccessLevelSysOp; }
    int mainWidgetCount() const override { return 1; }
    
signals:

public slots:
    // PluginInterface
    void initialize() override;
    void uninitialize() override;
    void requestMachineState(const PluginInterface::MachineState state) override { _current_state = state; }

    // PreferencesInterface
    void loadPreferences();

    void timerEvent(QTimerEvent *event) override;
    void log(QtMsgType type, const QMessageLogContext &context, const QString &msg, const QDateTime &timestamp, const QString &source);

private:
    QDir _logDir;
    QMutex _fileMutex;
    QFile *_logFile;
    int _timer;

    PluginInterface::MachineState _current_state = Production;

    LoggerWidget *_widget;
	LoggerOptionPanel *_optionPanel;
    LoggerModel *_model;
    
    QDir logDirectory() const;
    void updateLogFile();
    void cleanLogFiles();
};

#endif // LOGGERPLUGIN_H
