#include "baumercameraplugin.h"

#include <QtCore>
#include <QtGui>

#include <opencv2/core.hpp>

#include "baumercamera.h"
#include "baumercamerawidget.h"


Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(QWeakPointer<BaumerCamera>)


typedef enum
{
	ePvErrSuccess = 0,        // No error
	ePvErrCameraFault = 1,        // Unexpected camera fault
	ePvErrInternalFault = 2,        // Unexpected fault in PvApi or driver
	ePvErrBadHandle = 3,        // Camera handle is invalid
	ePvErrBadParameter = 4,        // Bad parameter to API call
	ePvErrBadSequence = 5,        // Sequence of API calls is incorrect
	ePvErrNotFound = 6,        // Camera or attribute not found
	ePvErrAccessDenied = 7,        // Camera cannot be opened in the specified mode
	ePvErrUnplugged = 8,        // Camera was unplugged
	ePvErrInvalidSetup = 9,        // Setup is invalid (an attribute is invalid)
	ePvErrResources = 10,       // System/network resources or memory not available
	ePvErrBandwidth = 11,       // 1394 bandwidth not available
	ePvErrQueueFull = 12,       // Too many frames on queue
	ePvErrBufferTooSmall = 13,       // Frame buffer is too small
	ePvErrCancelled = 14,       // Frame cancelled by user
	ePvErrDataLost = 15,       // The data for the frame was lost
	ePvErrDataMissing = 16,       // Some data in the frame is missing
	ePvErrTimeout = 17,       // Timeout during wait
	ePvErrOutOfRange = 18,       // Attribute value is out of the expected range
	ePvErrWrongType = 19,       // Attribute is not this type (wrong access function) 
	ePvErrForbidden = 20,       // Attribute write forbidden at this time
	ePvErrUnavailable = 21,       // Attribute is not available at this time
	ePvErrFirewall = 22,       // A firewall is blocking the traffic (Windows only)
	__ePvErr_force_32 = 0xFFFFFFFF

} tPvErr;


void BGAPI2CALL PnPEventHandler(void * callbackOwner, BGAPI2::Events::PnPEvent * pPnPEvent);


BaumerCameraPlugin::BaumerCameraPlugin(QObject *parent) :
        Plugin(parent)
{
	_systemList = nullptr;

    setupUi();

    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<QWeakPointer<BaumerCamera> >("QWeakPointer<BaumerCamera>");
}

BaumerCameraPlugin::~BaumerCameraPlugin()
{

}

void BaumerCameraPlugin::initialize()
{
	try
	{
		_systemList = BGAPI2::SystemList::GetInstance();
		_systemList->Refresh();

		for (BGAPI2::SystemList::iterator sysIterator = _systemList->begin(); sysIterator != _systemList->end(); sysIterator++) {
			BGAPI2::System *system = sysIterator->second;

			qDebug() << "System Type:" << system->GetTLType();
			qDebug() << "System Filename:" << system->GetFileName();
			qDebug() << "System Version:" << system->GetVersion();
			qDebug() << "System Path:" << system->GetPathName();

			system->Open();
			BGAPI2::InterfaceList *interfaceList = system->GetInterfaces();
			interfaceList->Refresh(100);
			for (BGAPI2::InterfaceList::iterator ifIterator = interfaceList->begin(); ifIterator != interfaceList->end(); ifIterator++) {
				BGAPI2::Interface *bgapi2_interface = ifIterator->second;

				qDebug() << "Interface:" << bgapi2_interface->GetDisplayName();

				bgapi2_interface->Open();
				bgapi2_interface->RegisterPnPEvent(BGAPI2::Events::EVENTMODE_EVENT_HANDLER);
				bgapi2_interface->RegisterPnPEventHandler(this, (BGAPI2::Events::PnPEventHandler) &PnPEventHandler);

				BGAPI2::DeviceList *deviceList = bgapi2_interface->GetDevices();
				deviceList->Refresh(100);
				for (BGAPI2::DeviceList::iterator devIterator = deviceList->begin(); devIterator != deviceList->end(); devIterator++) {
					BGAPI2::Device *device = devIterator->second;
					plug(device->GetSerialNumber().get());
				}
			}
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}
}

void BaumerCameraPlugin::uninitialize()
{
	// unplug all cameras
	for (const QString &cameraId : _threads.keys()) {
		unplug(cameraId);
	}

	try
	{
		if (_systemList) {
			for (BGAPI2::SystemList::iterator sysIterator = _systemList->begin(); sysIterator != _systemList->end(); sysIterator++) {
				BGAPI2::System *system = sysIterator->second;
				BGAPI2::InterfaceList *interfaceList = system->GetInterfaces();
				for (BGAPI2::InterfaceList::iterator ifIterator = interfaceList->begin(); ifIterator != interfaceList->end(); ifIterator++) {
					BGAPI2::Interface *bgapi2_interface = ifIterator->second;
					bgapi2_interface->UnregisterPnPEvent();
					bgapi2_interface->Close();
				}
				system->Close();
			}

			_systemList = nullptr;
			BGAPI2::SystemList::ReleaseInstance();
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}
}

void BaumerCameraPlugin::setupUi()
{
    _widget = new BaumerCameraWidget();
    connect(this, &BaumerCameraPlugin::pluggedCamera, _widget, &BaumerCameraWidget::registerCamera);
    connect(this, &BaumerCameraPlugin::unpluggedCamera, _widget, &BaumerCameraWidget::unregisterCamera);
}

void BaumerCameraPlugin::plug(QString cameraId)
{
	qDebug() << "Plug" << cameraId;

    QMutexLocker locker(&_mutex);
    Q_UNUSED(locker);
	
	try
	{
		for (BGAPI2::SystemList::iterator sysIterator = _systemList->begin(); sysIterator != _systemList->end(); sysIterator++) {
			BGAPI2::System *system = sysIterator->second;

			BGAPI2::InterfaceList *interfaceList = system->GetInterfaces();
			for (BGAPI2::InterfaceList::iterator ifIterator = interfaceList->begin(); ifIterator != interfaceList->end(); ifIterator++) {
				BGAPI2::Interface *bgapi2_interface = ifIterator->second;

				BGAPI2::DeviceList *deviceList = bgapi2_interface->GetDevices();
				deviceList->Refresh(100);
				for (BGAPI2::DeviceList::iterator devIterator = deviceList->begin(); devIterator != deviceList->end(); devIterator++) {
					BGAPI2::Device *device = devIterator->second;

					if (cameraId == device->GetSerialNumber().get()) {
						qDebug() << "Camera Name:" << device->GetDisplayName();
						qDebug() << "Camera ID:" << device->GetID();
						qDebug() << "Camera Serial Number:" << device->GetSerialNumber();
						
						// connect to camera
						device->Open();

						// prepare thread for camera
						QSharedPointer<BaumerCamera> camera = QSharedPointer<BaumerCamera>(new BaumerCamera(device));

						// create thread for camera
						QThread *thread = new QThread(camera.data());
						thread->start();
						thread->setObjectName(QString("Camera %1").arg(cameraId));
						thread->setPriority(QThread::IdlePriority);

						// attach plugin to thread
						camera->moveToThread(thread);
						QMetaObject::invokeMethod(camera.data(), "initialize", Qt::BlockingQueuedConnection);
						_threads.insert(cameraId, camera);

						connect(camera.data(), &BaumerCamera::newImage, this, &BaumerCameraPlugin::newImage, Qt::DirectConnection);
						emit pluggedCamera(camera.toWeakRef());

                        // notify that camera is available
                        emit valueChanged(cameraId + "/StatusCamera", ePvErrSuccess);
                        emit valueChanged(cameraId + "/CameraName", camera->getCameraTitle());

						break;
					}
				}
			}
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}
}

void BaumerCameraPlugin::unplug(QString cameraId)
{
	qDebug() << "Unplug" << cameraId;

    QMutexLocker locker(&_mutex);
    Q_UNUSED(locker);

    if (_threads.contains(cameraId)) {
        // get camera
        QSharedPointer<BaumerCamera> camera = _threads[cameraId];

        // notify that the camera will be unplugged
        emit unpluggedCamera(camera.toWeakRef());

        QMetaObject::invokeMethod(camera.data(), "unplug", Qt::BlockingQueuedConnection);

        // quit thread for camera and remove
        camera->thread()->quit();
        camera->thread()->wait();
        _threads.remove(cameraId);

        // notify that the camera was unplugged
        emit valueChanged(cameraId + "/StatusCamera", ePvErrUnplugged);

        qDebug() << "Camera" << cameraId << "unplugged";
    }
}

void BaumerCameraPlugin::setValue(const QString &name, const QVariant &value)
{
    QStringList parts = name.split("/", QString::SkipEmptyParts);
    if (parts.size() > 1) {
        QString cameraId = parts.at(0);

        if (_threads.contains(cameraId)) {
            parts.removeFirst();
            QString parameter = parts.join("/");
            QSharedPointer<BaumerCamera> camera = _threads[cameraId];
            if (camera)
                camera->setValue(parameter, value);
        }
    }
}

void BaumerCameraPlugin::setValues(const QHash<QString, QVariant> &values)
{
    defaultSetValues(values);
}

/*
 * Callbacks
 */

void BGAPI2CALL PnPEventHandler(void * callbackOwner, BGAPI2::Events::PnPEvent * pPnPEvent)
{
	BaumerCameraPlugin *callbackHandler = (BaumerCameraPlugin *)callbackOwner;

	switch (pPnPEvent->GetPnPType()) {
	case BGAPI2::Events::PNPTYPE_DEVICEADDED:
		if (callbackHandler)
			callbackHandler->plug(pPnPEvent->GetSerialNumber().get());
		break;
	case BGAPI2::Events::PNPTYPE_DEVICEREMOVED:
		if (callbackHandler)
			callbackHandler->unplug(pPnPEvent->GetSerialNumber().get());
		break;
	default:
		qWarning() << "Unknown event" << pPnPEvent->GetPnPType() << "!";
		break;
	}
}
