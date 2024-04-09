#pragma once


#undef signals
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#define signals Q_SIGNALS

#include "qbytearray.h"
#include "qmutex.h"

#include <thread>
typedef struct
{
	gboolean white;
	GstClockTime timestamp;
	guint num_frames;
	GstClockTime duration;
} StreamContext;


class StreamingImage
{
public:
	StreamingImage();
	~StreamingImage();
	void createServer();
	void SetNewJpegRawData(QByteArray &JpegRawData);
private:
	static void need_data(GstElement * appsrc, guint unused, StreamContext * ctx);
	static void media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data);
	std::thread _serverThread;
	GMainLoop *m_ServerMainLoop;
	static QByteArray m_JpegRawData;
	static QMutex m_Mutex;
};

