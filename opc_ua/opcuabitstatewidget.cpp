#include "opcuabitstatewidget.h"


OPCUABitStateWidget::OPCUABitStateWidget(const std::shared_ptr<OPCValue> &config, QWidget *parent) :
    OPCUAStateWidget(config, parent)
{
    _state_button = new BSwitch(this);
    _state_button->setCheckable(true);
    connect(_state_button, &BSwitch::clicked, this, &OPCUABitStateWidget::setOutputState);
    
    setupUi();
}

QWidget * OPCUABitStateWidget::contentWidget()
{
    return _state_button;
}

void OPCUABitStateWidget::setOutputState()
{
    changeValueTo(_state_button->isChecked());
}

void OPCUABitStateWidget::setValue(const QVariant &value)
{
    if (value.toBool())
        _state_button->setChecked(true);
    else
        _state_button->setChecked(false);
}

void OPCUABitStateWidget::setState(bool valid)
{
    _state_button->setEnabled(valid);
}

void OPCUABitStateWidget::setForced(bool forced)
{
    _state_button->setCheckedColor(forced ? HMIColor::WarningHigh : HMIColor::OK);
    _state_button->setUncheckedColor(forced ? HMIColor::WarningLow : HMIColor::Light);
}
