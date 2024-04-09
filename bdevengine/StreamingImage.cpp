#include "StreamingImage.h"



const guint FPS = 10;
const guint WIDTH = 640;
const guint HEIGHT = 480;

QMutex StreamingImage::m_Mutex;
QByteArray StreamingImage::m_JpegRawData;

StreamingImage::StreamingImage()
{
	m_ServerMainLoop = NULL;
	_serverThread = std::thread(&StreamingImage::createServer, this);
}


StreamingImage::~StreamingImage()
{
	if (m_ServerMainLoop)
		g_main_loop_quit(m_ServerMainLoop);
	if (_serverThread.joinable())
		_serverThread.join();
}

/* called when we need to give data to appsrc */
void StreamingImage::need_data(GstElement * appsrc, guint unused, StreamContext *ctx)
{
	GstMapInfo info;
	GstBuffer *buffer;
	guint size,HeaderSize;
	GstFlowReturn ret;
	unsigned char *data;
	char *jdata;
	gint num_samples = 1;

	
	HeaderSize = 0;
	m_Mutex.lock();
	size = m_JpegRawData.count() + HeaderSize;
	ctx->duration = ((double)num_samples / FPS) * GST_SECOND;
	ctx->timestamp = ctx->num_frames * ctx->duration;

	buffer = gst_buffer_new_allocate(NULL, size, NULL);
	gst_buffer_map(buffer, &info, (GstMapFlags)GST_MAP_WRITE);

	//data = (guchar *)info.data;
	//jdata = m_JpegRawData.data();

	data = (guchar *)info.data;
	/*for (int i = 0; i < HeaderSize; i++)
	{
		*data = 0xFF;// m_JpegRawData.at(i);
		data++;
	}
	*/
	for (int i = 0; i < m_JpegRawData.count(); i++)
	{
		*data = m_JpegRawData.at(i);
		data++;
	}

	//memcpy(data, jdata, size);

	
	

	/* this makes the image black/white */
	//gst_buffer_memset(buffer, 0, ctx->white ? 0xff : 0x0, size);
	//gst_buffer_memset((guchar *)(info.data), 0, 128, size);

	//std::memset((guchar *)(info.data), ctx->white ? 0xff : 0x0, size);

	//ctx->white = !ctx->white;

	
	GST_BUFFER_PTS(buffer) = ctx->timestamp;
	GST_BUFFER_DURATION(buffer) = ctx->duration;
	//ctx->timestamp += GST_BUFFER_DURATION(buffer);

	
	
	GST_BUFFER_DTS(buffer) = ctx->timestamp;
	GST_BUFFER_OFFSET(buffer) = ctx->num_frames;


	g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
	gst_buffer_unmap(buffer, &info);
	gst_buffer_unref(buffer);
	m_Mutex.unlock();
	ctx->num_frames++;
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
void StreamingImage::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media,gpointer user_data)
{
	GstElement *element, *appsrc;
	StreamContext *ctx;

	/* get the element used for providing the streams of the media */
	element = gst_rtsp_media_get_element(media);

	
	/* get our appsrc, we named it 'mysrc' with the name property */
	appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

	/* this instructs appsrc that we will be dealing with timed buffer */
	gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");

	

	/* configure the caps of the video */
	g_object_set(G_OBJECT(appsrc), "caps",
		gst_caps_new_simple("video/x-raw",
			"format", G_TYPE_STRING, "RGB",
			"width", G_TYPE_INT, WIDTH,
			"height", G_TYPE_INT, HEIGHT,
			"framerate", GST_TYPE_FRACTION, FPS, 1, NULL), NULL);
			
	ctx = g_new0(StreamContext, 1);
	ctx->white = TRUE;
	ctx->timestamp = 0;
	ctx->num_frames = 0;
	/* make sure ther datais freed when the media is gone */
	g_object_set_data_full(G_OBJECT(media), "my-extra-data", ctx,(GDestroyNotify)g_free);

	/* install the callback that will be called when a buffer is needed */
	g_signal_connect(appsrc, "need-data", (GCallback)need_data, ctx);
	gst_object_unref(appsrc);
	gst_object_unref(element);
}

void StreamingImage::createServer()
{
	GstRTSPServer *server;
	GstRTSPMountPoints *mounts;
	GstRTSPMediaFactory *factory;

	gst_init(NULL,NULL);

	m_ServerMainLoop = g_main_loop_new(NULL, FALSE);

	/* create a server instance */
	server = gst_rtsp_server_new();

	/* get the mount points for this server, every server has a default object
	 * that be used to map uri mount points to media factories */
	mounts = gst_rtsp_server_get_mount_points(server);

	/* make a media factory for a test stream. The default media factory can use
	 * gst-launch syntax to create pipelines.
	 * any launch line works as long as it contains elements named pay%d. Each
	 * element with pay%d names will be a stream */
	factory = gst_rtsp_media_factory_new();
	//format = I420
	//gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )");
	//gst_rtsp_media_factory_set_launch(factory,"( appsrc name=mysrc ! videoconvert ! video/x-raw, format=YUY2,width=384,height=288,framerate=10/1 ! x264enc ! rtph264pay name=pay0 pt=96 )");
	//gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc !  videoconvert ! video/x-raw ! jpegenc ! rtpjpegpay name = pay0 pt = 96 )");
	//gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! videoconvert ! vp8enc ! webmmux )");
	gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! videoconvert ! video/x-raw, format=YUY2, width=640, height=480, framerate=10/1 !  jpegenc ! rtpjpegpay name = pay0 pt = 96 )");

	/* notify when our media is ready, This is called whenever someone asks for
	 * the media and a new pipeline with our appsrc is created */
	g_signal_connect(factory, "media-configure", (GCallback)media_configure,NULL);

	/* attach the test factory to the /test url */
	gst_rtsp_mount_points_add_factory(mounts, "/stream1", factory);

	/* don't need the ref to the mounts anymore */
	g_object_unref(mounts);

	/* attach the server to the default maincontext */
	gst_rtsp_server_attach(server, NULL);

	/* start serving */
	//g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
	g_main_loop_run(m_ServerMainLoop);

	return;
}


void StreamingImage::SetNewJpegRawData(QByteArray &JpegRawData)
{
	m_Mutex.lock();
	m_JpegRawData = JpegRawData;
	m_Mutex.unlock();
}

