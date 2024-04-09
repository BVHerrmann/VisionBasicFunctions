#include "baumercameraview.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <imagewidget.h>

#include "baumercameraitemdelegate.h"
#include "baumercamera.h"


BaumerCameraView::BaumerCameraView(QWeakPointer<BaumerCamera> camera_pointer, QWidget *parent) :
    QWidget(parent)
{
    // remember camera
    _camera_pointer = camera_pointer;

    QSharedPointer<BaumerCamera> camera = _camera_pointer.toStrongRef();
    if (camera) {
        // prepare Ui
        setupUi();

        // let view know about changes of values
        connect(camera.data(), &BaumerCamera::valueChanged, this, &BaumerCameraView::setValue);

        // register for events
        connect(camera.data(), &BaumerCamera::cameraEvent, this, &BaumerCameraView::cameraEvent);
    }
}

BaumerCameraView::~BaumerCameraView()
{

}

void BaumerCameraView::setupUi()
{
    QSharedPointer<BaumerCamera> camera = _camera_pointer.toStrongRef();
    if (!camera)
        return;

    QGridLayout *layout = new QGridLayout(this);
    layout->setColumnStretch(0, 3);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(0, 2);

    _image_view = new ImageWidget(this);
    connect(camera.data(), &BaumerCamera::newImage, this, &BaumerCameraView::consumeImage);
    connect(this, &BaumerCameraView::finishedProcessing, _image_view, &ImageWidget::displayProcessingResult);
    layout->addWidget(_image_view, 0, 0, 2, 1);

	BaumerCameraItemDelegate *item_delegate = new BaumerCameraItemDelegate(this);

    _tree_widget = new QTreeWidget(this);
    _tree_widget->setItemDelegate(item_delegate);
    populateTreeView();
    layout->addWidget(_tree_widget, 0, 1, 1, 1);

    connect(_tree_widget, &QTreeWidget::itemChanged, this, &BaumerCameraView::itemChanged);

    _eventLabel = new QTextEdit();
    _eventLabel->setReadOnly(true);
    layout->addWidget(_eventLabel, 1, 1, 1, 1);

    QMetaObject::connectSlotsByName(this);
}

void BaumerCameraView::populateTreeView()
{
    QSharedPointer<BaumerCamera> camera = _camera_pointer.toStrongRef();
    if (!camera)
        return;

    _tree_widget->setColumnCount(2);
    _tree_widget->setColumnWidth(0, 200);
    QStringList labels;
    labels << tr("Attribute") << tr("Value");
    _tree_widget->setHeaderLabels(labels);

    for (const QString &attribute : camera->getAttributeList()) {
        QString category = camera->getAttributeCategory(attribute);
        QStringList categoryPartList = category.split("/", QString::SkipEmptyParts);

        QTreeWidgetItem *parent = nullptr;
        for (const QString &categoryPart : categoryPartList) {
            QTreeWidgetItem *item = nullptr;

            // check if the path already exists
            QList<QTreeWidgetItem *> items = _tree_widget->findItems(categoryPart, Qt::MatchExactly | Qt::MatchRecursive);
            for (QTreeWidgetItem *leaf : items) {
                if (leaf->parent() == parent) {
                    item = leaf;
                    break;
                }
            }

            // have to create a new item
            if (!item) {
                if (parent) {
                    item = new QTreeWidgetItem(parent, QStringList(categoryPart));
                } else {
                    item = new QTreeWidgetItem(QStringList(categoryPart));
                }
                _tree_widget->addTopLevelItem(item);
            }

            // set this item as parent
            parent = item;
        }

        // add this attribute
        QStringList values(attribute.split("/").last());
        if (camera->isAttributeReadable(attribute)) {
            QVariant value = camera->getValue(attribute);
            if (value.isValid())
            values << value.toString();
        }
		
        QTreeWidgetItem *item = new QTreeWidgetItem(parent, values);

        if (camera->isAttributeWriteable(attribute))
            item->setFlags(item->flags() |= Qt::ItemIsEditable);
        else
            item->setData(1, Qt::ForegroundRole, QBrush(Qt::darkGray));

        // set item data
        QList<QVariant> user_data;
        user_data << camera->isAttributeWriteable(attribute);
        if (camera->isAttributeWriteable(attribute)) {
            int attributeType = camera->getAttributeType(attribute);
            user_data << attributeType;
            if (attributeType == QMetaType::QStringList
                || attributeType == QMetaType::UInt
                || attributeType == QMetaType::LongLong
                || attributeType == QMetaType::Float) {
                user_data << camera->getMinimumForValue(attribute);
                user_data << camera->getMaximumForValue(attribute);
            }
        }
        item->setData(1, Qt::UserRole, user_data);

        _tree_widget->addTopLevelItem(item);
        //treeWidget->openPersistentEditor(item, 1);
    }
}

QString BaumerCameraView::title()
{
    QSharedPointer<BaumerCamera> camera = _camera_pointer.toStrongRef();

    if (camera)
        return camera->getCameraTitle();

    return tr("Unknown");
}

void BaumerCameraView::itemChanged(QTreeWidgetItem * item, int column)
{
    QSharedPointer<BaumerCamera> camera = _camera_pointer.toStrongRef();
    if (!camera)
        return;

    Q_UNUSED(column);

    // TODO: don't use Qt::DisplayRole here
    QString name = item->data(0, Qt::DisplayRole).toString();
    QString value = item->data(1, Qt::DisplayRole).toString();

    if (camera->isAttributeWriteable(name)) {
        camera->setValue(name, value);
    }
}

void BaumerCameraView::setValue(const QString &name, const QVariant &value)
{
    QString displayName = name.split("/").last();
	QList<QTreeWidgetItem *> items = _tree_widget->findItems(displayName, Qt::MatchExactly | Qt::MatchRecursive);
	
    // Just go through the list. Even if there should be only one item at all times.
    for (QTreeWidgetItem *item : items) {
        item->setText(1, value.toString());
    }
}

void BaumerCameraView::cameraEvent(unsigned long eventId, quint64 timestamp)
{
    QString text = _eventLabel->toPlainText();
    text.prepend(QString("%1: %2\n").arg(timestamp).arg(eventId));
    while (text.length() > 1024 * 1024) {
        int pos = text.lastIndexOf("\n");
        if (pos == -1)
            break;
        else
            text.chop(text.length() - pos);
    }
    _eventLabel->setPlainText(text);
}

void BaumerCameraView::consumeImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image)
{
    Q_UNUSED(frameStatus);

    // ignore all frames if display is not visible
    if (!isVisible())
        return;

    // run processing of image in background
    auto future = std::async(std::launch::async, &BaumerCameraView::processImage, this, cameraId, frameNumber, image);
}

#define PI 3.14159265358979323846
inline QPointF rotateAround(QPointF point, double angle, cv::Point2f center)
{
    double dx = point.x() - center.x;
    double dy = point.y() - center.y;
    double distance = qSqrt(qPow(dx, 2) + qPow(dy, 2));
    double src_angle = qAtan2(dx, dy);
    double new_angle = angle * PI / 180.0 + src_angle;

    double dx1 = qSin(new_angle) * distance;
    double dy1 = qCos(new_angle) * distance;

    return QPointF(dx1 + center.x, dy1 + center.y);
}

void BaumerCameraView::processImage(const QString &cameraId, unsigned long frameNumber, const cv::Mat &image)
{
    QSharedPointer<BaumerCamera> camera = _camera_pointer.toStrongRef();
    if (!camera)
        return;

    // make sure only one image is processed at any point
    if (_image_mutex.tryLock()) {
        qDebug() << "CameraView received frame" << frameNumber;
    } else {
        qDebug() << "CameraView dropped frame" << frameNumber;
        return;
    }

    // create result for image
    std::shared_ptr<const ProcessingResult> result = std::shared_ptr<const ProcessingResult>(new ProcessingResult(qHash(cameraId), image));

    // unlock
    _image_mutex.unlock();

    // display result
    emit finishedProcessing(result);
}
