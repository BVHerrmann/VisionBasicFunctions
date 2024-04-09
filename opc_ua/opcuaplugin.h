#ifndef OPCUAPLUGIN_H
#define OPCUAPLUGIN_H

#include <plugin.h>
#include <interfaces.h>

#include <QtCore>

class OPCUAInterface;
#include "opcuawidget.h"


class OPCUAPlugin : public Plugin, PluginInterface, MainWindowInterface, CommunicationInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.OPCUAPlugin")
    Q_INTERFACES(PluginInterface)
    Q_INTERFACES(MainWindowInterface)
    Q_INTERFACES(CommunicationInterface)

public:
    enum Type {
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32,
        INT64,
        UINT64,
        FLOAT32,
        RAW8,
        RAW16,
        RAW32,
        RAW64
    };
    
    explicit OPCUAPlugin(QObject *parent = 0);
    virtual ~OPCUAPlugin();

    // PluginInterface
    const QString identifier() const override { return "OPC UA"; }
    const QString name() const override { return tr("OPC UA"); }
    QThread::Priority priority() const override { return QThread::IdlePriority; }
    
    const MachineState machineState() const override; 
    void updateMessages() override;
    
    // MainWindowInterface
    const WidgetType widgetType(const int idx) const override { Q_UNUSED(idx); return Diagnostics; }
    const QString title(const int idx) const override { Q_UNUSED(idx); return name(); }
    QWidget * mainWidget(const int idx) const override { Q_UNUSED(idx); return _widget; }
    int preferredTabIndex(const int idx) const override { Q_UNUSED(idx); return INT_MAX; }
    int requiredWidgetAccessLevel(const int idx) const override { Q_UNUSED(idx); return kAccessLevelSysOp; }
    
    void writeValues(QHash<QString, QVariant> values);

signals:
    // CommunicationInterface
    void valuesChanged(const QHash<QString, QVariant> &values) override;
    void valueChanged(const QString &name, const QVariant &value) override;

    // Helper
    void setupWidget(OPCUAInterface *opcua_interface);

public slots:
    // PluginInterface
    void uninitialize() override;
    void requestMachineState(const PluginInterface::MachineState state) { _request_state = state; }

    // CommunicationInterface
    void setValues(const QHash<QString, QVariant> &values) override;
    void setValue(const QString &name, const QVariant &value) override;

    void printStatistics();

    void connectOPCUA();
    void disconnectOPCUA();

private:
    PluginInterface::MachineState _request_state = PluginInterface::Off;
    OPCUAWidget *_widget;

    std::shared_ptr<OPCUAInterface> _interface;
    
    QTimer *_timer;
};

#endif // OPCUAPLUGIN_H
