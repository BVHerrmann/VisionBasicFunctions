#include "VideoProducer.h"
#include "Imagedata.h"
#include <fstream>

const int MAX_WIDTH = 640;
const int MAX_HEIGHT = 480;


static const Resolution stream[] = {
		//{ "mysrc1", 320, 240, 10 },
		//{ "mysrc2", 320, 240, 25 },
		{ "mysrc3", MAX_WIDTH, MAX_HEIGHT, 10 },
		//{ "mysrc4", 960, 720, 25 },
};



QMutex VideoProducer::m_Mutex;
QByteArray VideoProducer::m_JpegRawData;
QByteArray VideoProducer::m_JpegRawData1;


VideoProducer::VideoProducer()
{
	
	m_ServerMainLoop = NULL;
	_bProcessing = true;
	//_imageThread = std::thread(&VideoProducer::constructImage, this);
	_serverThread = std::thread(&VideoProducer::createServer, this);
	_streams = sizeof(stream) / sizeof(Resolution);
}


VideoProducer::~VideoProducer()
{
	_bProcessing = false;
	if(m_ServerMainLoop)
		g_main_loop_quit(m_ServerMainLoop);
	//if (_imageThread.joinable())
	//	_imageThread.join();
	
	if (_serverThread.joinable()) 
		_serverThread.join();
}


void VideoProducer::createServer()
{
	GstRTSPServer *server;
	GstRTSPMountPoints *mounts;
	GstRTSPMediaFactory *factory[1];// _streams];
	char *container = NULL;
	GstBus *bus;
	GstMessage *msg;

	gst_init(NULL, NULL);
	m_ServerMainLoop = g_main_loop_new(NULL, FALSE);
	// create a server instance 
	server = gst_rtsp_server_new();
	// get the mount points for this server, every server has a default object
	// that be used to map uri mount points to media factories 
	mounts = gst_rtsp_server_get_mount_points(server);
	// make a media factory array for our different streams. Each media factory can use
	// gst-launch syntax to create pipelines.
	// any launch line works as long as it contains elements named pay%d. Each
	// element with pay%d names will be a stream 
	for (int i = 0; i < _streams; ++i)
	{
		container = (char *)g_malloc(512);
		sprintf(container,
			//"( appsrc name=mysrc ! videoconvert ! jpegenc ! rtpjpegpay name=pay0 pt=96 )");
			//"(appsrc name=mysrc3 !rtspsrc location = rtsp://127.0.0.1:8554/stream1.sdp ! rtph264depay !  h264parse ! omxh264dec ! nvvidconv ! video/x-raw, width=(int)2048, height=(int)1536, format=(string)BGRx ! videoconvert !  ximagesink sync=false )");
			//"(appsrc name=mysrc3 !ffenc_mpeg4 !rtpmp4vpay send - config = true !udpsink host = 127.0.0.1 port = 8554)");
			//"( appsrc name=mysrc3 ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )");
			//"(udpsrc uri=udp://127.0.0.1:8554 ! appsrc name=%s ! videoconvert ! video/x-raw, format=YUY2,width=%i,height=%i,framerate=%i/1 !ffenc_mpeg4!  rtpjpegpay name=pay0 pt=96 )",
			//"(appsrc name=mysrc3 ! video/x-raw, format=YUY2, width=640,height=480,framerate=25/1,bpp=24,depth=24 ! fmpegcolorspace ! ideoscale method = 1 ! heoraenc bitrate = 150 ! dpsink host = 127.0.0.1 port = 8554 )");
			//"(appsrc name=%s ! videoconvert ! video/x-raw, format=YUY2,width=%i,height=%i,framerate=%i/1 ! jpegenc )",
			//"(v4l2src num - buffers = 1 !jpegenc !filesink location = d:/temp/DumpImage.jpeg )");
			//"( appsrc name=%1 ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )",
			//"( appsrc name=mysrc3 !videoconvert ! video/x-raw, format=I420,width=%i,height=%i,framerate=%i/1 !x264enc  !rtph264pay name=pay0 pt=96 )",
			//"(appsrc name=%s ! videoconvert ! video/x-raw, format=YUY2,width=%i,height=%i,framerate=%i/1 ! jpegenc !  rtpjpegpay name=pay0 pt=96 )",
			//"(videotestsrc pattern=snow ! videoconvert ! video/x-raw, format=YUY2,width=%i,height=%i,framerate=%i/1 ! jpegenc !  rtpjpegpay name=pay0 pt=96 )",
			"(videotestsrc !video/x-raw, format = YUY2 !videoconvert !autovideosink )");
			//"(appsrc name=%s ! videoconvert ! video/x-raw, format=YUY2,width=%i,height=%i,framerate=%i/1 ! jpegenc !  rtpjpegpay name=pay0 pt=96 )",
			//stream[i].name, stream[i].width, stream[i].height, stream[i].framerate);
		factory[i] = gst_rtsp_media_factory_new();
		gst_rtsp_media_factory_set_launch(factory[i], container);
		//notify when our media is ready, This is called whenever someone asks for
		// the media and a new pipeline with our appsrc is created 
		g_signal_connect(factory[i], "media-configure", (GCallback)media_configure, GINT_TO_POINTER(i));
		// attach the factory[i] to the /stream%d url 
		sprintf(container, "/stream%d", i+1);
		gst_rtsp_mount_points_add_factory(mounts, container, factory[i]);
		
	}
	// don't need the ref to the mounts anymore 
	g_object_unref(mounts);
	// attach the server to the default maincontext 
	gst_rtsp_server_attach(server, NULL);
	// start serving 
	g_main_loop_run(m_ServerMainLoop);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
void VideoProducer::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data)
{
	GstElement *element, *appsrc;
	MyContext *ctx = g_new0(MyContext, 1);

	//ctx->image_type = GPOINTER_TO_INT(user_data);
	int index = GPOINTER_TO_INT(user_data);// ctx->image_type - 1;
	ctx->img_resolution = stream[index];

	// get the element used for providing the streams of the media 
	element = gst_rtsp_media_get_element(media);

	// get our appsrc, we named it 'mysrc' with the name property 
	appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), stream[index].name);
	// this instructs appsrc that we will be dealing with timed buffer 
	gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
	// configure the caps of the video 
	g_object_set(G_OBJECT(appsrc),"caps",gst_caps_new_simple("video/x-raw","format",G_TYPE_STRING,"RGB","width",G_TYPE_INT,stream[index].width,"height",G_TYPE_INT, stream[index].height,"framerate", GST_TYPE_FRACTION, stream[index].framerate, 1,NULL),NULL);
	   
   	ctx->timestamp = 0;
	ctx->num_frames = 0;
	ctx->sourceId = 0;
	ctx->appsrc = appsrc;
	// make sure their datas freed when the media is gone 
	g_object_set_data_full(G_OBJECT(media), "my-extra-data", ctx, (GDestroyNotify)g_free);
	// install the callback that will be called when a buffer is needed 
	g_signal_connect(appsrc, "need-data", (GCallback)need_data, ctx);
	//g_signal_connect(appsrc, "enough-data", (GCallback)enough_data, ctx);
	gst_object_unref(appsrc);
	
	gst_object_unref(element);
}

/* This signal callback is called when appsrc needs data, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
void VideoProducer::start_need_data(GstElement * pipeline, guint size, MyContext * ctx)
{
	if (ctx->sourceId == 0)
	{
		ctx->sourceId = g_idle_add((GSourceFunc)need_data, ctx);
	}
}

/* called when we need to give data to appsrc */
void VideoProducer::need_data(GstElement * appsrc, guint unused, MyContext * ctx)
{
	GstMapInfo info;
	GstBuffer *buffer;
	GstFlowReturn ret;
	unsigned char *data;
	gint num_samples = 1;
		
	m_Mutex.lock();
	ctx->duration = ((double)num_samples / ctx->img_resolution.framerate) * GST_SECOND;
	ctx->timestamp = ctx->num_frames * ctx->duration;
	buffer = gst_buffer_new_allocate(NULL, m_JpegRawData.count(), NULL);
	gst_buffer_map(buffer, &info, (GstMapFlags)GST_MAP_READ);
	
	
	data = (guchar *)info.data;
	for (int i = 0; i < m_JpegRawData.count(); i++)
	{
			*data = m_JpegRawData.at(i);
			data++;
	}
    m_Mutex.unlock();
	GST_BUFFER_DURATION(buffer) = ctx->duration;
	GST_BUFFER_PTS(buffer) = ctx->timestamp;
	GST_BUFFER_DTS(buffer) = ctx->timestamp;
	GST_BUFFER_OFFSET(buffer) =  ctx->num_frames;

    g_signal_emit_by_name(ctx->appsrc, "push-buffer", buffer, &ret);
	gst_buffer_unmap(buffer, &info);
	
	if (ret != GST_FLOW_OK)
	{
	    // something wrong, stop pushing 
			//g_main_loop_quit (loop);
	}
	gst_buffer_unref(buffer);
	ctx->num_frames++;
	
}


void VideoProducer::enough_data(GstElement *appSrc, MyContext * ctx)
{
	if (ctx->sourceId != 0) 
	{
		g_source_remove(ctx->sourceId);
		ctx->sourceId = 0;
	}
}



void VideoProducer::SetNewJpegRawData(QByteArray &JpegRawData)
{
	m_Mutex.lock();
	m_JpegRawData = JpegRawData;
	m_Mutex.unlock();
}




void VideoProducer::constructImage()
{
	while (_bProcessing)
	{
		if (m_JpegRawData1.count()>0)
		{
			m_Mutex.lock(); 
			m_JpegRawData = m_JpegRawData1;
			m_JpegRawData1.clear();
	       	m_Mutex.unlock();
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}


long VideoProducer::ReadBuffer(QString &fileName, uint8_t *buffer)
{

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) return 0;
	QByteArray blob = file.readAll();

	buffer = (uint8_t*)calloc((size_t)blob.count(), sizeof(uint8_t));
	for (int i = 0; i < blob.count(); i++)
	{
		*buffer = blob.at(i);
	}
	
	//free(buffer);
	return blob.count();
}