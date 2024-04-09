#ifndef LOGGERWIDGET_H
#define LOGGERWIDGET_H

#include <QWidget>

class QModelIndex;
class QTableView;
class LoggerModel;


class LoggerWidget : public QWidget
{
Q_OBJECT
public:
    explicit LoggerWidget(LoggerModel *model, QWidget *parent = 0);

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
signals:

public slots:
    void exportToFile();

private:
    void setupUi();

    LoggerModel *_model;
    QTableView *_table_view;
    bool _auto_scroll;
};

#endif // LOGGERWIDGET_H
