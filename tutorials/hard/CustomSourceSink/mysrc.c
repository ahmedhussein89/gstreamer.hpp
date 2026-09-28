// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomSourceSink/mysrc.c
// A minimal GstBaseSrc subclass producing a synthetic counting-pattern video stream.
//
// Every produced buffer is filled with a single repeating byte value equal to
// (buffers_sent % 256) -- lazy but sufficient to prove data actually flows and changes
// from buffer to buffer. Caps negotiation picks width/height/framerate via `fixate`.
#include "mysrc.h"

#include <string.h>

struct _MySrc {
  GstBaseSrc parent_instance;

  gint    width;
  gint    height;
  gint    num_buffers;
  guint64 buffers_sent;
  gsize   frame_size;
  gint    fps_n;
  gint    fps_d;
};

G_DEFINE_TYPE(MySrc, my_src, GST_TYPE_BASE_SRC)

enum {
  PROP_0,
  PROP_NUM_BUFFERS,
  PROP_WIDTH,
  PROP_HEIGHT,
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, format=(string)GRAY8, "
                     "width=(int)[1,2147483647], height=(int)[1,2147483647], "
                     "framerate=(fraction)[0/1,2147483647/1]"));

static void my_src_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void my_src_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);
static gboolean  my_src_start(GstBaseSrc* bsrc);
static gboolean  my_src_stop(GstBaseSrc* bsrc);
static gboolean  my_src_set_caps(GstBaseSrc* bsrc, GstCaps* caps);
static GstCaps*  my_src_fixate(GstBaseSrc* bsrc, GstCaps* caps);
static gboolean  my_src_is_seekable(GstBaseSrc* bsrc);
static GstFlowReturn my_src_fill(GstBaseSrc* bsrc, guint64 offset, guint size, GstBuffer* buf);

static void my_src_class_init(MySrcClass* klass) {
  GObjectClass*     gobject_class   = G_OBJECT_CLASS(klass);
  GstElementClass*  element_class   = GST_ELEMENT_CLASS(klass);
  GstBaseSrcClass*  base_src_class  = GST_BASE_SRC_CLASS(klass);

  gobject_class->set_property = my_src_set_property;
  gobject_class->get_property = my_src_get_property;

  // -1 means "never stop", matching videotestsrc/filesrc; anything >= 0 is a hard buffer budget.
  g_object_class_install_property(gobject_class, PROP_NUM_BUFFERS,
      g_param_spec_int("num-buffers", "Num Buffers", "Number of buffers to output before sending EOS (-1 = unlimited)",
          -1, G_MAXINT, 100, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property(gobject_class, PROP_WIDTH,
      g_param_spec_int("width", "Width", "Frame width in pixels", 1, G_MAXINT, 64,
          (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property(gobject_class, PROP_HEIGHT,
      g_param_spec_int("height", "Height", "Frame height in pixels", 1, G_MAXINT, 64,
          (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata(element_class, "My Pattern Source", "Source",
      "Synthetic counting-pattern GRAY8 video source", "gstreamer.hpp tutorials");
  gst_element_class_add_static_pad_template(element_class, &src_template);

  base_src_class->start       = my_src_start;
  base_src_class->stop        = my_src_stop;
  base_src_class->set_caps    = my_src_set_caps;
  base_src_class->fixate      = my_src_fixate;
  base_src_class->is_seekable = my_src_is_seekable;
  base_src_class->fill        = my_src_fill;
}

static void my_src_init(MySrc* self) {
  self->width        = 64;
  self->height       = 64;
  self->num_buffers  = 100;
  self->buffers_sent = 0;
  self->frame_size   = 0;
  self->fps_n        = 25;
  self->fps_d        = 1;
  gst_base_src_set_format(GST_BASE_SRC(self), GST_FORMAT_TIME);
}

static void my_src_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  MySrc* self = MY_SRC(object);
  switch(prop_id) {
    case PROP_NUM_BUFFERS: self->num_buffers = g_value_get_int(value); break;
    case PROP_WIDTH: self->width = g_value_get_int(value); break;
    case PROP_HEIGHT: self->height = g_value_get_int(value); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
  }
}

static void my_src_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  MySrc* self = MY_SRC(object);
  switch(prop_id) {
    case PROP_NUM_BUFFERS: g_value_set_int(value, self->num_buffers); break;
    case PROP_WIDTH: g_value_set_int(value, self->width); break;
    case PROP_HEIGHT: g_value_set_int(value, self->height); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
  }
}

static gboolean my_src_start(GstBaseSrc* bsrc) {
  MY_SRC(bsrc)->buffers_sent = 0;
  return TRUE;
}

static gboolean my_src_stop(GstBaseSrc* bsrc) {
  (void)bsrc;
  return TRUE;
}

static gboolean my_src_is_seekable(GstBaseSrc* bsrc) {
  (void)bsrc;
  return FALSE;
}

// Picks concrete width/height/framerate for the negotiated caps, preferring this
// element's configured `width`/`height` properties, then lets the base class
// fixate any remaining (non-numeric) fields such as `format`.
static GstCaps* my_src_fixate(GstBaseSrc* bsrc, GstCaps* caps) {
  MySrc* self = MY_SRC(bsrc);

  caps = gst_caps_make_writable(caps);
  GstStructure* structure = gst_caps_get_structure(caps, 0);
  gst_structure_fixate_field_nearest_int(structure, "width", self->width);
  gst_structure_fixate_field_nearest_int(structure, "height", self->height);
  gst_structure_fixate_field_nearest_fraction(structure, "framerate", 25, 1);

  return GST_BASE_SRC_CLASS(my_src_parent_class)->fixate(bsrc, caps);
}

static gboolean my_src_set_caps(GstBaseSrc* bsrc, GstCaps* caps) {
  MySrc*        self      = MY_SRC(bsrc);
  GstStructure* structure = gst_caps_get_structure(caps, 0);

  gint width  = 0;
  gint height = 0;
  if(!gst_structure_get_int(structure, "width", &width) || !gst_structure_get_int(structure, "height", &height)) {
    return FALSE;
  }

  gint fps_n = 0;
  gint fps_d = 1;
  if(gst_structure_get_fraction(structure, "framerate", &fps_n, &fps_d) && fps_n > 0) {
    self->fps_n = fps_n;
    self->fps_d = fps_d;
  }

  self->width      = width;
  self->height     = height;
  self->frame_size = (gsize)width * (gsize)height;
  gst_base_src_set_blocksize(bsrc, (guint)self->frame_size);
  return TRUE;
}

static GstFlowReturn my_src_fill(GstBaseSrc* bsrc, guint64 offset, guint size, GstBuffer* buf) {
  (void)offset;
  (void)size;
  MySrc* self = MY_SRC(bsrc);

  if(self->num_buffers >= 0 && self->buffers_sent >= (guint64)self->num_buffers) {
    return GST_FLOW_EOS;
  }

  GstMapInfo map;
  if(!gst_buffer_map(buf, &map, GST_MAP_WRITE)) {
    return GST_FLOW_ERROR;
  }
  memset(map.data, (guint8)(self->buffers_sent % 256), map.size);
  gst_buffer_unmap(buf, &map);

  // GstBaseSrc does not timestamp for you unless `do-timestamp` is set. This element declares
  // GST_FORMAT_TIME, so it owes downstream a PTS and duration derived from the negotiated framerate.
  const GstClockTime duration = gst_util_uint64_scale_int(GST_SECOND, self->fps_d, self->fps_n);
  GST_BUFFER_PTS(buf)         = gst_util_uint64_scale(self->buffers_sent, GST_SECOND * (guint64)self->fps_d,
                                                      (guint64)self->fps_n);
  GST_BUFFER_DTS(buf)         = GST_BUFFER_PTS(buf);
  GST_BUFFER_DURATION(buf)    = duration;
  GST_BUFFER_OFFSET(buf)      = self->buffers_sent;
  GST_BUFFER_OFFSET_END(buf)  = self->buffers_sent + 1;
  self->buffers_sent++;

  return GST_FLOW_OK;
}

gboolean my_src_register(void) {
  return gst_element_register(NULL, "mysrc", GST_RANK_NONE, MY_TYPE_SRC);
}
