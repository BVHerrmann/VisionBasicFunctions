#ifndef PLUGINS_PNIODEVICE_PNIODEVICE_H_
#define PLUGINS_PNIODEVICE_PNIODEVICE_H_

#include "opcuainterface.h"

#include <QtCore>

#include <open62541/client_highlevel.h>

class OPCUAClient : public OPCUAInterface
{
    Q_OBJECT
public:
    explicit OPCUAClient(const QHash<QString, QVariant> &config, const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config);
    virtual ~OPCUAClient();

    // subscription callbacks
    void dataChangeNotification(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value);
    void deleteMonitoredItem(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext);
    
signals:

public slots:
    void connectOPCUA() override;
    void disconnectOPCUA() override;

private:
    QString _endpoint;
    UA_Client *_client = nullptr;
    UA_UInt32 _subscriptionId = 0;
    
    QElapsedTimer _reconnect_timer;
    UA_StatusCode connectToServer();
    UA_StatusCode disconnectFromServer();
    UA_StatusCode registerSubscriptions(UA_Client *client);
    
    // triggers async reading and writing of data to the controller.
	void readDataInternal() override;
    std::map<std::shared_ptr<OPCValue>, UA_StatusCode> writeData(const std::map<std::shared_ptr<OPCValue>, UA_Variant> &data) override;
};

#endif  // PLUGINS_PNIODEVICE_PNIODEVICE_H_
