#include "opcuaplugin.h"

#include <logic.h>

#include "opcuaserver.h"
#include "opcuaclient.h"
#include "opcuawidget.h"


OPCUAPlugin::OPCUAPlugin(QObject *parent) :
    Plugin(parent)
{
    _widget = new OPCUAWidget(this);
    connect(this, &OPCUAPlugin::setupWidget, _widget, &OPCUAWidget::setupWidget);
    
    // start a timer on the main thread!
    _timer = new QTimer();
    connect(_timer, &QTimer::timeout, this, &OPCUAPlugin::printStatistics, Qt::DirectConnection);
    _timer->start(60000);
}

OPCUAPlugin::~OPCUAPlugin()
{
    _timer->stop();
    delete _timer;
}

void OPCUAPlugin::uninitialize()
{
    disconnectOPCUA();
}

void OPCUAPlugin::printStatistics()
{
    if (_interface) {
        _interface->printStatistics();
    }
}

void OPCUAPlugin::connectOPCUA()
{
    if (_interface)
        _interface->connectOPCUA();
}

const PluginInterface::MachineState OPCUAPlugin::machineState() const
{
    if (_request_state == PluginInterface::Terminate) {
        return _request_state;
    } else if (!_interface) {
        return PluginInterface::Initializing;
    }

    return _interface->machineState();
}

void OPCUAPlugin::updateMessages()
{
    QList<PluginInterface::Message> messages;
    
    if (!_interface) {
        messages << PluginInterface::Message(100, tr("OPC-UA not configured"), QtWarningMsg);
    } else {
        long state = _interface->state();
        if (state != UA_STATUSCODE_GOOD) {
            messages << PluginInterface::Message(90000 | state, QString(UA_StatusCode_name(state)), QtCriticalMsg);
        }
    }
    
    PluginInterface::updateMessages(messages);
}

void OPCUAPlugin::disconnectOPCUA()
{
    if (_interface)
        _interface->disconnectOPCUA();
}

void OPCUAPlugin::setValue(const QString &name, const QVariant &value)
{
    // check for special messages
    if (!_interface && name == "OPCUA/configure") {
        QHash<QString, QVariant> config = value.toHash();
        
        // convert in and out config
        QVector<std::shared_ptr<OPCInputValue>> in_config_tmp = config["in_config"].value<QVector<std::shared_ptr<OPCInputValue>>>();
        QVector<std::shared_ptr<OPCOutputValue>> out_config_tmp = config["out_config"].value<QVector<std::shared_ptr<OPCOutputValue>>>();
        QVector<std::shared_ptr<OPCMethod>> method_config_tmp = config["method_config"].value<QVector<std::shared_ptr<OPCMethod>>>();
        auto in_config = std::vector<std::shared_ptr<OPCInputValue>>(in_config_tmp.begin(), in_config_tmp.end());
        auto out_config = std::vector<std::shared_ptr<OPCOutputValue>>(out_config_tmp.begin(), out_config_tmp.end());
        auto method_config = std::vector<std::shared_ptr<OPCMethod>>(method_config_tmp.begin(), method_config_tmp.end());

        // get config and create interface
		QHash<QString, QVariant> interface_config = config;
        config.remove("in_config");
        config.remove("out_config");
		
		// create interface
        if (config["type"] == "client") {
            _interface = std::shared_ptr<OPCUAClient>(new OPCUAClient(interface_config, in_config, out_config));
        } else if (config["type"] == "server") {
            _interface = std::shared_ptr<OPCUAServer>(new OPCUAServer(interface_config, in_config, out_config, method_config));
        }
        
        if (_interface) {
            // directly connect logic
            if (config.contains("logic") && config["logic"].canConvert<std::shared_ptr<Logic> >()) {
                std::shared_ptr<Logic> logic = config["logic"].value<std::shared_ptr<Logic> >();
                if (logic) {
                    _interface->setLogic(logic);
                    logic->setInterface(_interface);
                }
            }
            
            // needs to use connection, as widget stuff has to be done on main thread
            if (in_config.size() || out_config.size())
                emit setupWidget(_interface.get());

            // wire new interface
            connect(_interface.get(), &OPCUAInterface::valueChanged, this, &OPCUAPlugin::valueChanged);
            
            QTimer::singleShot(1000, _interface.get(), &OPCUAInterface::connectOPCUA);
        } else {
            qWarning() << config << "does not contain a valid interface type!";
        }

    } else if (_interface && name == "OPCUA/setup") {
        QHash<QString, QVariant> config = value.toHash();

        // convert in and out config
        QVector<std::shared_ptr<OPCInputValue>> in_config_tmp = config["in_config"].value<QVector<std::shared_ptr<OPCInputValue>>>();
        QVector<std::shared_ptr<OPCOutputValue>> out_config_tmp = config["out_config"].value<QVector<std::shared_ptr<OPCOutputValue>>>();
        QVector<std::shared_ptr<OPCMethod>> method_config_tmp = config["method_config"].value<QVector<std::shared_ptr<OPCMethod>>>();
        auto in_config = std::vector<std::shared_ptr<OPCInputValue>>(in_config_tmp.begin(), in_config_tmp.end());
        auto out_config = std::vector<std::shared_ptr<OPCOutputValue>>(out_config_tmp.begin(), out_config_tmp.end());
        auto method_config = std::vector<std::shared_ptr<OPCMethod>>(method_config_tmp.begin(), method_config_tmp.end());
        
        _interface->updateConfig(in_config, out_config, method_config);

        // needs to use connection, as widget stuff has to be done on main thread
        if (in_config.size() || out_config.size())
            emit setupWidget(_interface.get());
    }
}

void OPCUAPlugin::setValues(const QHash<QString, QVariant> &values)
{
    defaultSetValues(values);
}
