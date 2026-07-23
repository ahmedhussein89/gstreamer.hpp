// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomPlugin/myedgedetector.h
#ifndef GSTREAMER_HPP_TUTORIALS_HARD_CUSTOMPLUGIN_MYEDGEDETECTOR_H
#define GSTREAMER_HPP_TUTORIALS_HARD_CUSTOMPLUGIN_MYEDGEDETECTOR_H

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define MY_TYPE_EDGE_DETECTOR (my_edge_detector_get_type())
G_DECLARE_FINAL_TYPE(MyEdgeDetector, my_edge_detector, MY, EDGE_DETECTOR, GstBaseTransform)

// Element name used with gst_element_factory_make() / gst_element_register().
#define MY_EDGE_DETECTOR_NAME "myedgedetector"

// Registers MyEdgeDetector with the default GstRegistry. Safe to call more than
// once (gst_element_register() itself is idempotent for the caller's purposes here).
// Returns TRUE on success.
gboolean my_edge_detector_register(void);

G_END_DECLS

#endif    // GSTREAMER_HPP_TUTORIALS_HARD_CUSTOMPLUGIN_MYEDGEDETECTOR_H
