#include <sys/stat.h>
#include <libusb-1.0/libusb.h>
#include "gstlibuvch264src.h"
#include <gst/gst.h>
#include <libuvc/libuvc.h>

GST_DEBUG_CATEGORY_STATIC(gst_libuvc_h264_src_debug);
#define GST_CAT_DEFAULT gst_libuvc_h264_src_debug

enum {
  PROP_0,
  PROP_INDEX,
  PROP_LAST
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS("video/x-h264, "
                  "stream-format=(string)byte-stream, "
                  "alignment=(string)au")
);

G_DEFINE_TYPE_WITH_CODE(GstLibuvcH264Src, gst_libuvc_h264_src, GST_TYPE_PUSH_SRC,
  GST_DEBUG_CATEGORY_INIT(gst_libuvc_h264_src_debug, "libuvch264src", 0, "libuvch264src element"));

static gboolean gst_libuvc_h264_src_set_caps(GstBaseSrc *basesrc, GstCaps *caps);
static void gst_libuvc_h264_src_set_property(GObject *object, guint prop_id,
                                             const GValue *value, GParamSpec *pspec);
static void gst_libuvc_h264_src_get_property(GObject *object, guint prop_id,
                                             GValue *value, GParamSpec *pspec);
static gboolean gst_libuvc_h264_src_start(GstBaseSrc *src);
static gboolean gst_libuvc_h264_src_stop(GstBaseSrc *src);
static GstFlowReturn gst_libuvc_h264_src_create(GstPushSrc *src, GstBuffer **buf);
static void gst_libuvc_h264_src_finalize(GObject *object);

static void gst_libuvc_h264_src_class_init(GstLibuvcH264SrcClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS(klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS(klass);

  base_src_class->set_caps = GST_DEBUG_FUNCPTR(gst_libuvc_h264_src_set_caps);
  gobject_class->set_property = gst_libuvc_h264_src_set_property;
  gobject_class->get_property = gst_libuvc_h264_src_get_property;

  g_object_class_install_property(gobject_class, PROP_INDEX,
    g_param_spec_string("index", "Index", "Device location, e.g., '0'",
                        DEFAULT_DEVICE_INDEX, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata(element_class,
    "UVC H.264 Video Source", "Source/Video",
    "Captures H.264 video from a UVC device", "Name");

  gst_element_class_add_pad_template(element_class,
    gst_static_pad_template_get(&src_template));

  base_src_class->start = gst_libuvc_h264_src_start;
  base_src_class->stop = gst_libuvc_h264_src_stop;
  push_src_class->create = gst_libuvc_h264_src_create;
  gobject_class->finalize = gst_libuvc_h264_src_finalize;
}

void create_hidden_directory(GstLibuvcH264Src *self) {
	const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        GST_WARNING_OBJECT(self, "Warning: HOME environment variable not set.");
        home_dir = "";
    }
	
    // Construct the path to the hidden directory
    char hidden_dir[255];
	sprintf(hidden_dir, "%s/.spspps/", home_dir);

    // Check if the directory exists
    struct stat st;
    if (stat(hidden_dir, &st) == -1) {
        // Directory does not exist; create it
        if (mkdir(hidden_dir, 0700) != 0)
            GST_ERROR_OBJECT(self, "Error creating directory %s\n", hidden_dir);
        else
            GST_WARNING_OBJECT(self, "Directory %s created successfully.\n", hidden_dir);
    } else if (!S_ISDIR(st.st_mode))
        // Path exists but is not a directory
        GST_WARNING_OBJECT(self, "Warning: %s exists but is not a directory.\n", hidden_dir);
}

void load_spspps(GstLibuvcH264Src *self) {
	create_hidden_directory(self);
	
	const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        GST_WARNING_OBJECT(self, "Warning: HOME environment variable not set.");
        home_dir = "";
    }
	
	char file_name[255] = { 0 };
	sprintf(file_name, "%s/.spspps/%s", home_dir, self->index);
	
	FILE* fp = fopen(file_name, "rb");
	if (fp) {
		gint read_bytes = fread(self->spspps, 1, 1024, fp);
		self->spspps_length = read_bytes;
		fclose(fp);
	}
	else {
		fp = fopen(file_name, "wb");
		if (fp) {
			fwrite(self->spspps, 1, self->spspps_length, fp);
			fclose(fp);
		}
	}
}

void store_spspps(GstLibuvcH264Src *self, gchar* spspps, gint spspps_length, gint nal_type) {
	const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        GST_WARNING_OBJECT(self, "Warning: HOME environment variable not set.");
        home_dir = "";
    }
	
	char file_name[255] = { 0 };
	sprintf(file_name, "%s/.spspps/%s", home_dir, self->index);
	
	FILE* fp = NULL;
	
	if (nal_type == 7)
		fp = fopen(file_name, "wb");
	else if (nal_type == 8)
		fp = fopen(file_name, "ab");
	
	if (fp) {
		fwrite(spspps, 1, spspps_length, fp);
		fclose(fp);
	}
	
	// check PPS (integrated sps/pps)
	if (nal_type == 7) {
		int i;
		for (i = 0; i < spspps_length - 4; i++) {
			if (spspps[i] == 0 && spspps[i + 1] == 0 && spspps[i + 2] == 0 && spspps[i + 3] == 1 && (spspps[i + 4] & 0x1F) == 8) {
				nal_type = 8;
				break;
			}
		}
	}
	
	if (nal_type == 8)
		load_spspps(self);
}

static void gst_libuvc_h264_src_init(GstLibuvcH264Src *self) {
  self->index = g_strdup(DEFAULT_DEVICE_INDEX);
  self->uvc_ctx = NULL;
  self->uvc_dev = NULL;
  self->uvc_devh = NULL;
  self->frame_queue = g_async_queue_new();
  self->streaming = FALSE;
  self->width = DEFAULT_WIDTH;
  self->height = DEFAULT_HEIGHT;
  self->framerate = DEFAULT_FRAMERATE;
  self->frame_count = 0; // Added to track frames for timestamping
  // Initialization, not fixed
  self->spspps_length = 39;
  gchar spspps[] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x34, 0xAC, 0x4D, 0x00, 0xA0, 0x0B, 0x74, 0xD4, 0x04, 0x04, 0x05, 0x00, 0x00, 0x03, 0x03, 0xE8, 0x00, 0x00, 0xEA, 0x60, 0x0F, 0x1C, 0x32, 0xA0, 0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0xB0 };
  memcpy(self->spspps, spspps, self->spspps_length);
  
  gst_base_src_set_live(GST_BASE_SRC(self), TRUE);
  gst_base_src_set_format(GST_BASE_SRC(self), GST_FORMAT_TIME);
}

static gboolean gst_libuvc_h264_src_set_caps(GstBaseSrc *basesrc, GstCaps *caps) {
    GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(basesrc);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    if (gst_structure_has_name(structure, "video/x-h264")) {
        gint width, height, framerate_num, framerate_den;

        if (gst_structure_get_int(structure, "width", &width))
            self->width = width;
        if (gst_structure_get_int(structure, "height", &height))
            self->height = height;
        if (gst_structure_get_fraction(structure, "framerate", &framerate_num, &framerate_den) && framerate_den != 0)
            self->framerate = framerate_num / framerate_den;

        GST_INFO("Set caps: width=%d, height=%d, framerate=%d", self->width, self->height, self->framerate);
    } else {
        GST_ERROR("Unsupported caps: %s", gst_structure_get_name(structure));
        return FALSE;
    }

    return TRUE;
}

static void gst_libuvc_h264_src_set_property(GObject *object, guint prop_id,
                                             const GValue *value, GParamSpec *pspec) {
  GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(object);

  switch (prop_id) {
    case PROP_INDEX:
      g_free(self->index);
      self->index = g_value_dup_string(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void gst_libuvc_h264_src_get_property(GObject *object, guint prop_id,
                                             GValue *value, GParamSpec *pspec) {
  GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(object);

  switch (prop_id) {
    case PROP_INDEX:
      g_value_set_string(value, self->index);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static gboolean gst_libuvc_h264_src_start(GstBaseSrc *src) {
  GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(src);
  uvc_error_t res;

  // Initialize libuvc context
  res = uvc_init(&self->uvc_ctx, NULL);
  if (res < 0) {
    GST_ERROR_OBJECT(self, "Failed to initialize libuvc: %s", uvc_strerror(res));
    return FALSE;
  }

  /*
  // Find the 1st UVC device
  res = uvc_find_device(self->uvc_ctx, &self->uvc_dev, 0, 0, NULL);
  if (res < 0) {
    GST_ERROR_OBJECT(self, "Unable to find UVC device: %s", uvc_strerror(res));
    uvc_exit(self->uvc_ctx);
    return FALSE;
  }
  */
  
  uvc_device_t **dev_list;
  //res = uvc_find_devices(self->uvc_ctx, &dev_list, 0, 0, NULL);
  res = uvc_find_devices(self->uvc_ctx, &dev_list, DJI_VENDOR_ID, DJI_PRODUCT_ID, NULL);
  if (res < 0) {
    GST_ERROR_OBJECT(self, "Unable to find any UVC devices");
    uvc_exit(self->uvc_ctx);
    return FALSE;
  }

  for (int i = 0; dev_list[i] != NULL; ++i) {
    uvc_device_t *dev = dev_list[i];
	if (i == atoi(self->index)) {
		self->uvc_dev = dev;
		break;
	}
  }
  
  if (!self->uvc_dev) {
    GST_ERROR_OBJECT(self, "Unable to find UVC device: %s", self->index);
    uvc_exit(self->uvc_ctx);
    return FALSE;
  }

  // Open the UVC device
  res = uvc_open(self->uvc_dev, &self->uvc_devh);
  if (res < 0) {
    GST_ERROR_OBJECT(self, "Unable to open UVC device: %s", uvc_strerror(res));
    uvc_unref_device(self->uvc_dev);
    uvc_exit(self->uvc_ctx);
    return FALSE;
  }
  
  // Enumerate and log supported formats and frame sizes
  const uvc_format_desc_t *format_desc = uvc_get_format_descs(self->uvc_devh);
  while (format_desc) {
      GST_INFO("Found format: %s", format_desc->guidFormat);
      const uvc_frame_desc_t *frame_desc = format_desc->frame_descs;
      while (frame_desc) {
          GST_INFO("  Frame size: %ux%u", frame_desc->wWidth, frame_desc->wHeight);
		  if (frame_desc->intervals) {
            const uint32_t *interval = frame_desc->intervals;
            while (*interval) {
                double fps = 1.0 / (*interval * 1e-7);
                GST_INFO("    Frame rate: %.2f fps", fps);
                interval++;
            }
		  }
		  else {
			GST_INFO("    Frame rate range: %.2f - %.2f fps",
					 1.0 / (frame_desc->dwMaxFrameInterval * 1e-7),
					 1.0 / (frame_desc->dwMinFrameInterval * 1e-7));
		  }
		  
          frame_desc = frame_desc->next;
      }
	  
      format_desc = format_desc->next;
  }

  res = uvc_get_stream_ctrl_format_size(self->uvc_devh, &self->uvc_ctrl,
                                        UVC_FRAME_FORMAT_H264, self->width, self->height, self->framerate);
  if (res < 0) {
    GST_ERROR_OBJECT(self, "Unable to get stream control: %s", uvc_strerror(res));
    uvc_close(self->uvc_devh);
    uvc_unref_device(self->uvc_dev);
    uvc_exit(self->uvc_ctx);
    return FALSE;
  }
  
  load_spspps(self);

  return TRUE;
}

static gboolean gst_libuvc_h264_src_stop(GstBaseSrc *src) {
  GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(src);

  if (self->streaming) {
    uvc_stop_streaming(self->uvc_devh);
    self->streaming = FALSE;
  }

  if (self->uvc_devh) {
    uvc_close(self->uvc_devh);
    self->uvc_devh = NULL;
  }

  if (self->uvc_dev) {
    uvc_unref_device(self->uvc_dev);
    self->uvc_dev = NULL;
  }

  if (self->uvc_ctx) {
    uvc_exit(self->uvc_ctx);
    self->uvc_ctx = NULL;
  }

  return TRUE;
}

// Callback to handle frame data
void frame_callback(uvc_frame_t *frame, void *ptr) {
    GstLibuvcH264Src *self = (GstLibuvcH264Src *)ptr;

    if (!frame || !frame->data || frame->data_bytes == 0) {
        GST_WARNING_OBJECT(self, "Empty or invalid frame received.");
        return;
    }
	
	unsigned char* data = frame->data;
	int nal_type = (data[4] & 0x1F);
	
	// Parse SPS/PPS
	if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1) {
		if (nal_type == 7 || nal_type == 8)
			store_spspps(self, frame->data, frame->data_bytes, nal_type);
	}
	
	GstBuffer *buffer = NULL;
	if (nal_type == 5) {
		gint new_bytes = frame->data_bytes + self->spspps_length;
		buffer = gst_buffer_new_allocate(NULL, new_bytes, NULL);
		gst_buffer_fill(buffer, 0, self->spspps, self->spspps_length);
		gst_buffer_fill(buffer, self->spspps_length, frame->data, frame->data_bytes);
		
		if (!self->had_idr)
			self->had_idr = TRUE;
	} else if (!self->had_idr) {
		return;
	}
	else {
		buffer = gst_buffer_new_allocate(NULL, frame->data_bytes, NULL);
		gst_buffer_fill(buffer, 0, frame->data, frame->data_bytes);
	}
	
	// Set timestamps on the buffer
    GstClockTime timestamp = gst_util_uint64_scale(self->frame_count * GST_SECOND, 1, self->framerate);
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, self->framerate);

    self->frame_count++;

	// Push the buffer onto the queue
    g_async_queue_push(self->frame_queue, buffer);
}

static GstFlowReturn gst_libuvc_h264_src_create(GstPushSrc *src, GstBuffer **buf) {
  GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(src);
  uvc_error_t res;

  if (!self->streaming) {
    // Start streaming
    res = uvc_start_streaming(self->uvc_devh, &self->uvc_ctrl, frame_callback, self, 0);
    if (res < 0) {
      GST_ERROR_OBJECT(self, "Unable to start streaming: %s", uvc_strerror(res));
      return GST_FLOW_ERROR;
    }
    self->streaming = TRUE;
	self->frame_count = 0; // Initialize frame count for timestamping
  }

  // Retrieve a buffer from the queue
  /*
  *buf = g_async_queue_timeout_pop(self->frame_queue, TIMEOUT_DURATION);
  if (*buf == NULL) {
    GST_ERROR_OBJECT(self, "No frame available within the timeout period.");
    return GST_FLOW_OK;
  }
  */
  *buf = g_async_queue_pop(self->frame_queue);
  if (*buf == NULL) {
    GST_ERROR_OBJECT(self, "No frame available.");
    return GST_FLOW_ERROR;
  }

  return GST_FLOW_OK;
}

static void gst_libuvc_h264_src_finalize(GObject *object) {
    GstLibuvcH264Src *self = GST_LIBUVC_H264_SRC(object);

    // Ensure streaming is stopped
    if (self->streaming) {
        uvc_stop_streaming(self->uvc_devh);
        self->streaming = FALSE;
    }

    // Unreference and free the frame queue
    if (self->frame_queue) {
        g_async_queue_unref(self->frame_queue);
        self->frame_queue = NULL;
    }

    // Chain up to the parent class
    G_OBJECT_CLASS(gst_libuvc_h264_src_parent_class)->finalize(object);
}

// Plugin initialization function
static gboolean plugin_init(GstPlugin *plugin) {
    // Register your element
    return gst_element_register(plugin, "libuvch264src", GST_RANK_NONE, GST_TYPE_LIBUVC_H264_SRC);
}

// Define the plugin using GST_PLUGIN_DEFINE
#define PACKAGE "libuvch264src"
#define VERSION "1.0"
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    libuvch264src,
    "UVC H264 Source Plugin",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "https://gstreamer.freedesktop.org/"
)
