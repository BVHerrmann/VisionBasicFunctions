#ifndef BAUMERCAMERA_H
#define BAUMERCAMERA_H

#include <QStringList>
#include <QTimer>
#include <QVariant>

#include <bgapi2_genicam/bgapi2_genicam.hpp>

namespace cv {
    class Mat;
}

#define FRAMES_COUNT 25


class BaumerCamera : public QObject
{
    Q_OBJECT
public:
    explicit BaumerCamera(BGAPI2::Device *device, QObject *parent = 0);
    virtual ~BaumerCamera();

    unsigned long cameraId() { return QString(_device->GetSerialNumber().get()).toULong(); }

    void run();

    void eventCB(BGAPI2::Events::DeviceEvent *devEvent);
    void frameDone(BGAPI2::Buffer *buffer);

    // CommunicationInterface
    QVariant getValue(QString name);
    QVariant getMinimumForValue(QString name);
    QVariant getMaximumForValue(QString name);

	BGAPI2::Node * getNode(QString name);

    QMetaType::Type getAttributeType(QString name);

    bool isAttributeReadable(QString name);
    bool isAttributeWriteable(QString name);
	BGAPI2::String getCurrentAccessMode(QString name);
	bool isAttributeCurrentlyReadable(QString name);
	bool isAttributeCurrentlyWriteable(QString name);
	bool isAttributeSelector(QString name);
    QString getAttributeCategory(QString name);

	QStringList getAttributeList(BGAPI2::Node *startNode = nullptr);
	QStringList getAttributeSelectedNodeList(QString name);

    QString getCameraTitle();
    QString getCameraName();

signals:
    // CommunicationInterface
    void valueChanged(const QString &name, const QVariant &value);

    // ImageProducerInterface
    void newImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image);

    void cameraEvent(unsigned long eventId, quint64 timestamp);
    void unplugged();

public slots:
    // CommunicationInterface
    bool setValue(const QString &name, const QVariant &value);

    void initialize();
    void unplug();
    
    void loadSettings();
    void saveSettings();

    void updateValues();

    void resetCamera();

private:
	BGAPI2::Device *_device;
	BGAPI2::DataStream *_dataStream;
	BGAPI2::BufferList *_bufferList;

    void setupCamera();
    void startCapturing();
    void stopCapturing();

    QTimer *_timer;
    QHash<QString, QVariant> _attributes;

    QVariant getRangeForValue(QString name, bool maximum);
};

#endif // BAUMERCAMERA_H
