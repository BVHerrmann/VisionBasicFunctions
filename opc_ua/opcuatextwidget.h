#ifndef OPCUATEXTWIDGET_H
#define OPCUATEXTWIDGET_H

#include "opcuastatewidget.h"


class OPCUATextWidget : public OPCUAStateWidget
{
public:
    OPCUATextWidget(const std::shared_ptr<OPCValue> &config, QWidget *parent = nullptr);

public slots:

protected:
    QWidget * contentWidget() override;

    void setValue(const QVariant &value) override;

    void setState(bool valid) override;
    void setForced(bool forced) override;

    QLabel *_label;
};

#endif // OPCUATEXTWIDGET_H
