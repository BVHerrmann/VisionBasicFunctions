#include "opcuaserver.h"

template<class> inline constexpr bool always_false_v = false;

OPCUAServer::OPCUAServer(const QHash<QString, QVariant> &config, 
    const std::vector<std::shared_ptr<OPCInputValue>> &in_config, 
    const std::vector<std::shared_ptr<OPCOutputValue>> &out_config,
    const std::vector<std::shared_ptr<OPCMethod>> &method_config) :
    OPCUAInterface(config, in_config, out_config)
{
    _method_config = method_config;

}

void OPCUAServer::connectOPCUA()
{
    auto statusCode = UA_STATUSCODE_GOOD;

    statusCode = setup();
    if (statusCode != UA_STATUSCODE_GOOD) {
        qWarning() << "Failed to setup the OPC UA Server:" << statusCode;
        updateState(statusCode);
        return;
    }

    setConfig();

    _tServer = std::thread(&OPCUAServer::execute, this);
    _machine_state = PluginInterface::Production;

    startTimer();
}

void OPCUAServer::updateConfig(const std::vector<std::shared_ptr<OPCInputValue>>& in_config, const std::vector<std::shared_ptr<OPCOutputValue>>& out_config, const std::vector<std::shared_ptr<OPCMethod>>& method_config)
{
    OPCUAInterface::updateConfig(in_config, out_config);

    std::vector<std::shared_ptr<OPCValue>> newConfig;
    std::copy(std::begin(in_config), std::end(in_config), std::back_inserter(newConfig));
    std::copy(std::begin(out_config), std::end(out_config), std::back_inserter(newConfig));

    //clear server
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;

    for (auto element : _config) {

        //check if new Config contains parts of the old config
        auto foundItem = std::find_if(newConfig.begin(), newConfig.end(), [&](const std::shared_ptr<OPCValue> &it)
        {
            auto newInput = std::dynamic_pointer_cast<OPCInputValue>(it);
            auto oldInput = std::dynamic_pointer_cast<OPCInputValue>(element);

            if (newInput && oldInput) {
                return (it->name() == element->name())
                    && (it->nodeId() == element->nodeId())
                    && (it->description() == element->description())
                    && (it->nsUri() == element->nsUri())
                    && (newInput->identifier() == oldInput->identifier())
                    && newInput == oldInput;
            } else {

                auto newOutput = std::dynamic_pointer_cast<OPCOutputValue>(it);
                auto oldOutput = std::dynamic_pointer_cast<OPCOutputValue>(element);

                if (newOutput && oldOutput) {
                    return (it->name() == element->name())
                        && (it->nodeId() == element->nodeId())
                        && (it->description() == element->description())
                        && (it->nsUri() == element->nsUri())
                        && (newOutput->identifier() == oldOutput->identifier())
                        && newOutput == oldOutput;
                }
            }
                
            return false;
        });

        if (foundItem != newConfig.end())
        {
            continue;
        }

        //Delete variable nodes
        const QString nodeSeparator = "/";
        const auto pathElements = element->nodeId().split(nodeSeparator);

        size_t nsIndex = 0;
        statusCode = UA_Server_getNamespaceByName(_server, UA_String_fromChars(element->nsUri().toUtf8().data()), &nsIndex);
        if (statusCode != UA_STATUSCODE_GOOD) {
            qWarning() << "Failed to get namespace for Node" << element->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
        }

        QString nodePath;
        for (auto it : pathElements) {
            nodePath.append((it == pathElements.constFirst()) ? it : nodeSeparator + it);

            //check if the new Config contains the same node structure
            bool containsPath = false;
            for (auto it : newConfig) {
                if (it->nodeId().contains(nodePath)) {
                    containsPath = true;
                    break;
                }
            }
            for (auto method : method_config) {
                if (method->nodeId().contains(nodePath)) {
                    containsPath = true;
                    break;
                }
            }

            if (containsPath) {
                continue;
            }

            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, nodePath.toUtf8().data());
            statusCode = UA_Server_deleteNode(_server, nodeId, true);
            if (statusCode != UA_STATUSCODE_GOOD && statusCode != UA_STATUSCODE_BADNODEIDUNKNOWN) {
                qWarning() << "Failed to delete Node" << element->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
            }
        }
    }

    //compare method config
    for (auto method : _method_config) {
        //check if new method config contains parts of the old config

        auto foundItem = std::find_if(method_config.begin(), method_config.end(), [&](const std::shared_ptr<OPCMethod> &it)
        {
            return (it == method)
                && (it->name() == method->name())
                && (it->nodeId() == method->nodeId())
                && (it->description() == method->description())
                && (it->nsUri() == method->nsUri())
                && (it->inputName() == method->inputName())
                && (it->inputDescription() == method->inputDescription())
                && (it->outputName() == method->outputName())
                && (it->outputDescription() == method->outputDescription());
        });

        if (foundItem != method_config.end())
        {
            continue;
        }

        //delete method nodes
        const QString nodeSeparator = "/";
        const auto pathElements = method->nodeId().split(nodeSeparator);

        size_t nsIndex = 0;
        statusCode = UA_Server_getNamespaceByName(_server, UA_String_fromChars(method->nsUri().toUtf8().data()), &nsIndex);
        if (statusCode != UA_STATUSCODE_GOOD) {
            qWarning() << "Failed to get namespace for Node" << method->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
        }

        QString nodePath;
        for (auto it : pathElements) {
            nodePath.append((it == pathElements.constFirst()) ? it : nodeSeparator + it);

            //check if the new Config contains the same node structure
            bool containsPath = false;
            for (auto it : newConfig) {
                if (it->nodeId().contains(nodePath)) {
                    containsPath = true;
                    break;
                }
            }
            for (auto method : method_config) {
                if (method->nodeId().contains(nodePath)) {
                    containsPath = true;
                    break;
                }
            }

            if (containsPath) {
                continue;
            }

            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, nodePath.toUtf8().data());
            statusCode = UA_Server_deleteNode(_server, nodeId, true);
            if (statusCode != UA_STATUSCODE_GOOD && statusCode != UA_STATUSCODE_BADNODEIDUNKNOWN) {
                qWarning() << "Failed to delete Node" << method->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
            }
        }
    }

    //set new method config
    _method_config = method_config;

    setConfig();
}

void OPCUAServer::disconnectOPCUA()
{
    stopTimer();

    _running = false;
    if (_tServer.joinable()) {
        _tServer.join();
    }
}

void OPCUAServer::readDataInternal()
{
    if (_server) {
        if (auto logic = _logic.lock()) {
            logic->doWorkInternal();
        } else {
            writeOutputs();
        }
    }
}

std::map<std::shared_ptr<OPCValue>, UA_StatusCode> OPCUAServer::writeData(const std::map<std::shared_ptr<OPCValue>, UA_Variant> &data)
{
    /*
    std::map<std::shared_ptr<OPCValue>, UA_StatusCode> state;
    for (const auto &[key, value] : data) {
        state[key] = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if (!_server) {
        qWarning() << "Server not started!";
        return state;
    }

    for (auto element : data) {
        UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
        size_t nsIndex = 0;
        statusCode = UA_Server_getNamespaceByName(_server, UA_String_fromChars(element.first->nsUri().toUtf8().data()), &nsIndex);

        if (statusCode == UA_STATUSCODE_GOOD) {
            UA_NodeId currentNodeId = UA_NODEID_STRING_ALLOC(nsIndex, element.first->nodeId().toUtf8().data());
            statusCode = UA_Server_writeValue(_server, currentNodeId, element.second);
        }

        state[element.first] = statusCode;
    }

    return state;
    */

    return std::map<std::shared_ptr<OPCValue>, UA_StatusCode>();
}

void OPCUAServer::execute()
{
    _running = true;
    qDebug() << "Start UPCUA Server";
    UA_Server_run(_server, &_running);

    qDebug() << "UPCUA Server stopped";
    UA_Server_delete(_server);
}

UA_StatusCode OPCUAServer::setup()
{
    _server = UA_Server_new();
    UA_ServerConfig* config = UA_Server_getConfig(_server);
    return UA_ServerConfig_setDefault(config);
}

void OPCUAServer::setConfig()
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;

    std::vector<std::shared_ptr<OPCValue>> config;
    auto inConfig = getInConfig();
    auto outConfig = getOutConfig();
    std::copy(std::begin(inConfig), std::end(inConfig), std::back_inserter(config));
    std::copy(std::begin(outConfig), std::end(outConfig), std::back_inserter(config));
    _config = config;
    
    for (auto element : config) {

        std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string, std::monostate> value;
        bool isInput = false;

        if (auto input = std::dynamic_pointer_cast<OPCInputValue>(element)) {
            value = input->value();
            isInput = true;
        } else if (auto output = std::dynamic_pointer_cast<OPCOutputValue>(element)) {
            value = output->value();
        } else {
            qFatal("Wrong OPC Value type");
            return;
        }

        //find namespace
        size_t nsIndex = 0;
        statusCode = UA_Server_getNamespaceByName(_server, UA_String_fromChars(element->nsUri().toUtf8().data()), &nsIndex);
        if (statusCode == UA_STATUSCODE_BADNOTFOUND) {
            nsIndex = UA_Server_addNamespace(_server, element->nsUri().toUtf8().data());
        } else if (statusCode != UA_STATUSCODE_GOOD) {
            qWarning() << "Failed to get Namespace for Node" << element->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
            continue;
        }

        //parse node id
        const QString nodeSeparator = "/";
        const auto pathElements = element->nodeId().split(nodeSeparator);

        QString nodePath;
        auto parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        for (auto it : pathElements) {
            nodePath.append((it == pathElements.constFirst()) ? it : nodeSeparator + it);

            if (it == pathElements.constLast()) {
                // Define the attribute of the variable node
                auto attr = UA_VariableAttributes_default;
                attr.displayName = UA_LOCALIZEDTEXT_ALLOC("", element->name().toUtf8().data());
                attr.description = UA_LOCALIZEDTEXT_ALLOC("", element->description().toUtf8().data());
                attr.accessLevel = (isInput) ? UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE : UA_ACCESSLEVELMASK_READ;

                std::visit([&attr](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_BOOLEAN]);
                    } else if constexpr (std::is_same_v<T, int8_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_SBYTE].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_SBYTE]);
                    } else if constexpr (std::is_same_v<T, uint8_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_BYTE]);
                    } else if constexpr (std::is_same_v<T, int16_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_INT16]);
                    } else if constexpr (std::is_same_v<T, uint16_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_UINT16]);
                    } else if constexpr (std::is_same_v<T, int32_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_INT32]);
                    } else if constexpr (std::is_same_v<T, uint32_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_UINT32]);
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_INT64].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_INT64]);
                    } else if constexpr (std::is_same_v<T, uint64_t>) {
                        attr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_UINT64]);
                    } else if constexpr (std::is_same_v<T, float>) {
                        attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_FLOAT]);
                    } else if constexpr (std::is_same_v<T, double>) {
                        attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &arg, &UA_TYPES[UA_TYPES_DOUBLE]);
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                        UA_Variant_setScalarCopy(&attr.value, &UA_String_fromChars(arg.c_str()), &UA_TYPES[UA_TYPES_STRING]);
                    } else if constexpr (std::is_same_v<T, std::monostate>) {
                        qWarning() << "Invalid OPC-UA Output!";
                    } else {
                        static_assert(always_false_v<T>, "non-exhaustive visitor!");
                    }
                }, value);

                // Add the variable node to the information model
                UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, nodePath.toUtf8().data());
                UA_QualifiedName nodeName = UA_QUALIFIEDNAME_ALLOC(nsIndex, nodePath.toUtf8().data());
                UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
                statusCode = UA_Server_addVariableNode(_server, nodeId, parentNodeId, parentReferenceNodeId, nodeName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, element.get(), NULL);
                if (statusCode != UA_STATUSCODE_GOOD && statusCode != UA_STATUSCODE_BADNODEIDEXISTS) {
                    qWarning() << "Failed to create Variable Node" << nodePath << "with:" << QString::number(statusCode, 16).toUpper();
                } else {
                    element->set_valid(true);
                }

                if (statusCode != UA_STATUSCODE_BADNODEIDEXISTS) {
                    // Add Callback for write requests
                    UA_ValueCallback callback;
                    callback.onRead = (!isInput) ? readValue : NULL;
                    callback.onWrite = (isInput) ? writeValue : NULL;
                    statusCode = UA_Server_setVariableNode_valueCallback(_server, nodeId, callback);
                    if (statusCode != UA_STATUSCODE_GOOD) {
                        qWarning() << "Failed to add callback to Node" << nodePath << "with:" << QString::number(statusCode, 16).toUpper();
                    }
                }

            } else {
                // Define the attribute of the object node
                auto oAttr = UA_ObjectAttributes_default;
                oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("", it.toUtf8().data());

                // Add objectnode to the information model
                UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, nodePath.toUtf8().data());
                UA_QualifiedName nodeName = UA_QUALIFIEDNAME_ALLOC(nsIndex, nodePath.toUtf8().data());
                UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
                statusCode = UA_Server_addObjectNode(_server, nodeId, parentNodeId, parentReferenceNodeId, nodeName, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), oAttr, NULL, NULL);
                if (statusCode != UA_STATUSCODE_GOOD && statusCode != UA_STATUSCODE_BADNODEIDEXISTS) {
                    qWarning() << "Failed to create Object Node with:" << QString::number(statusCode, 16).toUpper();
                }

                parentNodeId = nodeId;
            }
        }
    }

    for (auto method : _method_config) {
        addMethod(method);
    }
}

void OPCUAServer::addMethod(const std::shared_ptr<OPCMethod> method)
{
    //find namespace
    size_t nsIndex = 0;
    auto statusCode = UA_Server_getNamespaceByName(_server, UA_String_fromChars(method->nsUri().toUtf8().data()), &nsIndex);
    if (statusCode == UA_STATUSCODE_BADNOTFOUND) {
        nsIndex = UA_Server_addNamespace(_server, method->nsUri().toUtf8().data());
    } else if (statusCode != UA_STATUSCODE_GOOD) {
        qWarning() << "Failed to get Namespace for Node" << method->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
        return;
    }

    const QString nodeSeparator = "/";
    const auto pathElements = method->nodeId().split(nodeSeparator);

    QString nodePath;
    auto parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    for (auto it : pathElements) {
        nodePath.append((it == pathElements.constFirst()) ? it : nodeSeparator + it);

        if (it == pathElements.constLast()) {

            auto numInputArgument = 0;
            auto numOutputArgument = 0;

            UA_Argument* inputArgument = NULL;
            UA_Argument* outputArgument = NULL;

            UA_Argument outputArgumentTemp;
            UA_Argument_init(&outputArgumentTemp);
            outputArgumentTemp.name = UA_STRING_ALLOC(method->outputName().toUtf8().data());
            outputArgumentTemp.description = UA_LOCALIZEDTEXT_ALLOC("", method->outputDescription().toUtf8().data());
            outputArgumentTemp.valueRank = UA_VALUERANK_SCALAR;

            UA_Argument inputArgumentTemp;
            UA_Argument_init(&inputArgumentTemp);
            inputArgumentTemp.name = UA_STRING_ALLOC(method->inputName().toUtf8().data());
            inputArgumentTemp.description = UA_LOCALIZEDTEXT_ALLOC("", method->inputDescription().toUtf8().data());
            inputArgumentTemp.valueRank = UA_VALUERANK_SCALAR;

            switch (method->type()) {
            case 1: {

                break;
            }
            case 2: {
                outputArgumentTemp.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
                outputArgument = &outputArgumentTemp;

                numInputArgument = 0;
                numOutputArgument = 1;
                break;
            }
            case 3: {
                inputArgumentTemp.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
                inputArgument = &inputArgumentTemp;

                outputArgumentTemp.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
                outputArgument = &outputArgumentTemp;

                numInputArgument = 1;
                numOutputArgument = 1;
                break;
            }
            case 4: {
                inputArgumentTemp.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                inputArgument = &inputArgumentTemp;

                outputArgumentTemp.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
                outputArgument = &outputArgumentTemp;

                numInputArgument = 1;
                numOutputArgument = 1;
                break;
            }
            default: {
                qWarning() << "Method type not found";
                return;
            }
            }

            // Define the attribute of the method node
            auto attr = UA_MethodAttributes_default;
            attr.displayName = UA_LOCALIZEDTEXT_ALLOC("", method->name().toUtf8().data());
            attr.description = UA_LOCALIZEDTEXT_ALLOC("", method->description().toUtf8().data());
            attr.executable = true;
            attr.userExecutable = true;

            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, method->nodeId().toUtf8().data());
            UA_QualifiedName nodeName = UA_QUALIFIEDNAME_ALLOC(nsIndex, method->nodeId().toUtf8().data());

            UA_Server_addMethodNode(_server, nodeId,
                parentNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                nodeName, attr, &methodCallback,
                numInputArgument, inputArgument, numOutputArgument, outputArgument, method.get(), NULL);

        } else {
            // Define the attribute of the object node
            auto oAttr = UA_ObjectAttributes_default;
            oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("", it.toUtf8().data());

            // Add objectnode to the information model
            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, nodePath.toUtf8().data());
            UA_QualifiedName nodeName = UA_QUALIFIEDNAME_ALLOC(nsIndex, nodePath.toUtf8().data());
            UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
            statusCode = UA_Server_addObjectNode(_server, nodeId, parentNodeId, parentReferenceNodeId, nodeName, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), oAttr, NULL, NULL);
            if (statusCode != UA_STATUSCODE_GOOD && statusCode != UA_STATUSCODE_BADNODEIDEXISTS) {
                qWarning() << "Failed to create Object Node with:" << QString::number(statusCode, 16).toUpper();
            }

            parentNodeId = nodeId;
        }
    }
}

void OPCUAServer::readValue(UA_Server * server,
    const UA_NodeId * sessionId, void * sessionContext,
    const UA_NodeId * nodeid, void * nodeContext,
    const UA_NumericRange * range, const UA_DataValue * data)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;

    if (auto base = static_cast<OPCValue*>(nodeContext)) {
        if (auto identifier = dynamic_cast<OPCOutputValue *>(base)) {
            auto value = identifier->value();

            if (!std::holds_alternative<std::monostate>(value)) {
                UA_Variant var;
                UA_Variant_init(&var);

                std::visit([&var](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_BOOLEAN]);
                    } else if constexpr (std::is_same_v<T, int8_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_SBYTE]);
                    } else if constexpr (std::is_same_v<T, uint8_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_BYTE]);
                    } else if constexpr (std::is_same_v<T, int16_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_INT16]);
                    } else if constexpr (std::is_same_v<T, uint16_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_UINT16]);
                    } else if constexpr (std::is_same_v<T, int32_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_INT32]);
                    } else if constexpr (std::is_same_v<T, uint32_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_UINT32]);
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_INT64]);
                    } else if constexpr (std::is_same_v<T, uint64_t>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_UINT64]);
                    } else if constexpr (std::is_same_v<T, float>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_FLOAT]);
                    } else if constexpr (std::is_same_v<T, double>) {
                        UA_Variant_setScalarCopy(&var, &arg, &UA_TYPES[UA_TYPES_DOUBLE]);
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        UA_Variant_setScalarCopy(&var, &UA_String_fromChars(arg.c_str()), &UA_TYPES[UA_TYPES_STRING]);
                    } else if constexpr (std::is_same_v<T, std::monostate>) {
                        qWarning() << "Invalid OPC-UA Output!";
                    } else {
                        static_assert(always_false_v<T>, "non-exhaustive visitor!");
                    }
                }, value);

                statusCode = UA_Server_writeValue(server, *nodeid, var);
                identifier->set_valid(statusCode == UA_STATUSCODE_GOOD);

                if (statusCode != UA_STATUSCODE_GOOD) {
                    qWarning() << "Failed to read Data with status code:" << QString::number(statusCode, 16).toUpper();
                }
            }
        }
    }
}

void OPCUAServer::writeValue(UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range, const UA_DataValue *data)
{
    if (auto base = static_cast<OPCValue*>(nodeContext)) {
        if (auto identifier = dynamic_cast<OPCInputValue *>(base)) {
            if (data) {
                std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string> v;
                bool valid = false;

                if (data->hasValue) {
                    switch (data->value.type->typeIndex) {
                    case UA_TYPES_BOOLEAN:
                        v = *(UA_Boolean *)data->value.data;
                        break;
                    case UA_TYPES_SBYTE:
                        v = *(UA_SByte *)data->value.data;
                        break;
                    case UA_TYPES_BYTE:
                        v = *(UA_Byte *)data->value.data;
                        break;
                    case UA_TYPES_INT16:
                        v = *(UA_Int16 *)data->value.data;
                        break;
                    case UA_TYPES_UINT16:
                        v = *(UA_UInt16 *)data->value.data;
                        break;
                    case UA_TYPES_INT32:
                        v = *(UA_Int32 *)data->value.data;
                        break;
                    case UA_TYPES_UINT32:
                        v = *(UA_UInt32 *)data->value.data;
                        break;
                    case UA_TYPES_INT64:
                        v = *(UA_Int64 *)data->value.data;
                        break;
                    case UA_TYPES_UINT64:
                        v = *(UA_UInt64 *)data->value.data;
                        break;
                    case UA_TYPES_FLOAT:
                        v = *(UA_Float *)data->value.data;
                        break;
                    case UA_TYPES_DOUBLE:
                        v = *(UA_Double *)data->value.data;
                        break;
                    case UA_TYPES_STRING: {
                        auto uaString = *(UA_String *)data->value.data;
                        v = std::string((char *)uaString.data, uaString.length);
                        break;
                    }
                    default:
                        qWarning() << "Unhandeled data type" << data->value.type->typeIndex << "for" << identifier->nodeId();
                    }
                }

                if (data->hasStatus) {
                    valid = (data->status == UA_STATUSCODE_GOOD);
                } else {
                    valid = true;
                }

                identifier->update(v, valid);
            } else {
                identifier->set_valid(false);
                qWarning() << "No value!";
            }
        }
    } else {
        qWarning() << "Invalid OPC Value" << nodeContext;
    }
}

UA_StatusCode OPCUAServer::methodCallback(UA_Server * server, 
    const UA_NodeId * sessionId, void * sessionHandle, 
    const UA_NodeId * methodId, void * methodContext, 
    const UA_NodeId * objectId, void * objectContext, 
    size_t inputSize, const UA_Variant * input, size_t outputSize, 
    UA_Variant * output)
{    
    if (auto method = static_cast<OPCMethod*>(methodContext)) {
        

        switch (method->type()) {
        case 1: {
            method->v1()();
            break;
        }
        case 2: {
            auto returnValue = method->v2()();
            UA_Variant_setScalarCopy(output, &returnValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            break;
        }
        case 3: {
            UA_Boolean inputValue = *(UA_Boolean *)input->data;
            auto returnValue = method->v3()(inputValue);
            UA_Variant_setScalarCopy(output, &returnValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            break;
        }
        case 4: {
            auto uaString = *(UA_String *)input->data;
            auto inputValue = std::string((char *)uaString.data, uaString.length);

            auto returnValue = method->v4()(inputValue);
            UA_Variant_setScalarCopy(output, &returnValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            break;
        }
        default: {
            qWarning() << "Method type not found";
            return UA_STATUSCODE_BADUNEXPECTEDERROR;
        }
        }
    }
    
    return UA_STATUSCODE_GOOD;
}
