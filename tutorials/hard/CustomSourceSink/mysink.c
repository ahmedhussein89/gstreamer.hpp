// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomSourceSink/mysink.c
// A minimal GstBaseSink subclass that hashes (additive checksum) and counts incoming buffers.
#include "mysink.h"

#include <stdio.h>

struct _MySink {
  GstBaseSink parent_instance;

  guint64 checksum;
  guint64 count;
};

G_DEFINE_TYPE(MySink, my_sink, GST_TYPE_BASE_SINK)

static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static gboolean my_sink_start(GstBaseSink* bsink);
static gboolean my_sink_stop(GstBaseSink* bsink);
static GstFlowReturn my_sink_render(GstBaseSink* bsink, GstBuffer* buf);

static void my_sink_class_init(MySinkClass* klass) {
  GstElementClass*  element_class  = GST_ELEMENT_CLASS(klass);
  GstBaseSinkClass* base_sink_class = GST_BASE_SINK_CLASS(klass);

  gst_element_class_set_static_metadata(element_class, "My Checksum Sink", "Sink",
      "Sums and counts incoming buffers", "gstreamer.hpp tutorials");
  gst_element_class_add_static_pad_template(element_class, &sink_template);

  base_sink_class->start  = my_sink_start;
  base_sink_class->stop   = my_sink_stop;
  base_sink_class->render = my_sink_render;
}

static void my_sink_init(MySink* self) {
  self->checksum = 0;
  self->count    = 0;
}

static gboolean my_sink_start(GstBaseSink* bsink) {
  MySink* self  = MY_SINK(bsink);
  self->checksum = 0;
  self->count    = 0;
  return TRUE;
}

static gboolean my_sink_stop(GstBaseSink* bsink) {
  MySink* self = MY_SINK(bsink);
  fprintf(stdout, "mysink: received %" G_GUINT64_FORMAT " buffers, checksum=%" G_GUINT64_FORMAT "\n", self->count,
      self->checksum);
  return TRUE;
}

static GstFlowReturn my_sink_render(GstBaseSink* bsink, GstBuffer* buf) {
  MySink* self = MY_SINK(bsink);

  GstMapInfo map;
  if(!gst_buffer_map(buf, &map, GST_MAP_READ)) {
    return GST_FLOW_ERROR;
  }
  for(gsize i = 0; i < map.size; ++i) {
    self->checksum += map.data[i];
  }
  gst_buffer_unmap(buf, &map);

  self->count++;
  return GST_FLOW_OK;
}

gboolean my_sink_register(void) {
  return gst_element_register(NULL, "mysink", GST_RANK_NONE, MY_TYPE_SINK);
}
