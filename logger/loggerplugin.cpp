#include "loggerplugin.h"

#include "loggerdefines.h"
#include "loggeritem.h"


Q_DECLARE_METATYPE(QtMsgType)

static LoggerPlugin *logger = 0;

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    QString threadName = QThread::currentThread()->objectName();

    if (logger != 0) {
        QMetaObject::invokeMethod(logger, "log", Qt::QueuedConnection, Q_ARG(QtMsgType, type), Q_ARG(QMessageLogContext, context), Q_ARG(QString, msg), Q_ARG(QDateTime, timestamp), Q_ARG(QString, threadName));
    }
}

LoggerPlugin::LoggerPlugin(QObject *parent) :
    Plugin(parent)
{
    qRegisterMetaType<QtMsgType>("QtMsgType");
    qRegisterMetaType<QtMsgType>("QMessageLogContext");
    qRegisterMetaType<LoggerItem *>("LoggerItem *");
    
    _logDir = logDirectory();
    _logFile = nullptr;

    // initialize model
    _model = new LoggerModel();

    // initialize views
    _widget = new LoggerWidget(_model);
	_optionPanel = new LoggerOptionPanel(_model);
    
    loadPreferences();
}

LoggerPlugin::~LoggerPlugin()
{
    delete _model;
}

void LoggerPlugin::initialize()
{
    // init log file
    updateLogFile();

    // start timer to watch log file
    _timer = startTimer(3000);

    // install custom message handler and start logging
    logger = this;
    qInstallMessageHandler(messageOutput);
    qDebug() << "Logging Database Initialized";
}

void LoggerPlugin::uninitialize()
{
    // stop timer
    killTimer(_timer);
    
    // restore previouse message handler
    qInstallMessageHandler(0);
    logger = 0;

    _logFile->flush();
}

void LoggerPlugin::loadPreferences()
{
    _model->loadPreferences();
}

QDir LoggerPlugin::logDirectory() const
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QDir::separator() + "Logs");
    qDebug() << "Log directory:" << dir.path();
    
    if (!dir.exists()) {
        bool result = dir.mkpath(dir.path());
        if (!result) {
            qWarning() << "Failed to create log dir at:" << dir.path();
        }
    }
    
    return dir;
}

void LoggerPlugin::updateLogFile()
{
    QMutexLocker locker(&_fileMutex);

    // close log file if necessary
    if (_logFile != nullptr) {
        _logFile->close();
        delete _logFile;
        _logFile = nullptr;
    }

    // open new log file
    _logFile = new QFile(_logDir.absoluteFilePath(QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss.log")), this);
    if (!_logFile->open(QIODevice::Append | QIODevice::Text))
        qCritical() << "Failed to open log file:" << _logFile->fileName();
}

void LoggerPlugin::cleanLogFiles()
{
    QMutexLocker locker(&_fileMutex);

    QStringList nameFilters;
    nameFilters << "*.log";
    QStringList files = _logDir.entryList(nameFilters, QDir::Files | QDir::Writable, QDir::Name);

    while (files.count() > kDefaultLogFiles) {
        QString fileName = files.first();

        if (!_logDir.remove(fileName))
            qCritical() << "Failed to remove" << _logDir.absoluteFilePath(fileName);

        files.removeFirst();
    }
}

void LoggerPlugin::timerEvent(QTimerEvent *event)
{
    (void)event;

    if(_logFile->size() > kDefaultLogFileSize) {
        updateLogFile();
        cleanLogFiles();
    }
}

void LoggerPlugin::log(QtMsgType type, const QMessageLogContext &context, const QString &msg, const QDateTime &timestamp, const QString &source)
{
    (void)context;
    
    assert(QThread::currentThread() == thread());
    
    // create log item
    LoggerItem *item = new LoggerItem(type, msg, timestamp, source);
    QString complete = item->toString();
    
    // log message to console
    fprintf(stderr, "%s", qPrintable(complete));
    
    // log message to gui
    QMetaObject::invokeMethod(_model, "log", Qt::QueuedConnection, Q_ARG(LoggerItem*, item));

    // aquire mutex and write to file
    QMutexLocker locker(&_fileMutex);
    _logFile->write(complete.toUtf8());
}
