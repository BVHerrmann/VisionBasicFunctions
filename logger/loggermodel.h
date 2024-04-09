#ifndef LOGGERMODEL_H
#define LOGGERMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QMutex>

class LoggerItem;


class LoggerModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LoggerModel(QObject *parent = 0);
    virtual ~LoggerModel();

    // QAbstractTableModel
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void saveAs(QString &fileName) const;
    
    void pauseRemovingItems() { _removeItems = false; }
    void resumeRemovingItems() { _removeItems = true; }
    
signals:

public slots:
    // PreferencesInterface
    void loadPreferences();

    void log(LoggerItem *item);

private:
    QList<QString> _columns;
    QList<LoggerItem *> _items;

    bool _removeItems;
    uint _maxResults;
};

#endif // LOGGERMODEL_H
