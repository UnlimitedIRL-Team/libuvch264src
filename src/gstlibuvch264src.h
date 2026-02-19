#ifndef GST_LIBUVC_H264_SRC_H
#define GST_LIBUVC_H264_SRC_H

#include <glib.h>
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <libuvc/libuvc.h>

G_BEGIN_DECLS

#define GST_TYPE_LIBUVC_H264_SRC (gst_libuvc_h264_src_get_type())
G_DECLARE_FINAL_TYPE(GstLibuvcH264Src, gst_libuvc_h264_src, GST, LIBUVC_H264_SRC, GstPushSrc)

#define DEFAULT_DEVICE_INDEX "0"
#define TIMEOUT_DURATION G_TIME_SPAN_SECOND // 1 second
#define DJI_VENDOR_ID 0x2ca3
#define DJI_PRODUCT_ID 0x0023

#define SPSPPSBUFSZ 1024

#define MIN_FRAMES_CALC_INTERVAL 60

struct _GstLibuvcH264Src {
  GstPushSrc parent_instance;
  gchar* index;
  uvc_context_t *uvc_ctx;
  uvc_device_t *uvc_dev;
  uvc_device_handle_t *uvc_devh;
  uvc_stream_ctrl_t uvc_ctrl;
  GAsyncQueue *frame_queue;
  gboolean streaming;
  GstClockTime uvc_start_time;
  GstClockTime prev_pts;
  gint64 frame_interval; // in ns
  guint64 prev_int_ts;
  gint frame_count;
  gboolean had_idr;
  gboolean send_sps_pps;
  gint sps_length;
  gint pps_length;
  unsigned char sps[SPSPPSBUFSZ];
  unsigned char pps[SPSPPSBUFSZ];
};

G_END_DECLS

#endif /* GST_LIBUVC_H264_SRC_H */
