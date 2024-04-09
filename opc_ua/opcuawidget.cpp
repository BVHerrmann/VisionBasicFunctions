#include "opcuawidget.h"

#include "opcuaplugin.h"
#include "opcuastatewidget.h"
#include "opcuabitstatewidget.h"
#include "opcuatextwidget.h"


OPCUAWidget::OPCUAWidget(OPCUAPlugin *plugin, QWidget *parent) :
    QWidget(parent)
{
    _plugin = plugin;

    setupUi();

    _update_timer = new QTimer(this);
    _update_timer->start(100);
}

OPCUAWidget::~OPCUAWidget()
{
    _update_timer->stop();
}

void OPCUAWidget::setupUi()
{
    QBoxLayout *layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);

    QGroupBox *in_box = new QGroupBox(tr("Input"));
    QPalette pal_in = in_box->palette();
    pal_in.setColor(QPalette::Window, Qt::transparent);
    in_box->setPalette(pal_in);
    layout->addWidget(in_box);

    QScrollArea *in_area = new QScrollArea();
    in_area->setWidgetResizable(true);
    in_area->setFrameStyle(QFrame::Plain);
    in_box->setLayout(new QVBoxLayout());
    in_box->layout()->setContentsMargins(0, 0, 0, 0);
    in_box->layout()->addWidget(in_area);

    QWidget *in_widget = new QWidget();
    in_area->setWidget(in_widget);

    _in_layout = new QFormLayout();
    _in_layout->setLabelAlignment(Qt::AlignVCenter);
    _in_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    _in_layout->setVerticalSpacing(0);
    in_widget->setLayout(_in_layout);


    QGroupBox *out_box = new QGroupBox(tr("Output"));
    QPalette pal_out = out_box->palette();
    pal_out.setColor(QPalette::Window, Qt::transparent);
    out_box->setPalette(pal_out);
    layout->addWidget(out_box);

    QScrollArea *out_area = new QScrollArea();
    out_area->setWidgetResizable(true);
    out_area->setFrameStyle(QFrame::Plain);
    out_box->setLayout(new QVBoxLayout());
    out_box->layout()->setContentsMargins(0, 0, 0, 0);
    out_box->layout()->addWidget(out_area);

    QWidget *out_widget = new QWidget();
    out_area->setWidget(out_widget);

    _out_layout = new QFormLayout();
    _out_layout->setLabelAlignment(Qt::AlignVCenter);
    _out_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    _out_layout->setVerticalSpacing(0);
    out_widget->setLayout(_out_layout);
}

void OPCUAWidget::setupWidget(OPCUAInterface *opcua_interface)
{
    for (int i = _in_layout->rowCount() - 1; i >= 0; i--) {
        _in_layout->removeRow(i);
    }

    QString last_in_ns = QString();
    for (const std::shared_ptr<OPCInputValue> &config : opcua_interface->getInConfig()) {
        if (config->nsUri() != last_in_ns) {
            if (!last_in_ns.isEmpty()) {
                QFrame *line = new QFrame();
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->setFixedHeight(20);
                line->setContentsMargins(0, 0, 0, 0);
                _in_layout->addRow(line);
            }
            last_in_ns = config->nsUri();
        }

        OPCUAStateWidget *widget = newWidgetForConfig(config);
        _in_layout->addRow(config->name(), widget);
    }

    for (int i = _out_layout->rowCount() - 1; i >= 0; i--) {
        _out_layout->removeRow(i);
    }

    QString last_out_ns = QString();
    for (const std::shared_ptr<OPCOutputValue> &config : opcua_interface->getOutConfig()) {
        if (config->nsUri() != last_out_ns) {
            if (!last_out_ns.isEmpty()) {
                QFrame *line = new QFrame();
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->setFixedHeight(20);
                line->setContentsMargins(0, 0, 0, 0);
                _out_layout->addRow(line);
            }
            last_out_ns = config->nsUri();
        }

        OPCUAStateWidget *widget = newWidgetForConfig(config);
        _out_layout->addRow(config->name(), widget);
    }
}

OPCUAStateWidget * OPCUAWidget::newWidgetForConfig(const std::shared_ptr<OPCValue> &config)
{
    OPCUAStateWidget *widget = 0;
    switch(config->datatype()) {
    case QVariant::Bool:
        widget = new OPCUABitStateWidget(config);
        break;
    default:
        widget = new OPCUATextWidget(config);
        break;
    }
    
    connect(_update_timer, &QTimer::timeout, widget, &OPCUAStateWidget::updateUi);

    return widget;
}
