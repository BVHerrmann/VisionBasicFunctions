#include "audittrailwidget.h"

#include <QtGui>
#include <QtWidgets>

#include "audittrailmodel.h"


AuditTrailWidget::AuditTrailWidget(AuditTrailModel *model, QWidget *parent) :
    QWidget(parent)
{
    _auto_scroll = true;
    setupUi();

    // set model
    _model = model;
    _table_view->setModel(model);
    _table_view->setColumnHidden(0, true);
}

void AuditTrailWidget::setupUi()
{
    _table_view = new QTableView(this);

    _table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _table_view->setShowGrid(false);
    _table_view->setAlternatingRowColors(true);
    _table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    _table_view->setSelectionMode(QAbstractItemView::NoSelection);

    // this will cause a slow-down
    _table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    _table_view->horizontalHeader()->setStretchLastSection(true);
    _table_view->horizontalHeader()->setSectionsClickable(false);
    _table_view->verticalHeader()->hide();
    
    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(_table_view, 0, 0);

    setLayout(layout);
}

void AuditTrailWidget::showEvent(QShowEvent *event)
{
    (void)event;

    _model->select();
    _table_view->scrollToTop();
}
