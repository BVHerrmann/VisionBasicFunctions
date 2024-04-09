#include "CameraSimulation.h"
#include "ImageData.h"


CameraSimulation::CameraSimulation(ImageData *pImageData) : QThread()
, m_TerminateInspection(false)
, m_ImageData(NULL)
, m_VideoState(2)
, m_SimulationFrameInterval(100)
{
	m_ImageData = pImageData;
}


CameraSimulation::~CameraSimulation()
{
   WaitForFinshed();
   wait();
}


void CameraSimulation::StartSimulation(QString &FileLocation)
{
	m_FileLocation = FileLocation;
	m_TerminateInspection = false;
	start();
}


void CameraSimulation::WaitForFinshed()
{
	if (isRunning())
	{//thread läuft noch
		m_WaitConditionGetNextImage.wakeAll();
		m_TerminateInspection = true;
		m_WaitThreadIsFinished.lock();
		m_WaitConditionThreadIsFinished.wait(&m_WaitThreadIsFinished, 15000);
		m_WaitThreadIsFinished.unlock();
	}
}

void CameraSimulation::StepImage()
{
	m_WaitConditionGetNextImage.wakeAll();
}


void CameraSimulation::run()
{
	QString PathAndFileNameImage;
	QDir Path;
	QStringList filters;
	QFileInfoList list;
	cv::Mat         Image;
	QMutex mutex;

	
	Path.setPath(m_FileLocation);
	filters IMAGE_FITERS;
	Path.setFilter(QDir::Files);
	Path.setNameFilters(filters);
	list = Path.entryInfoList();
	
	while (!m_TerminateInspection)
	{
		for (int i = 0; i < list.count(); i++)
		{
			if (m_TerminateInspection)
				break;
			PathAndFileNameImage = list.at(i).filePath();
			Image = cv::imread(PathAndFileNameImage.toLatin1().data(), cv::IMREAD_GRAYSCALE);
			if (Image.data)
			{
					if (m_VideoState == 1)
					{
						GetImageData()->NewIncommingImage(Image);
					}
					else
					{
					    mutex.lock();
						m_WaitConditionGetNextImage.wait(&mutex);
						m_CurrentFilename = PathAndFileNameImage;
						if (!m_TerminateInspection)
							GetImageData()->NewIncommingImage(Image);
						mutex.unlock();
					}
					msleep(m_SimulationFrameInterval);
			}
    	}
		if (list.count() == 0)
		{
			emit SignalShowMessage(tr("Can Not Run Simulation! No Files(*.%1) In Dir:%2").arg("bmp").arg(m_FileLocation), QtMsgType::QtFatalMsg);
			msleep(m_SimulationFrameInterval);
		}
	}
    m_WaitConditionThreadIsFinished.wakeAll();
}




