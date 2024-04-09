#include "opcuaclient.h"

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>


static void logCallback(void *logContex, UA_LogLevel level, UA_LogCategory category, const char *msg, va_list args)
{
    QString message;
    QMessageLogContext context(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, nullptr);
    switch (category) {
        case UA_LOGCATEGORY_NETWORK:
            message = "Network: " + QString::vasprintf(msg, args);
            break;
        case UA_LOGCATEGORY_SECURECHANNEL:
            message = "Secure Channel: " + QString::vasprintf(msg, args);
            break;
        case UA_LOGCATEGORY_SESSION:
            message = "Session: " + QString::vasprintf(msg, args);
            break;
        case UA_LOGCATEGORY_SERVER:
            message = "Server: " + QString::vasprintf(msg, args);
            break;
        case UA_LOGCATEGORY_CLIENT:
            message = "Client: " + QString::vasprintf(msg, args);
            break;
        case UA_LOGCATEGORY_USERLAND:
            message = "Userland: " + QString::vasprintf(msg, args);
            break;
        case UA_LOGCATEGORY_SECURITYPOLICY:
            message = "Security Policy: " + QString::vasprintf(msg, args);
            break;
        default:
            qWarning() << "Unknown OPCUA log category" << category;
    }
    switch (level) {
        case UA_LOGLEVEL_TRACE:
        case UA_LOGLEVEL_DEBUG:
            qt_message_output(QtDebugMsg, context, message);
            break;
        case UA_LOGLEVEL_INFO:
            qt_message_output(QtInfoMsg, context, message);
            break;
        case UA_LOGLEVEL_WARNING:
            qt_message_output(QtWarningMsg, context, message);
            break;
        case UA_LOGLEVEL_ERROR:
            qt_message_output(QtCriticalMsg, context, message);
            break;
        case UA_LOGLEVEL_FATAL:
            qt_message_output(QtFatalMsg, context, message);
            break;
        default:
            qWarning() << "Unknown OPCUA log level" << level;
    }
};

static void dataChangeNotificationCallback(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value)
{
	OPCUAClient *context = qobject_cast<OPCUAClient *>((QObject *)subContext);
    if (context) {
        context->dataChangeNotification(client, subId, subContext, monId, monContext, value);
    } else {
        qWarning() << "No context for Subscription!";
    }
}

static void deleteMonitoredItemCallback(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext)
{
    OPCUAClient *context = qobject_cast<OPCUAClient *>((QObject *)subContext);
    if (context) {
        context->deleteMonitoredItem(client, subId, subContext, monId, monContext);
    } else {
        qWarning() << "No context for Subscription!";
    }
}


OPCUAClient::OPCUAClient(const QHash<QString, QVariant> &config, const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config) :
    OPCUAInterface(config, in_config, out_config)
{
    _endpoint = config["endpoint"].toString();
}

OPCUAClient::~OPCUAClient()
{
    if (_client) {
        disconnectFromServer();
    }
}

void OPCUAClient::connectOPCUA()
{
    startTimer();
}

UA_StatusCode OPCUAClient::connectToServer()
{
	qWarning() << "Connect to server";

	UA_Client *client = UA_Client_new();

	// prepare config
	UA_ClientConfig *config = UA_Client_getConfig(client);
	UA_ClientConfig_setDefault(config);

    config->timeout = 2500;
	config->logger.log = logCallback;
    config->stateCallback = [](UA_Client *client, UA_SecureChannelState channelState, UA_SessionState sessionState, UA_StatusCode connectStatus) {
        
        // set status to bad if not connected completely
        if (channelState != UA_SECURECHANNELSTATE_OPEN || sessionState != UA_SESSIONSTATE_ACTIVATED || connectStatus != UA_STATUSCODE_GOOD) {
            OPCUAClient *context = static_cast<OPCUAClient *>(UA_Client_getContext(client));
            if (context) {
                for (auto &config : context->getInConfig()) {
                    config->set_valid(false);
                }
                for (auto &config : context->getOutConfig()) {
                    config->set_valid(false);
                }
            }
        }
        
        switch (channelState) {
            case UA_SECURECHANNELSTATE_OPEN:
                break;
            default:
                qWarning() << "Secore Channel status not good:" << channelState;
        }
        switch (sessionState) {
            case UA_SESSIONSTATE_ACTIVATED:
            {
                qDebug() << "Client state changed to session activated";
                OPCUAClient *context = static_cast<OPCUAClient *>(UA_Client_getContext(client));
                if (context) {
                    context->registerSubscriptions(client);
                }
                else {
                    qWarning() << "No Client context!";
                }
                break;
            }
            default:
                qWarning() << "Session status not good:" << sessionState;
        }
        switch (connectStatus) {
            case UA_STATUSCODE_GOOD:
                break;
            default:
                qWarning() << "Connection status not good:" << UA_StatusCode_name(connectStatus);
        }
    };
    config->connectivityCheckInterval = 2500;
    config->inactivityCallback = [](UA_Client *client) {
        qWarning() << "Client inactive!";
    };
    config->clientContext = this;
    config->subscriptionInactivityCallback = [](UA_Client *client, UA_UInt32 subscriptionId, void *subContext) {
        qWarning() << "Subscription with id" << subscriptionId << "and context" << subContext << "is inactive";
    };
    
    // connect
    UA_StatusCode status = UA_Client_connect(client, _endpoint.toUtf8().data());
    if (status == UA_STATUSCODE_GOOD) {
        qDebug() << "Connected to server";
        
        _client = client;
    } else {
        qWarning() << "Connection failed:" << UA_StatusCode_name(status);
        
        UA_Client_delete(client);
        client = nullptr;
    }
    
    return status;
}

UA_StatusCode OPCUAClient::disconnectFromServer()
{
	qWarning() << __FUNCTION__;

	if (_client) {
		UA_Client *client = _client;
		_client = nullptr;

		UA_StatusCode status = UA_Client_disconnect(client);

		UA_Client_delete(client);

		if (status == UA_STATUSCODE_GOOD) {
			qDebug() << "Disconnected from server";
		}
		else {
			qWarning() << "Disconnection failed:" << UA_StatusCode_name(status);
		}
        
        updateNodeStates(UA_STATUSCODE_BADNOTCONNECTED);
        
		return status;
	}

	return UA_STATUSCODE_GOOD;
}

UA_StatusCode OPCUAClient::registerSubscriptions(UA_Client *client)
{
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_Client_StatusChangeNotificationCallback statusChangeNotificationCallback = [](UA_Client *client, UA_UInt32 subId, void *subContext, UA_StatusChangeNotification *notification) {
        qDebug() << "Subscription status changed:" << UA_StatusCode_name(notification->status);
    };
    UA_Client_DeleteSubscriptionCallback deleteSubscriptionCallback = [](UA_Client *client, UA_UInt32 subId, void *subContext) {
        qDebug() << "Subscription deleted:" << subId;
    };
    
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request, this, statusChangeNotificationCallback, deleteSubscriptionCallback);
    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_StatusCode status;
        UA_StatusCode_copy(&response.responseHeader.serviceResult, &status);
        qWarning() << "Could not create subscriptions" << UA_StatusCode_name(response.responseHeader.serviceResult);
        
        // cleanup
        UA_CreateSubscriptionRequest_clear(&request);
        UA_CreateSubscriptionResponse_clear(&response);
        
        return status;
    }
    
    UA_UInt32_copy(&response.subscriptionId, &_subscriptionId);
    qDebug() << "Created subscription succeeded. ID:" << _subscriptionId;
    
    // cleanup
    UA_CreateSubscriptionRequest_clear(&request);
    UA_CreateSubscriptionResponse_clear(&response);
    
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    std::vector<UA_MonitoredItemCreateRequest> items;
    std::vector<UA_Client_DataChangeNotificationCallback> callbacks;
    std::vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbacks;
    std::vector<void *> contexts;
    
    for (const auto &config : _in_config) {
        UA_UInt16 nsIndex = 0;
        auto statusCode = UA_Client_NamespaceGetIndex(client, &UA_String_fromChars(config->nsUri().toUtf8().data()), &nsIndex);
        if (statusCode != UA_STATUSCODE_GOOD) {
            qWarning() << "Failed to get Namespace for Node" << config->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
        } else {
            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, config->nodeId().toUtf8().data());

            items.push_back(UA_MonitoredItemCreateRequest_default(nodeId));
            callbacks.push_back(dataChangeNotificationCallback);
            contexts.push_back(config.get());
            deleteCallbacks.push_back(deleteMonitoredItemCallback);
        }
    }
    
    // Prepare the request
    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = _subscriptionId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items.data();
    createRequest.itemsToCreateSize = items.size();
    UA_CreateMonitoredItemsResponse createResponse = UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts.data(), callbacks.data(), deleteCallbacks.data());
    if (createResponse.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        qWarning() << "Failed to add monitored items:" << UA_StatusCode_name(createResponse.responseHeader.serviceResult);
        status = createResponse.responseHeader.serviceResult;
    }
    
    for (int i=0; i < createResponse.resultsSize; ++i) {
        // assuming response size and order is same as request. not sure if true, but there seems no other way
        auto identifier = reinterpret_cast<OPCInputValue *>(contexts[i]);
        identifier->set_valid(createResponse.results[i].statusCode == UA_STATUSCODE_GOOD);
        
        if (createResponse.results[i].statusCode != UA_STATUSCODE_GOOD) {
            qDebug() << "Failed to create monitor item for" << identifier->nodeId() << ":" << UA_StatusCode_name(createResponse.results[i].statusCode);
        }
        
        status |= createResponse.results[i].statusCode;
    }
    
    // cleanup
    for (int i=0; i < items.size(); ++i) {
        UA_MonitoredItemCreateRequest_clear(&items[i]);
    }
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    return status;
}

void OPCUAClient::disconnectOPCUA()
{
    stopTimer();
}

void OPCUAClient::readDataInternal()
{
    if (_client) {
        UA_SecureChannelState channelState;
        UA_SessionState sessionState;
        UA_StatusCode connectStatus;
        UA_Client_getState(_client, &channelState, &sessionState, &connectStatus);
        
        if (connectStatus == UA_STATUSCODE_GOOD) {
            UA_StatusCode status = UA_Client_run_iterate(_client, 1000);
            if (status != UA_STATUSCODE_GOOD) {
                qWarning() << "Error in OPC UA_Client_run_iterate:" << status << UA_StatusCode_name(status);
            } else {
                // run main logic loop or just write outputs
                if (auto logic = _logic.lock()) {
                    logic->doWorkInternal();
                } else {
                    writeOutputs();
                }
            }
        } else {
            qWarning() << "Unrecoverable OPC Error:" << connectStatus << UA_StatusCode_name(connectStatus);
            disconnectFromServer();
        }
        updateState(connectStatus);
    } else {
        if (!_reconnect_timer.isValid() || _reconnect_timer.elapsed() > 1000 * 10) {
            _reconnect_timer.restart();
            UA_StatusCode status = connectToServer();
            if (status == UA_STATUSCODE_GOOD) {
                _machine_state = PluginInterface::Production;
            } else {
                _machine_state = PluginInterface::Initializing;
            }
            updateState(status);
        }
    }
}

std::map<std::shared_ptr<OPCValue>, UA_StatusCode> OPCUAClient::writeData(const std::map<std::shared_ptr<OPCValue>, UA_Variant> &data)
{
    std::map<std::shared_ptr<OPCValue>, UA_StatusCode> state;
    for (const auto &[key, value] : data) {
        state[key] = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    
	if (!_client) {
		qWarning() << "Client not connected!";
		return state;
	}

    UA_SecureChannelState channelState;
    UA_SessionState sessionState;
    UA_StatusCode connectStatus;
	UA_Client_getState(_client, &channelState, &sessionState, &connectStatus);
    
    switch (channelState) {
        case UA_SECURECHANNELSTATE_OPEN:
            break;
        default:
            qWarning() << "Secore Channel status not good:" << channelState;
            return state;
    }
    switch (sessionState) {
        case UA_SESSIONSTATE_ACTIVATED:
            break;
        default:
            qWarning() << "Session status not good:" << sessionState;
            return state;
    }
    switch (connectStatus) {
        case UA_STATUSCODE_GOOD:
            break;
        default:
            qWarning() << "Connection status not good:" << UA_StatusCode_name(connectStatus);
            return state;
    }
    
    // prepare data to write
    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    request.nodesToWriteSize = data.size();
    request.nodesToWrite = (UA_WriteValue* )UA_Array_new(request.nodesToWriteSize, &UA_TYPES[UA_TYPES_WRITEVALUE]);
    
    uint i = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        UA_UInt16 nsIndex = 0;
        auto statusCode = UA_Client_NamespaceGetIndex(_client, &UA_String_fromChars(it->first->nsUri().toUtf8().data()), &nsIndex);
        if (statusCode != UA_STATUSCODE_GOOD) {
            qWarning() << "Failed to get Namespace for Node" << it->first->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
        } else {
            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(nsIndex, it->first->nodeId().toUtf8().data());

            UA_NodeId_copy(&nodeId, &request.nodesToWrite[i].nodeId);
            UA_Variant_copy(&it->second, &request.nodesToWrite[i].value.value);

            request.nodesToWrite[i].attributeId = UA_ATTRIBUTEID_VALUE;
            request.nodesToWrite[i].value.hasValue = true;
        }

        ++i;
    }
    
    // perform write
	/*
	UA_Client_AsyncService_write(_client, request,
			UA_ClientAsyncServiceCallback callback,
			void *userdata, UA_UInt32 *requestId) {
*/
    UA_WriteResponse response = UA_Client_Service_write(_client, request);
    
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    for (int i=0; i < response.resultsSize; ++i) {
        UA_NodeId nodeId;
        UA_NodeId_copy(&request.nodesToWrite[i].nodeId, &nodeId);
        
        UA_StatusCode result_state;
        UA_StatusCode_copy(&response.results[i], &result_state);
        status |= result_state;
        
        for (auto& [config, value]: data) {
            UA_String nodeName = UA_String_fromChars(config->nodeId().toUtf8().data());
            UA_UInt16 nsIndex = 0;
            auto statusCode = UA_Client_NamespaceGetIndex(_client, &UA_String_fromChars(config->nsUri().toUtf8().data()), &nsIndex);
            if (statusCode != UA_STATUSCODE_GOOD) {
                qWarning() << "Failed to get Namespace for Node" << config->nodeId() << "with:" << QString::number(statusCode, 16).toUpper();
            }
            if (nsIndex == nodeId.namespaceIndex && UA_String_equal(&nodeName, &nodeId.identifier.string)) {
                if (result_state != UA_STATUSCODE_GOOD) {
                    qWarning() << "Failed to write" << config->nodeId() << ":" << result_state;
                }
                state[config] = result_state;
                break;
            }
        }
    }
    updateState(status);
    
    // cleanup
    UA_WriteRequest_clear(&request);
    UA_WriteResponse_clear(&response);
    
    return state;
}

void OPCUAClient::dataChangeNotification(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value)
{
    if (monContext) {
        auto identifier = reinterpret_cast<OPCInputValue *>(monContext);
        if (identifier) {
            processData(identifier, value);
        } else {
            qWarning() << "Invalid context" << monContext << "for monitored id" << monId;
        }
    } else {
        qWarning() << "No context for monitored item!";
    }
}

void OPCUAClient::deleteMonitoredItem(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext)
{
    if (monContext) {
        auto identifier = reinterpret_cast<OPCInputValue *>(monContext);
        if (identifier) {
            identifier->set_valid(false);
            qDebug() << "Deleted monitored item" << monId << "for subscription" << subId << ":" << identifier->nodeId();
        } else {
            qDebug() << "Unknown monitored item" << monId << "for subscription" << subId << "deleted!";
        }
    } else {
        qWarning() << "Invalid monitored item" << monId << "for subscription" << subId << "deleted!";
    }
}
