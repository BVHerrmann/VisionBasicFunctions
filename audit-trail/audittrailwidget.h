#ifndef AUDITTRAILWIDGET_H
#define AUDITTRAILWIDGET_H

#include <QWidget>

class QModelIndex;
class QTableView;
class AuditTrailModel;


class AuditTrailWidget : public QWidget
{
Q_OBJECT
public:
    explicit AuditTrailWidget(AuditTrailModel *model, QWidget *parent = 0);

    void showEvent(QShowEvent *event) override;
signals:

public slots:

private:
    void setupUi();

    AuditTrailModel *_model;
    QTableView *_table_view;
    bool _auto_scroll;
};

#endif // AUDITTRAILWIDGET_H
