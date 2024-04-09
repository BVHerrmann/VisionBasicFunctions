#include "loggerwidget.h"

#include <QtGui>
#include <QtWidgets>

#include "loggermodel.h"


LoggerWidget::LoggerWidget(LoggerModel *model, QWidget *parent) :
    QWidget(parent)
{
    _auto_scroll = true;
    setupUi();

    // set model
    _model = model;
    _table_view->setModel(model);
    _table_view->setColumnHidden(0, true);
    _table_view->setColumnWidth(1, 170);
}

void LoggerWidget::setupUi()
{
    _table_view = new QTableView(this);
//    _table_view->setContentsMargins(0, 0, 0, 0);

    _table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _table_view->setShowGrid(false);
    _table_view->setAlternatingRowColors(true);
    _table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    //tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    _table_view->setSelectionMode(QAbstractItemView::NoSelection);

    // this will cause a slow-down
    _table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    _table_view->horizontalHeader()->setStretchLastSection(true);
    _table_view->horizontalHeader()->setSectionsClickable(false);
    _table_view->verticalHeader()->hide();

    // this will cause a slow-down
    // tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    QGridLayout *layout = new QGridLayout(this);
//    layout->setContentsMargins(12, 12, 12, 12);
    layout->addWidget(_table_view, 0, 0);

    setLayout(layout);
}

void LoggerWidget::showEvent(QShowEvent *event)
{
    (void)event;

    _model->pauseRemovingItems();
    _table_view->scrollToBottom();
}

void LoggerWidget::hideEvent(QHideEvent *event)
{
    (void)event;

    _model->resumeRemovingItems();
}

void LoggerWidget::exportToFile()
{
    QSettings settings;
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Log File"), settings.value("Logger/LastDir").toString() + "Inspector-" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss.log"), tr("Log Files (*.log)"));
    
    if (fileName.isNull())
        return;
    
    // store last folder
    QFileInfo info(fileName);
    settings.setValue("Logger/LastDir", info.dir().canonicalPath() + "/");
    
    // save log
    _model->saveAs(fileName);
}
