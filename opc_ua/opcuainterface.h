#ifndef OPCUAINTERFACE_H
#define OPCUAINTERFACE_H

#include <QtCore>

#include <open62541/client.h>

#ifndef Q_MOC_RUN
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#endif
using namespace boost::accumulators;

#include <interfaces.h>
#include <iointerface.h>
#include <logic.h>
#include <opcvalue.h>

#ifdef WIN32
    #define DEBUG_LOOP  0
#else
    #define DEBUG_LOOP  1
#endif


class OPCUAInterface : public IOInterface
{
    Q_OBJECT
public:
    explicit OPCUAInterface(const QHash<QString, QVariant> &config, const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config);

    const PluginInterface::MachineState machineState() const { return _machine_state; };
    const UA_StatusCode state() const { return _state; };
    
    virtual void updateConfig(const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config);
    virtual void updateConfig(const std::vector<std::shared_ptr<OPCInputValue>> &in_config, const std::vector<std::shared_ptr<OPCOutputValue>> &out_config, const std::vector<std::shared_ptr<OPCMethod>> &method_config);
    std::vector<std::shared_ptr<OPCInputValue>> getInConfig() { return _in_config; }
    std::vector<std::shared_ptr<OPCOutputValue>> getOutConfig() { return _out_config; }
    
    void setLogic(std::shared_ptr<Logic> logic);
    void writeOutputs() override;

signals:
    // CommunicationInterface
    void valueChanged(const QString &name, const QVariant &value);

public slots:
    // triggers async fetching of data. called by timer.
    void readData();
    
	virtual void connectOPCUA() = 0;
    virtual void disconnectOPCUA() = 0;

    void printStatistics();

protected:
    // state
    PluginInterface::MachineState _machine_state;
    
    std::weak_ptr<Logic> _logic;
    
    // config
    std::vector<std::shared_ptr<OPCInputValue>> _in_config;
    std::vector<std::shared_ptr<OPCOutputValue>> _out_config;
    
    // timer for cyclic reading and writing of data
    QMutex _read_mutex;
    
	void startTimer();
	void stopTimer();

    void updateState(const UA_StatusCode state);
    void updateNodeStates(const UA_StatusCode state);
    
    // convert raw data to name/value pairs
    void processData(OPCInputValue *identifier, const UA_DataValue *value);
    
    // triggers async reading and writing of data to the controller.
    virtual void readDataInternal() = 0;
    virtual std::map<std::shared_ptr<OPCValue>, UA_StatusCode> writeData(const std::map<std::shared_ptr<OPCValue>, UA_Variant> &data) = 0;

private:
    UA_StatusCode _state;
    
    QMutex _timerMutex;
	QTimer *_read_timer;

    std::mutex _timing_mutex;
    QElapsedTimer _timer;
    accumulator_set<int64_t, features<tag::min, tag::max, tag::mean > > _read_performance;
    accumulator_set<int64_t, features<tag::min, tag::max, tag::mean > > _read_delay;
    accumulator_set<int64_t, features<tag::min, tag::max, tag::mean > > _read_jitter;
};

#endif // OPCUAINTERFACE_H
