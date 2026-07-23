// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomSourceSink/mysink.h
// A minimal GstBaseSink subclass that hashes (additive checksum) and counts incoming buffers.
#pragma once

#include <glib-object.h>
#include <gst/base/gstbasesink.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define MY_TYPE_SINK (my_sink_get_type())
G_DECLARE_FINAL_TYPE(MySink, my_sink, MY, SINK, GstBaseSink)

// Registers the "mysink" element with the default (process-global) plugin registry.
// Returns TRUE on success. Safe to call more than once.
gboolean my_sink_register(void);

G_END_DECLS
