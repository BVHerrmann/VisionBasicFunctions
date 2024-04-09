#include "audittrailplugin.h"

#include <audittrail.h>


AuditTrailPlugin::AuditTrailPlugin(QObject *parent) :
    Plugin(parent)
{
    // initialize model
    _db = new AuditTrailDatabase();

    // initialize views
    _widget = new AuditTrailWidget(_db->tableModel());
	_optionPanel = new AuditTrailOptionPanel(_db);
    
    loadPreferences();
    
    // install custom message handler and start logging
    qApp->installEventFilter(this);
    for (QWidget *widget : QApplication::allWidgets()) {
        registerWidget(widget);
    }
}

AuditTrailPlugin::~AuditTrailPlugin()
{
    
}

void AuditTrailPlugin::initialize()
{
    qDebug() << "Audit Trail Database Initialized";
}

void AuditTrailPlugin::uninitialize()
{
    // restore previouse message handler
    qApp->removeEventFilter(this);
}

void AuditTrailPlugin::loadPreferences()
{

}

void AuditTrailPlugin::log(const QString &msg)
{
    assert(QThread::currentThread() == thread());
    
    // log message
    QMetaObject::invokeMethod(_db, "log", Qt::QueuedConnection,
                              Q_ARG(QDateTime, QDateTime::currentDateTime()),
                              Q_ARG(QString, currentUsername()),
                              Q_ARG(QString, msg)
                              );
}

QVariant AuditTrailPlugin::getCurrentValue(QWidget *widget)
{
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(widget)) {
        if (button->isCheckable()) {
            return button->isChecked();
        }
    } else if (QAbstractSlider *slider = qobject_cast<QAbstractSlider *>(widget)) {
        return slider->value();
    } else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget)) {
        return comboBox->currentText();
    } else if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox *>(widget)) {
        return spinBox->value();
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        return lineEdit->text();
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox *>(widget)) {
        return spinBox->value();
    } else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget)) {
        return textEdit->toPlainText();
    }
    
    return QVariant();
}

void AuditTrailPlugin::logChange(QWidget *widget)
{
    QVariant property = widget->property(kAuditTrail);
    if (property.isValid()) {
        QString name = property.toString();
        QVariant previousValue = _storedValues.value(widget);
        QVariant currentValue = storeValue(widget, true);
        if (!currentValue.isValid() && !previousValue.isValid()) {
            log(tr("%1").arg(name));
        } else if (currentValue != previousValue) {
            if (previousValue.isValid()) {
                log(tr("%1 changed from %2 to %3").arg(name).arg(previousValue.toString()).arg(currentValue.toString()));
            } else {
                log(tr("%1 changed to %2").arg(name).arg(currentValue.toString()));
            }
        }
    }
}

void AuditTrailPlugin::registerWidget(QWidget *widget)
{
    if (!_registeredWidgets.contains(widget)) {
        _registeredWidgets.append(widget);
        
        QVariant property = widget->property(kAuditTrail);
        if (property.isValid()) {
            QString propertyName = property.toString();
            
            if (QAbstractButton *button = qobject_cast<QAbstractButton *>(widget)) {
                connect(button, &QAbstractButton::clicked, this, [=](){
                    logChange(button);
                });
            } else if (QAbstractSlider *slider = qobject_cast<QAbstractSlider *>(widget)) {
                connect(slider, &QAbstractSlider::sliderMoved, this, [=](const int value) {
                    QTimer::singleShot(2500, [=]() { logChange(slider); });
                });
            } else if (QAbstractSpinBox *spinBox = qobject_cast<QAbstractSpinBox *>(widget)) {
                connect(spinBox, &QAbstractSpinBox::editingFinished, this, [=]() {
                    logChange(spinBox);
                });
            } else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget)) {
                connect(comboBox, &QComboBox::currentTextChanged, this, [=](const QString &string) {
                    logChange(comboBox);
                });
            } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
                connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
                    logChange(lineEdit);
                });
            } else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget)) {
                connect(textEdit, &QTextEdit::textChanged, this, [=](){
                    logChange(textEdit);
                });
            } else {
                qWarning() << "Unknown Object for Audit Trail:" << widget;
            }
        }
    }
}

void AuditTrailPlugin::unregisterWidget(QWidget *widget)
{
    if (!_registeredWidgets.removeOne(widget)) {
        qWarning() << "Widget" << widget << "not registered!";
    }
}

QVariant AuditTrailPlugin::storeValue(QWidget *widget, bool force)
{
    if (force || !_storedValues.contains(widget)) {
        _storedValues[widget] = getCurrentValue(widget);
    }
    
    return _storedValues[widget];
}

bool AuditTrailPlugin::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == AuditTrailEventType) {
        AuditTrailEvent *auditTrailEvent = static_cast<AuditTrailEvent *>(event);
        log(auditTrailEvent->message());
        
    } else if (obj->isWidgetType()
               && obj->property(kAuditTrail).isValid()) {
        QWidget *widget = qobject_cast<QWidget *>(obj);
        switch (event->type()) {
            case QEvent::ShowToParent:
                registerWidget(widget);
                break;
            case QEvent::Destroy:
                unregisterWidget(widget);
                break;
            case QEvent::Enter:
            case QEvent::FocusIn:
                storeValue(widget);
                break;
            default:
                break;
        }
    }
    
    return false;
}
