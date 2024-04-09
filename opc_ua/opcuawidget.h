#ifndef OPCUAWIDGET_H
#define OPCUAWIDGET_H

#include <QtCore>
#include <QtWidgets>

#include "opcuainterface.h"
class OPCUAPlugin;
class OPCUAStateWidget;


class OPCUAWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OPCUAWidget(OPCUAPlugin *plugin, QWidget *parent = 0);
    ~OPCUAWidget();

signals:

public slots:
    void setupWidget(OPCUAInterface *opcua_interface);

private:
    void setupUi();

    OPCUAStateWidget * newWidgetForConfig(const std::shared_ptr<OPCValue> &config);

    QFormLayout *_in_layout;
    QFormLayout *_out_layout;

    OPCUAPlugin *_plugin;

    QTimer *_update_timer;
};

#endif // OPCUAWIDGET_H
