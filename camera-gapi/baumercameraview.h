#ifndef BAUMERCAMERAVIEW_H
#define BAUMERCAMERAVIEW_H

#include <QMutex>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;

namespace cv {
    class Mat;
}

class BaumerCamera;
class ImageWidget;
#include "processingresult.h"


class BaumerCameraView : public QWidget
{
    Q_OBJECT
public:
    explicit BaumerCameraView(QWeakPointer<BaumerCamera> camera_pointer, QWidget *parent = 0);
    virtual ~BaumerCameraView();

    QString title();
    QWeakPointer<BaumerCamera> camera() const { return _camera_pointer; }

signals:
    void finishedProcessing(std::shared_ptr<const ProcessingResult> result);

public slots:
    // CommunicationInterface
    void setValue(const QString &name, const QVariant &value);

    // ImageConsumerInterface
    void consumeImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image);

    // Events
    void cameraEvent(unsigned long eventId, quint64 timestamp);

    void itemChanged(QTreeWidgetItem * item, int column);

private:
    void processImage(const QString &cameraId, unsigned long frameNumber, const cv::Mat &image);

    QWeakPointer<BaumerCamera> _camera_pointer;
    QMutex _image_mutex;

    ImageWidget *_image_view;
    QTreeWidget *_tree_widget;
    QTextEdit *_eventLabel;

    void setupUi();
    void populateTreeView();
};

#endif // BAUMERCAMERAVIEW_H
