#include "opcuainterface.h"

template<class> inline constexpr bool always_false_v = false;


OPCUAInterface::OPCUAInterface(const QHash<QString, QVariant> &config, const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config) :
    IOInterface()
{
    _machine_state = PluginInterface::Initializing;
    _state = UA_STATUSCODE_GOOD;
    
    updateConfig(in_config, out_config);

    // invalidate performance timers
    _timer.invalidate();
    
    // initialize timer
    _read_timer = new QTimer(this);
    _read_timer->setInterval(250);
    connect(_read_timer, &QTimer::timeout, this, &OPCUAInterface::readData);
}

void OPCUAInterface::updateState(const UA_StatusCode state)
{
    if (_state != state) {
        _state = state;
    }
}

void OPCUAInterface::updateNodeStates(const UA_StatusCode state)
{
    for (auto &config : _in_config) {
        config->set_valid(state == UA_STATUSCODE_GOOD);
    }
    for (auto &config : _out_config) {
        config->set_valid(state == UA_STATUSCODE_GOOD);
    }
}

void OPCUAInterface::updateConfig(const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config)
{
    _in_config = in_config;
    _out_config = out_config;
}

void OPCUAInterface::updateConfig(const std::vector<std::shared_ptr<OPCInputValue>>& in_config, const std::vector<std::shared_ptr<OPCOutputValue>>& out_config, const std::vector<std::shared_ptr<OPCMethod>>& method_config)
{
    updateConfig(in_config, out_config);
}

void OPCUAInterface::startTimer()
{
    // check if timer already started
    if (!_timerMutex.tryLock()) {
        return;
    }

    // this is very time consuming. reserve one thread from global pool.
    QThreadPool::globalInstance()->reserveThread();
    
	_read_timer->start();
}

void OPCUAInterface::stopTimer()
{
    if (!_timerMutex.tryLock()) {   // make sure timer was already locked
        // release one thread
        QThreadPool::globalInstance()->releaseThread();
    }
    
    if (_read_timer->isActive()) {
        _read_timer->stop();
    }

    // unlock timer mutex
    _timerMutex.unlock();
}

void OPCUAInterface::readData()
{
#ifdef DEBUG
#if defined(Q_OS_WIN)
	DWORD prio = GetThreadPriority(GetCurrentThread());
	if (prio != 15)
		qDebug() << "Wrong Thread Priority!" << prio;
#endif
#endif

    int64_t read_delay = _timer.isValid() ? _timer.nsecsElapsed() : 0;
    _timer.restart();

    readDataInternal();

    int64_t read_duration = _timer.nsecsElapsed();
    
    std::lock_guard<std::mutex> guard(_timing_mutex);

    _read_performance(read_duration);
    if (read_delay > 0) {
        _read_delay(read_delay);
        _read_jitter(read_delay - (1 * 250000000));
    }
}

void OPCUAInterface::printStatistics()
{
    _timing_mutex.lock();
    
    int64_t min_delay = extract_result< tag::min >(_read_delay);
    int64_t mean_delay = mean(_read_delay);
    int64_t max_delay = extract_result< tag::max >(_read_delay);
    
    int64_t min_jitter = extract_result< tag::min >(_read_jitter);
    int64_t mean_jitter = mean(_read_jitter);
    int64_t max_jitter = extract_result< tag::max >(_read_jitter);
    
    int64_t min_performance = extract_result< tag::min >(_read_performance);
    int64_t mean_performance = mean(_read_performance);
    int64_t max_performance = extract_result< tag::max >(_read_performance);
    
    _timing_mutex.unlock();
    
    if (min_delay != INT64_MAX) {
        qDebug() << "read delay:" << min_delay / 1000000.0 << mean_delay / 1000000.0 << max_delay / 1000000.0;
        qDebug() << "read jitter:" << min_jitter / 1000000.0 << mean_jitter / 1000000.0 << max_jitter / 1000000.0;
        if (mean(_read_performance) > mean(_read_delay))
            qWarning() << "read performance:" << min_performance / 1000000.0 << mean_performance / 1000000.0 << max_performance / 1000000.0;
        else
            qDebug() << "read performance:" << min_performance / 1000000.0 << mean_performance / 1000000.0 << max_performance / 1000000.0;
    }
}

void OPCUAInterface::processData(OPCInputValue *identifier, const UA_DataValue *value)
{
    if (value) {
        std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string> v;
        bool valid = false;
        
        if (value->hasValue) {
            switch (value->value.type->typeIndex) {
            case UA_TYPES_BOOLEAN:
                v = *(UA_Boolean *)value->value.data;
                break;
            case UA_TYPES_SBYTE:
                v = *(UA_SByte *)value->value.data;
                break;
            case UA_TYPES_BYTE:
                v = *(UA_Byte *)value->value.data;
                break;
            case UA_TYPES_INT16:
                v = *(UA_Int16 *)value->value.data;
                break;
            case UA_TYPES_UINT16:
                v = *(UA_UInt16 *)value->value.data;
                break;
            case UA_TYPES_INT32:
                v = *(UA_Int32 *)value->value.data;
                break;
            case UA_TYPES_UINT32:
                v = *(UA_UInt32 *)value->value.data;
                break;
            case UA_TYPES_INT64:
                v = *(UA_Int64 *)value->value.data;
                break;
            case UA_TYPES_UINT64:
                v = *(UA_UInt64 *)value->value.data;
                break;
            case UA_TYPES_FLOAT:
                v = *(UA_Float *)value->value.data;
                break;
            case UA_TYPES_DOUBLE:
                v = *(UA_Double *)value->value.data;
                break;
            case UA_TYPES_STRING: {
                auto uaString = *(UA_String *)value->value.data;
                v = std::string((char *)uaString.data, uaString.length);
                break;
            }
                default:
                    qWarning() << "Unhandeled data type" << value->value.type->typeIndex << "for" << identifier->nodeId();
            }
        }
        
        if (value->hasStatus) {
            valid = (value->status == UA_STATUSCODE_GOOD);
        } else {
            valid = true;
        }
        
        identifier->update(v, valid);
    } else {
        identifier->set_valid(false);
        qWarning() << "No value!";
    }
}

void OPCUAInterface::writeOutputs()
{
    std::map<std::shared_ptr<OPCValue>, UA_Variant> data;
    
    // parse buffer according to config
    for (const std::shared_ptr<OPCOutputValue> &config : _out_config) {
        auto value = config->update();
        
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
            
            data[config] = var;
        }
    }
    
    // check if there is actually new data
	if (data.size()) {
		// write data
		std::map<std::shared_ptr<OPCValue>, UA_StatusCode> state = writeData(data);
        
        // update variable states
        for (auto& [config, value]: state) {
            config->set_valid(value == UA_STATUSCODE_GOOD);
        }
    }
}

void OPCUAInterface::setLogic(std::shared_ptr<Logic> logic)
{
    _logic = logic;
    moveToThread(logic->thread());
}
