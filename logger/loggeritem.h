#ifndef LOGGERITEM_H
#define LOGGERITEM_H

#include <QObject>
#include <QDateTime>


class LoggerItem : public QObject
{
    Q_OBJECT
public:
    explicit LoggerItem(QtMsgType type, QString msg, QDateTime timestamp, QString source, QObject *parent = 0);

    QVariant data(int column, int role) const;

    QString toString() const;
    
signals:

public slots:

private:
    QDateTime _timestamp;
    QtMsgType _type;
    QString _msg;
    QString _threadName;
};

#endif // LOGGERITEM_H
