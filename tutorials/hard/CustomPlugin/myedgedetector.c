// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomPlugin/myedgedetector.c
#include "myedgedetector.h"

#include <stdlib.h>

struct _MyEdgeDetector {
  GstBaseTransform parent;
  gint             width;
  gint             height;
};

G_DEFINE_TYPE(MyEdgeDetector, my_edge_detector, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw,format=GRAY8"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw,format=GRAY8"));

static gboolean my_edge_detector_set_caps(GstBaseTransform* trans, GstCaps* incaps, GstCaps* outcaps) {
  (void)incaps;
  MyEdgeDetector* self      = MY_EDGE_DETECTOR(trans);
  GstStructure*   structure = gst_caps_get_structure(outcaps, 0);
  return gst_structure_get_int(structure, "width", &self->width) &&
         gst_structure_get_int(structure, "height", &self->height);
}

// Horizontal-difference edge detector: out[x] = 255 if |in[x] - in[x-1]| > threshold else 0.
// Processed right-to-left so each source pixel is read before it is overwritten, which keeps
// the transform correct in place without a second buffer.
// ponytail: assumes stride == width (no row padding); use gst_video_frame_map/GstVideoInfo for
// formats/resolutions where GRAY8 rows are padded.
static GstFlowReturn my_edge_detector_transform_ip(GstBaseTransform* trans, GstBuffer* buf) {
  MyEdgeDetector* self = MY_EDGE_DETECTOR(trans);
  GstMapInfo      map;

  if(0 == self->width || 0 == self->height) {
    return GST_FLOW_OK;
  }

  if(!gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
    return GST_FLOW_ERROR;
  }

  // The caps say how big a frame is; the buffer is what upstream actually handed us.
  // Never index past the mapping if they disagree.
  if(map.size < (gsize)self->width * (gsize)self->height) {
    GST_ERROR_OBJECT(trans, "buffer of %" G_GSIZE_FORMAT " bytes is smaller than the negotiated %dx%d frame",
        map.size, self->width, self->height);
    gst_buffer_unmap(buf, &map);
    return GST_FLOW_ERROR;
  }

  const gint width     = self->width;
  const gint threshold = 25;
  for(gint row = 0; row < self->height; ++row) {
    guint8* line = map.data + (gsize)row * (gsize)width;
    for(gint col = width - 1; col >= 1; --col) {
      const gint diff = abs((gint)line[col] - (gint)line[col - 1]);
      line[col]       = (diff > threshold) ? 255 : 0;
    }
    line[0] = 0;
  }

  gst_buffer_unmap(buf, &map);
  return GST_FLOW_OK;
}

static void my_edge_detector_class_init(MyEdgeDetectorClass* klass) {
  GstElementClass*       element_class   = GST_ELEMENT_CLASS(klass);
  GstBaseTransformClass* transform_class = GST_BASE_TRANSFORM_CLASS(klass);

  gst_element_class_set_static_metadata(element_class, "MyEdgeDetector", "Filter/Effect/Video",
      "Simple horizontal-difference edge detector on GRAY8 video", "gstreamer.hpp tutorials");

  gst_element_class_add_static_pad_template(element_class, &src_template);
  gst_element_class_add_static_pad_template(element_class, &sink_template);

  transform_class->set_caps      = GST_DEBUG_FUNCPTR(my_edge_detector_set_caps);
  transform_class->transform_ip  = GST_DEBUG_FUNCPTR(my_edge_detector_transform_ip);
}

static void my_edge_detector_init(MyEdgeDetector* self) {
  self->width  = 0;
  self->height = 0;
}

gboolean my_edge_detector_register(void) {
  return gst_element_register(NULL, MY_EDGE_DETECTOR_NAME, GST_RANK_NONE, MY_TYPE_EDGE_DETECTOR);
}
