#pragma once



#include <opencv2/opencv.hpp>
#undef signals
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#define signals Q_SIGNALS

#include "halconcpp/HalconCpp.h"

#include <thread>
#include "qimage.h"
#include "qmutex.h"




typedef struct
{
	char name[7];
	int width;
	int height;
	int framerate;
} Resolution;


typedef struct
{
	bool white;
	guint sourceId;
	guint num_frames;
	GstClockTime duration, timestamp;
	Resolution img_resolution;
	GstElement *appsrc;
} MyContext;

class ImageData;
class VideoProducer
{
public:
	VideoProducer();
	~VideoProducer();
	void createServer();
	void SetNewJpegRawData(QByteArray &JpegRawData);
	void constructImage();
private:
	bool m_FirstCall;
	std::thread _imageThread;
	std::thread _serverThread;

	bool _bProcessing;
	
	//static std::mutex _img_mutex;
	static QMutex m_Mutex;
	static QByteArray m_JpegRawData1;
	static QByteArray m_JpegRawData;
    int _streams;

	
	static void start_need_data(GstElement * pipeline, guint size, MyContext * ctx);
	static void need_data(GstElement * appsrc, guint unused, MyContext * ctx);
	static void enough_data(GstElement *appSrc, MyContext * ctx);
	static void media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data);
	static long ReadBuffer(QString &Path, uint8_t *buffer);

	GMainLoop *m_ServerMainLoop;
	
	
};


