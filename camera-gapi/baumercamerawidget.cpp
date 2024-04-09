#include "baumercamerawidget.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "baumercameraview.h"
#include "baumercamera.h"

BaumerCameraWidget::BaumerCameraWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void BaumerCameraWidget::setupUi()
{
    // prepare layout
    setLayout(new QHBoxLayout());
	layout()->setContentsMargins(0, 0, 0, 0);

    _stack = new QStackedWidget();
    layout()->addWidget(_stack);

    _tab_widget = new ThirdLevelNavigationWidget();

    _label = new QLabel(tr("No Cameras found!"));
    _label->setAlignment(Qt::AlignCenter);

    _stack->addWidget(_label);
    _stack->addWidget(_tab_widget);

    QMetaObject::connectSlotsByName(this);
}

void BaumerCameraWidget::setValue(const QString &name, const QVariant &value)
{
    Q_UNUSED(value);

    // set correct title of tab
    if (name == "CameraName" || name == "DeviceControl/DeviceUserID") {
        for (int i=0; i < _tab_widget->count(); i++) {
            QWidget *widget = _tab_widget->widget(i);
			BaumerCameraView *view = qobject_cast<BaumerCameraView *>(widget);
            if (view) {
                _tab_widget->setTabText(i, view->title());
            }
        }
    }
}

void BaumerCameraWidget::registerCamera(QWeakPointer<BaumerCamera> camera_pointer)
{
    QSharedPointer<BaumerCamera> camera = camera_pointer.toStrongRef();

    if (camera) {
		// create new view for camera
		BaumerCameraView *camera_view = new BaumerCameraView(camera_pointer);
        _tab_widget->addTab(camera_view, camera.data()->getCameraTitle());

		// make sure tab widget is visible
		if (_stack->currentWidget() != _tab_widget)
			_stack->setCurrentWidget(_tab_widget);

		// monitor changes of camere names
        connect(camera.data(), &BaumerCamera::valueChanged, this, &BaumerCameraWidget::setValue);
	}
}

void BaumerCameraWidget::unregisterCamera(QWeakPointer<BaumerCamera> camera_pointer)
{
	for (int i = 0; i < _tab_widget->count(); ++i) {
		QWidget *view = _tab_widget->widget(i);
		BaumerCameraView *camera_view = qobject_cast<BaumerCameraView *>(view);
		if (camera_view && camera_view->camera() == camera_pointer) {
			_tab_widget->removeTab(i);
			camera_view->deleteLater();
			break;
		}
	}
	
	if (_tab_widget->count() == 0 || !qobject_cast<BaumerCameraView *>(_tab_widget->widget(0))) {
		_stack->setCurrentWidget(_label);
	}
}