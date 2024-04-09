#include "opcuatextwidget.h"

#include <colors.h>


OPCUATextWidget::OPCUATextWidget(const std::shared_ptr<OPCValue> &config, QWidget *parent) :
    OPCUAStateWidget(config, parent)
{
    _label = new QLabel();

    setupUi();
}

QWidget * OPCUATextWidget::contentWidget()
{
    return _label;
}

void OPCUATextWidget::setValue(const QVariant &value)
{
    _label->setText(value.toString());
}

void OPCUATextWidget::setState(bool valid)
{
    _label->setEnabled(valid);
}

void OPCUATextWidget::setForced(bool forced)
{
    QPalette pal(_label->palette());

    if (forced) {
        pal.setColor(QPalette::Text, HMIColor::WarningHigh);
    } else {
        pal.setColor(QPalette::Text, HMIColor::DarkGrey);
    }

    _label->setPalette(pal);
}
