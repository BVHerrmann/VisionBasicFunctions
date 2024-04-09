#ifndef OPCUASTATEWIDGET_H
#define OPCUASTATEWIDGET_H

#include <QtCore>
#include <QtWidgets>

#include "opcuainterface.h"


class OPCUAStateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OPCUAStateWidget(const std::shared_ptr<OPCValue> &config, QWidget *parent = nullptr);

signals:
    // CommunicationInterface
    void valueChanged(const QString &name, const QVariant &value);

public slots:
    void updateUi();

protected:
    void setupUi();

    virtual QWidget * contentWidget() = 0;

    virtual void setValue(const QVariant &value) = 0;
    void changeValueTo(const QVariant &value);

    virtual void setState(bool valid) = 0;
    virtual void setForced(bool forced) = 0;
    
    bool _valid = false;
    bool _forced = false;
    QVariant _value;
    
    std::shared_ptr<OPCValue> _config;
};

#endif // OPCUASTATEWIDGET_H
