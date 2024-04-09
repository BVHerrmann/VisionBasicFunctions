#include "baumercamera.h"

#include <QtCore>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>


// CallBacks
void BGAPI2CALL DevEventHandler(void * callbackOwner, BGAPI2::Events::DeviceEvent * pDevEvent);
void BGAPI2CALL BufferHandler(void * callBackOwner, BGAPI2::Buffer * pBufferFilled);


BaumerCamera::BaumerCamera(BGAPI2::Device *device, QObject *parent) :
        QObject(parent)
{
    // remember camera
	_device = device;
	_dataStream = nullptr;
	_bufferList = nullptr;

    // load all settings from preferences
    loadSettings();

    // setup camera
    setupCamera();
}

BaumerCamera::~BaumerCamera()
{
	if (_dataStream) {
		_dataStream->Close();
		_dataStream = nullptr;
	}

	if (_device) {
		_device->Close();
		_device = nullptr;
	}
}

QVariant BaumerCamera::getValue(QString name)
{
	try {
		BGAPI2::Node *node = getNode(name);
		return QVariant(node->GetValue().get());
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in " << __FUNCTION__ << "function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return QVariant();
}

QVariant BaumerCamera::getMinimumForValue(QString name)
{
    return getRangeForValue(name, false);
}

QVariant BaumerCamera::getMaximumForValue(QString name)
{
    return getRangeForValue(name, true);
}

QVariant BaumerCamera::getRangeForValue(QString name, bool maximum)
{
	QVariant minimum_value;
	QVariant maximum_value;

	try {
		BGAPI2::Node *node = getNode(name);
		
		if (node->GetInterface() == BGAPI2_NODEINTERFACE_INTEGER) {
			minimum_value = node->GetIntMin();
			maximum_value = node->GetIntMax();
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_FLOAT) {
			minimum_value = node->GetDoubleMin();
			maximum_value = node->GetDoubleMax();
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_ENUMERATION) {
			QStringList options;
			BGAPI2::NodeMap *enumList = node->GetEnumNodeList();
			for (bo_uint64 i = 0; i < enumList->GetNodeCount(); i++) {
				BGAPI2::Node *n = enumList->GetNodeByIndex(i);
				if (n->GetImplemented() && n->GetAvailable()) {
					options.append(n->GetName().get());
				}
			}
			minimum_value = options;
			maximum_value = options;
		}
		else {
			qWarning() << "Unknow data type" << node->GetInterface() << "for" << name << "in" << __FUNCTION__;
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in " << __FUNCTION__ << "function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return maximum ? maximum_value : minimum_value;
}

bool BaumerCamera::setValue(const QString &name, const QVariant &value)
{
    QStringList attributeParts = name.split("/");
    if (name == "StartCapturing") {
        startCapturing();
    } else if (name == "StopCapturing") {
        stopCapturing();
    } else if (attributeParts.length() == 1 || attributeParts.length() == 2) {
        try {
            BGAPI2::Node *node = getNode(name);

            if (node->GetInterface() == BGAPI2_NODEINTERFACE_INTEGER) {
                node->SetInt(value.toInt());
                return true;
            }
            else if (node->GetInterface() == BGAPI2_NODEINTERFACE_BOOLEAN) {
                node->SetBool(value.toBool());
                return true;
            }
            else if (node->GetInterface() == BGAPI2_NODEINTERFACE_COMMAND) {
                node->Execute();
                return true;
            }
            else if (node->GetInterface() == BGAPI2_NODEINTERFACE_FLOAT) {
                node->SetDouble(value.toDouble());
                return true;
            }
            else if (node->GetInterface() == BGAPI2_NODEINTERFACE_ENUMERATION) {
                node->SetString(value.toString().toStdString().c_str());
                return true;
            }
            else if (node->GetInterface() == BGAPI2_NODEINTERFACE_STRING) {
                node->SetString(value.toString().toStdString().c_str());
                return true;
            }
            else {
                qWarning() << "Unknow data type" << node->GetInterface() << "for" << name;
            }
        }
        catch (BGAPI2::Exceptions::IException& ex)
        {
            qDebug() << "Error in " << __FUNCTION__ << "function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
        }
    }
    else if (attributeParts.length() == 4) {
        // store selector
        QVariant currentValue = getValue(attributeParts[1]);
        // set selector
        bool result = setValue(attributeParts[1], attributeParts[2]);
        if (result) {
            // set value
            result = setValue(attributeParts[3], value);
            // reset selector
            setValue(attributeParts[1], currentValue);
        }
        else {
            qWarning() << "Failed to set selector" << name << ":" << value;
        }

        return result;
    }
    else {
        qWarning() << "Camera setting for invalid attribute" << name << ":" << value;
    }

	return false;
}

BGAPI2::Node *BaumerCamera::getNode(QString name)
{
	BGAPI2::String nodeName = name.split("/").last().toStdString().c_str();

	try
	{
		return _device->GetRemoteNode(nodeName);
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return nullptr;
}

QMetaType::Type BaumerCamera::getAttributeType(QString name)
{
	try {
		BGAPI2::Node *node = getNode(name);

		if (node->GetInterface() == BGAPI2_NODEINTERFACE_CATEGORY) {
			return QMetaType::User;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_INTEGER) {
			return QMetaType::LongLong;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_REGISTER) {
			return QMetaType::QByteArray;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_BOOLEAN) {
			return QMetaType::Bool;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_COMMAND) {
			return QMetaType::Void;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_FLOAT) {
			return QMetaType::Float;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_ENUMERATION) {
			return QMetaType::QStringList;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_STRING) {
			return QMetaType::QString;
		}
		else if (node->GetInterface() == BGAPI2_NODEINTERFACE_PORT) {
			qWarning() << name << "BGAPI2_NODEINTERFACE_PORT";
			return QMetaType::User;
		}
		else {
			qWarning() << "Unknow data type" << node->GetInterface() << "for" << name;
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

    return QMetaType::User;
}

bool BaumerCamera::isAttributeReadable(QString name)
{
	try {
		BGAPI2::Node *node = getNode(name);
		return node->IsReadable();
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return false;
}

bool BaumerCamera::isAttributeWriteable(QString name)
{
	try {
		BGAPI2::Node *node = getNode(name);
		return node->IsWriteable();
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return false;
}

BGAPI2::String BaumerCamera::getCurrentAccessMode(QString name)
{
	try {
		BGAPI2::Node *node = getNode(name);
		return node->GetCurrentAccessMode();
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return BGAPI2_NODEACCESS_NOTIMPLEMENTED;
}

bool BaumerCamera::isAttributeCurrentlyReadable(QString name)
{
	return getCurrentAccessMode(name) == BGAPI2_NODEACCESS_READWRITE || getCurrentAccessMode(name) == BGAPI2_NODEACCESS_READONLY;
}

bool BaumerCamera::isAttributeCurrentlyWriteable(QString name)
{
	return getCurrentAccessMode(name) == BGAPI2_NODEACCESS_READWRITE || getCurrentAccessMode(name) == BGAPI2_NODEACCESS_WRITEONLY;
}

bool BaumerCamera::isAttributeSelector(QString name)
{
	try {
		BGAPI2::Node *node = getNode(name);
		return node->IsSelector();
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return false;
}

QStringList BaumerCamera::getAttributeSelectedNodeList(QString name)
{
	QStringList attributes;

	try {
		BGAPI2::Node *node = getNode(name);
		BGAPI2::NodeMap *nodeTree = node->GetSelectedNodeList();

		for (bo_uint64 i = 0; i < nodeTree->GetNodeCount(); i++) {
			BGAPI2::Node *node = nodeTree->GetNodeByIndex(i);
			attributes.append(QString(node->GetName().get()));
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qDebug() << "Error in function: " << ex.GetFunctionName() << "for " << name << "Error description: " << ex.GetErrorDescription();
	}

	return attributes;
}

QString BaumerCamera::getAttributeCategory(QString name)
{
	QStringList list = name.split("/");
	list.removeLast();

	return list.join("/");
}

QStringList BaumerCamera::getAttributeList(BGAPI2::Node *startNode)
{
	QStringList attributes;

	try {
		BGAPI2::NodeMap *nodeTree = nullptr;
		if (startNode) {
			nodeTree = startNode->GetNodeTree();
		}
		else {
			nodeTree = _device->GetRemoteNodeTree();
		}

		for (bo_uint64 i = 0; i < nodeTree->GetNodeCount(); i++) {
			BGAPI2::Node *node = nodeTree->GetNodeByIndex(i);
			if (node->GetInterface() == BGAPI2_NODEINTERFACE_CATEGORY) {
				QStringList subAttributes = getAttributeList(node);
				for (const QString &attr : subAttributes) {
					attributes.append(QString(node->GetName().get()) + "/" + attr);
				}
			}
			else {
				if (node->GetImplemented() == true && node->GetVisibility() != BGAPI2_NODEVISIBILITY_INVISIBLE && node->GetCurrentAccessMode() != BGAPI2_NODEACCESS_WRITEONLY) {
					attributes.append(node->GetName().get());
				}
				else {
					//qDebug() << "not listing" << node->GetName() << "implemented:" << node->GetImplemented() << "visibility:" << node->GetVisibility() << "acces:" << node->GetCurrentAccessMode();
				}
			}
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		// qWarning() << "Error of type" << ex.GetType() << "in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}

    return attributes;
}

QString BaumerCamera::getCameraTitle()
{
    return getCameraName() + " (" + _device->GetSerialNumber().get() + ")";
}

QString BaumerCamera::getCameraName()
{
    QString name = getValue("DeviceUserID").toString();
    if (name.isNull()) {
        name = _device->GetDisplayName().get();
    }

    return name;
}

void BaumerCamera::updateValues()
{
    // iterate over all attributes
    for (const QString &name : getAttributeList()) {
        // check if attribute is readable
		if (isAttributeReadable(name)) {
            if (getAttributeType(name) == QMetaType::QByteArray) {
                // ignore registers
            } else {
                QVariant value = getValue(name);

                // check if attribute has changed
                if (value.isValid() && value != _attributes.value(name)) {
                    _attributes.insert(name, value);
                    emit valueChanged(name, value);
                }
            }
        }
    }
}

void BaumerCamera::loadSettings()
{
    QSettings settings;
    settings.beginGroup("Camera/" + QString(_device->GetSerialNumber()));

    for (const QString &name : settings.allKeys()) {
        QVariant value = settings.value(name);
		if (value.isValid()) {
            setValue(name, value);
		}
		else {
			qWarning() << "Invalid Camera setting for attribue" << name << ":" << value;
		}
    }
}

void BaumerCamera::saveSettings()
{
	// collect selectors
	QMap<QString, QStringList> selectors;
	for (const QString &name : getAttributeList()) {
		if (isAttributeSelector(name)) {
			selectors[name] = getAttributeSelectedNodeList(name);
		}
	}
	
    QSettings settings;
    settings.beginGroup("Camera/" + QString(_device->GetSerialNumber()));

    for (const QString &name : getAttributeList()) {
		QString displayName = name.split("/").last();
		bool isDependent = false;
		for (QStringList values : selectors) {
			if (values.contains(displayName)) {
				isDependent = true;
				break;
			}
		}
		
		if (isDependent) {
			// this is a dependent, so do not store here
		} else if (isAttributeSelector(name)) {
			// iterate over all selector options and store values
			if (isAttributeCurrentlyWriteable(name)) {
				QString currentValue = getValue(name).toString();
				QStringList selectorOptions = getRangeForValue(name, 0).toStringList();
				for (const QString &selectorValue : selectorOptions) {
					// choose option
					bool result = setValue(name, selectorValue);
					if (result) {
						for (const QString &value : selectors[name]) {
							if (isAttributeReadable(value) && isAttributeWriteable(value)) {
								QVariant optionValue = getValue(value);
								if (optionValue.isValid()) {
									settings.setValue(name + "/" + selectorValue + "/" + value, optionValue);
								}
							}
						}
					}
				}
				// reset selector
				setValue(name, currentValue);
			}
		} else if (isAttributeReadable(name) && isAttributeWriteable(name)) {
			// standard attribute
            QVariant value = getValue(name);
            if (value.isValid())
                settings.setValue(name, value);
        }
    }
}

void BaumerCamera::resetCamera()
{

}

void BaumerCamera::setupCamera()
{
	// register device callbacks
	try
	{
		_device->RegisterDeviceEvent(BGAPI2::Events::EVENTMODE_EVENT_HANDLER);
		_device->RegisterDeviceEventHandler(this, (BGAPI2::Events::DeviceEventHandler) &DevEventHandler);
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qWarning() << "Error of type" << ex.GetType() << "in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}

	// Enable external trigger by default
	setValue("TriggerMode", "On");
}

void BaumerCamera::initialize()
{
    // cyclic update of values
    _timer = new QTimer();
    connect(_timer, &QTimer::timeout, this, &BaumerCamera::updateValues);
    _timer->start(1000);
    
    // start camera
    startCapturing();
}

void BaumerCamera::startCapturing()
{
	try {
		// get data stream
		_dataStream = nullptr;
		BGAPI2::DataStreamList *datastreamList = _device->GetDataStreams();
		datastreamList->Refresh();
		if (datastreamList->size() > 1) {
			qWarning() << "More than one data stream found:" << datastreamList->size();
		}

		for (BGAPI2::DataStreamList::iterator dstIterator = datastreamList->begin(); dstIterator != datastreamList->end(); dstIterator++) {
			_dataStream = dstIterator->second;
			_dataStream->Open();

			_bufferList = _dataStream->GetBufferList();
			
			// 4 buffers using internal buffer mode
			for (int i = 0; i<FRAMES_COUNT; i++) {
				BGAPI2::Buffer *buffer = new BGAPI2::Buffer();
				_bufferList->Add(buffer);
			}
			
			for (BGAPI2::BufferList::iterator bufIterator = _bufferList->begin(); bufIterator != _bufferList->end(); bufIterator++) {
				bufIterator->second->QueueBuffer();
			}
			
			// register stream callbacks
			_dataStream->RegisterNewBufferEvent(BGAPI2::Events::EVENTMODE_EVENT_HANDLER);
			_dataStream->RegisterNewBufferEventHandler(this, (BGAPI2::Events::NewBufferEventHandler) &BufferHandler);

			// start acquisition
			_dataStream->StartAcquisitionContinuous();
		}

		_device->GetRemoteNode("AcquisitionStart")->Execute();
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qWarning() << "Error of type" << ex.GetType() << "in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}
}

void BaumerCamera::unplug()
{
    _timer->stop();
    delete _timer;
    
    // notify
    emit unplugged();

    // stop capturing
    stopCapturing();
}

void BaumerCamera::stopCapturing()
{
	try
	{
		if (_dataStream) {
			_device->GetRemoteNode("AcquisitionStop")->Execute();
			_dataStream->StopAcquisition();
			_dataStream->UnregisterNewBufferEvent();
		}

		if (_bufferList) {
			_bufferList->DiscardAllBuffers();
			while (_bufferList->size() > 0) {
				BGAPI2::Buffer *buffer = _bufferList->begin()->second;
				_bufferList->RevokeBuffer(buffer);
				delete buffer;
			}
		}

        if (_dataStream) {
            _dataStream->Close();
            _dataStream = nullptr;
        }

		if (_device) {
			_device->UnregisterDeviceEvent();
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qWarning() << "Error of type" << ex.GetType() << "in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}
}

inline cv::Point2f rotateAround(cv::Point2f point, double angle, cv::Point2f center)
{
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    double distance = qSqrt(qPow(dx, 2) + qPow(dy, 2));
    double src_angle = qAtan2(dx, dy);
    double new_angle = angle * M_PI / 180.0 + src_angle;

    double dx1 = qSin(new_angle) * distance;
    double dy1 = qCos(new_angle) * distance;

    return cv::Point2f(dx1 + center.x, dy1 + center.y);
}

void BaumerCamera::eventCB(BGAPI2::Events::DeviceEvent *devEvent)
{
    emit cameraEvent(QString(devEvent->GetId()).toULong(), devEvent->GetTimeStamp());
}

void BaumerCamera::frameDone(BGAPI2::Buffer *buffer)
{
    if (buffer->GetPayloadType() == BGAPI2_PAYLOADTYPE_IMAGE) {
        if (buffer->GetPixelFormat() == "Mono8") {
            // create opencv image of frame and copy underlying data
            cv::Mat image(buffer->GetHeight(), buffer->GetWidth(), CV_8U, buffer->GetMemPtr());
            image = image.clone();

            // forward image
            emit newImage(_device->GetSerialNumber().get(), buffer->GetFrameID(), buffer->GetIsIncomplete(), image);
        }
        else {
            qWarning() << "Unsupported Pixel Format" << buffer->GetPixelFormat();
        }
    }
    else {
        qWarning() << "Received Buffer with unhandeled type:" << buffer->GetPayloadType();
    }
}

void BGAPI2CALL DevEventHandler(void * callbackOwner, BGAPI2::Events::DeviceEvent * pDevEvent)
{
	if (pDevEvent) {
		// get pointer to correct thread
		BaumerCamera *camera = (BaumerCamera *)callbackOwner;
		if (camera) {
			camera->eventCB(pDevEvent);
		}
	}
	else {
		qWarning() << "Invalid device event!";
	}
}

void BGAPI2CALL BufferHandler(void * callBackOwner, BGAPI2::Buffer * pBufferFilled)
{
	try
	{
		if (pBufferFilled == nullptr) {
			qWarning() << "Error: Buffer Timeout after 1000 msec";
		}
		else if (pBufferFilled->GetIsIncomplete() == true) {
			qWarning() << "Error: Image is incomplete!";
			// queue buffer again
			pBufferFilled->QueueBuffer();
		}
		else {
			// get pointer to correct thread
			BaumerCamera *camera = (BaumerCamera *)callBackOwner;
			if (camera) {
				camera->frameDone(pBufferFilled);
			}

			// queue buffer again
			pBufferFilled->QueueBuffer();
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		qWarning() << "Error of type" << ex.GetType() << "in function: " << ex.GetFunctionName() << "Error description: " << ex.GetErrorDescription();
	}
}
