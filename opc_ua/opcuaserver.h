#ifndef OPCUASERVER_H
#define OPCUASERVER_H

#include "opcuainterface.h"

#include <open62541/server.h>
#include <open62541/server_config_default.h>

class OPCUAServer : public OPCUAInterface
{
    Q_OBJECT
public:
    explicit OPCUAServer(const QHash<QString, QVariant> &config, 
        const std::vector<std::shared_ptr<OPCInputValue>> &in_config, 
        const std::vector<std::shared_ptr<OPCOutputValue>> &out_config,
        const std::vector<std::shared_ptr<OPCMethod>> &method_config);

    void updateConfig(const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config, const std::vector<std::shared_ptr<OPCMethod>>& method_config) override;
signals:

public slots:
    void connectOPCUA() override;
    void disconnectOPCUA() override;

private:
    // triggers async reading and writing of data to the controller.
    void readDataInternal() override;
    std::map<std::shared_ptr<OPCValue>, UA_StatusCode> writeData(const std::map<std::shared_ptr<OPCValue>, UA_Variant> &data) override;
    
    //config
    std::vector<std::shared_ptr<OPCValue>> _config;
    std::vector<std::shared_ptr<OPCMethod>> _method_config;

    void execute();
    UA_StatusCode setup();
    void setConfig();
    void addMethod(const std::shared_ptr<OPCMethod> method);

    static void readValue(UA_Server *server,
        const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeid, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data);
   
    static void writeValue(UA_Server *server,
        const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data);

    static UA_StatusCode methodCallback(UA_Server *server,
            const UA_NodeId *sessionId, void *sessionHandle,
            const UA_NodeId *methodId, void *methodContext,
            const UA_NodeId *objectId, void *objectContext,
            size_t inputSize, const UA_Variant *input,
            size_t outputSize, UA_Variant *output);
    
    UA_Boolean _running;
    UA_Server* _server;

    std::thread _tServer;
};

#endif // OPCUASERVER_H
