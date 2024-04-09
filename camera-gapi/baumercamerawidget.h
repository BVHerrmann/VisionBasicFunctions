#ifndef BAUMERCAMERAWIDGET_H
#define BAUMERCAMERAWIDGET_H

#include <QWidget>

class QLabel;
class QStackedWidget;
class QTabWidget;

#include <thirdlevelnavigationwidget.h>

class BaumerCamera;


class BaumerCameraWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaumerCameraWidget(QWidget *parent = 0);

signals:

public slots:
    // CommunicationInterface
    void setValue(const QString &name, const QVariant &value);

    void registerCamera(QWeakPointer<BaumerCamera> camera_pointer);
	void unregisterCamera(QWeakPointer<BaumerCamera> camera_pointer);

private:
    QStackedWidget *_stack;

    ThirdLevelNavigationWidget *_tab_widget;
    QLabel *_label;

    void setupUi();
};

#endif // BAUMERCAMERAWIDGET_H
