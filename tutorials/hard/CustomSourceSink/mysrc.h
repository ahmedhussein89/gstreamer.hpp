// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomSourceSink/mysrc.h
// A minimal GstBaseSrc subclass producing a synthetic counting-pattern video stream.
#pragma once

#include <glib-object.h>
#include <gst/base/gstbasesrc.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define MY_TYPE_SRC (my_src_get_type())
G_DECLARE_FINAL_TYPE(MySrc, my_src, MY, SRC, GstBaseSrc)

// Registers the "mysrc" element with the default (process-global) plugin registry.
// Returns TRUE on success. Safe to call more than once.
gboolean my_src_register(void);

G_END_DECLS
