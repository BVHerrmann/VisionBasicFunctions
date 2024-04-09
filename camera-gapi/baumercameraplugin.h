#ifndef BAUMERCAMERAPLUGIN_H
#define BAUMERCAMERAPLUGIN_H

#include <plugin.h>
#include <interfaces.h>

#include <QHash>
#include <QVariant>
#include <QMutex>
class QWidget;
class QString;
class QThread;

namespace cv {
    class Mat;
}

#include <bgapi2_genicam/bgapi2_genicam.hpp>

#include "baumercamerawidget.h"
class BaumerCamera;


class BaumerCameraPlugin : public Plugin, PluginInterface, MainWindowInterface, CommunicationInterface, ImageProducerInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.BaumerCameraPlugin")
	Q_INTERFACES(PluginInterface)
	Q_INTERFACES(MainWindowInterface)
	Q_INTERFACES(CommunicationInterface)
	Q_INTERFACES(ImageProducerInterface)

public:
    explicit BaumerCameraPlugin(QObject *parent = 0);
    virtual ~BaumerCameraPlugin();

    // PluginInterface
    const QString identifier() const override { return "camera-gapi"; }
    const QString name() const override { return tr("Camera"); }
    QThread::Priority priority() const override { return QThread::IdlePriority; }
    const MachineState machineState() const override { return _current_state; }

    // MainWindowInterface
	const WidgetType widgetType(const int idx) const override { Q_UNUSED(idx); return Diagnostics; }
    const QString title(const int idx) const override { Q_UNUSED(idx); return name(); }
    QWidget * mainWidget(const int idx) const override { Q_UNUSED(idx); return _widget; }
    int preferredTabIndex(const int idx) const override { Q_UNUSED(idx); return 100; }
    int requiredWidgetAccessLevel(const int idx) const override { Q_UNUSED(idx); return kAccessLevelAdmin; }

signals:
    // CommunicationInterface
    void valueChanged(const QString &name, const QVariant &value) override;
    void valuesChanged(const QHash<QString, QVariant> &values) override;

    // ImageProducerInterface
    void newImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image);

    void pluggedCamera(QWeakPointer<BaumerCamera> camera_pointer);
    void unpluggedCamera(QWeakPointer<BaumerCamera> camera_pointer);

public slots:
    // PluginInterface
    void initialize() override;
    void uninitialize() override;
    void requestMachineState(const PluginInterface::MachineState state) override { _current_state = state; }

    // CommunicationInterface
    void setValue(const QString &name, const QVariant &value) override;
    void setValues(const QHash<QString, QVariant> &values) override;

    // Callbacks
    void plug(QString cameraId);
    void unplug(QString cameraId);

protected:

private:
    PluginInterface::MachineState _current_state = PluginInterface::Production;

    BaumerCameraWidget *_widget;

	BGAPI2::SystemList *_systemList;

    QHash<QString, QSharedPointer<BaumerCamera> > _threads;

    void setupUi();

    QMutex _mutex;
};

#endif // BAUMERCAMERAPLUGIN_H
