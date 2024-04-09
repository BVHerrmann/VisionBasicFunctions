#ifndef AUDITTRAILPLUGIN_H
#define AUDITTRAILPLUGIN_H

#include <plugin.h>
#include <interfaces.h>

#include <QtCore>
#include <QtWidgets>

#include "audittraildatabase.h"
#include "audittrailwidget.h"
#include "audittrailoptionpanel.h"


class AuditTrailPlugin : public Plugin, MainWindowInterface, PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.AuditTrailPlugin")
    Q_INTERFACES(MainWindowInterface)
    Q_INTERFACES(PluginInterface)

public:
    explicit AuditTrailPlugin(QObject *parent = 0);
    virtual ~AuditTrailPlugin();

    // PluginWindowInterface
    const QString identifier() const override { return "AuditTrail"; }
    const QString name() const override { return tr("Audit Trail"); }
    const MachineState machineState() const override { return _current_state; }
    QThread::Priority priority() const override { return QThread::InheritPriority; }

    // MainWindowInterface
    const WidgetType widgetType(const int idx) const override { (void)idx; return Messages; }
    const QString title(const int idx) const override { (void)idx; return name(); }
    QWidget * mainWidget(const int idx) const override { (void)idx; return (QWidget *)_widget; }
	virtual OptionPanel *optionPanel(const int idx) const override { (void)idx; return currentAccessLevel() >= kAccessLevelAdmin ? _optionPanel : nullptr; }
    int preferredTabIndex(const int idx) const override { (void)idx; return INT_MAX - 15; }
    int requiredWidgetAccessLevel(const int idx) const override { (void)idx; return kAccessLevelService; }
    int mainWidgetCount() const override { return 1; }

signals:

public slots:
    // PluginInterface
    void initialize() override;
    void uninitialize() override;
    void requestMachineState(const PluginInterface::MachineState state) override { _current_state = state; }

    // PreferencesInterface
    void loadPreferences();
    
    void log(const QString &msg);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    
private:
    PluginInterface::MachineState _current_state = Production;
    
    AuditTrailWidget *_widget;
	AuditTrailOptionPanel *_optionPanel;
    
    AuditTrailDatabase *_db;
    
    void logChange(QWidget *widget);
    QVariant getCurrentValue(QWidget *widget);
    
    void registerWidget(QWidget *widget);
    void unregisterWidget(QWidget *widget);
    QWidgetList _registeredWidgets;
    
    QVariant storeValue(QWidget *widget, bool force = false);
    QMap<QWidget *, QVariant> _storedValues;
};

#endif // AUDITTRAILPLUGIN_H
