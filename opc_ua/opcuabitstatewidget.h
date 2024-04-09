#ifndef OPCUABITSTATEWIDGET_H
#define OPCUABITSTATEWIDGET_H

#include "opcuastatewidget.h"

#include <bswitch.h>


class OPCUABitStateWidget : public OPCUAStateWidget
{
    Q_OBJECT
public:
    explicit OPCUABitStateWidget(const std::shared_ptr<OPCValue> &config, QWidget *parent = nullptr);

signals:

public slots:
    void setOutputState();

protected:
    QWidget * contentWidget() override;

    void setValue(const QVariant &value) override;

    void setState(bool valid) override;
    void setForced(bool forced) override;

    BSwitch *_state_button;
};

#endif // OPCUABITSTATEWIDGET_H
