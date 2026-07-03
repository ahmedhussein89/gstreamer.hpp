#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {

struct DynCtx {
  GstElement* pipeline;
  GstElement* tee;
  GstElement* branch_queue;
  GstElement* branch_sink;
  GstPad*     tee_src_pad;
  bool        branch_added{false};
};

// Probe callback: called once when the tee pad is blocked.
// Creates and links the new branch, then removes itself to resume streaming.
GstPadProbeReturn add_branch_probe(GstPad* pad, GstPadProbeInfo* /*info*/, gpointer user_data) {
  auto* ctx = static_cast<DynCtx*>(user_data);

  ctx->branch_queue = gst_element_factory_make("queue",    "branch-queue");
  ctx->branch_sink  = gst_element_factory_make("fakesink", "branch-sink");
  if(nullptr == ctx->branch_queue || nullptr == ctx->branch_sink) {
    fmt::print(stderr, "Failed to create branch elements.\n");
    return GST_PAD_PROBE_REMOVE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(ctx->branch_sink), "sync", FALSE, nullptr);

  gst_bin_add_many(GST_BIN(ctx->pipeline), ctx->branch_queue, ctx->branch_sink, nullptr);
  gst_element_link(ctx->branch_queue, ctx->branch_sink);

  GstPad* queue_sink = gst_element_get_static_pad(ctx->branch_queue, "sink");
  gst_pad_link(pad, queue_sink);
  gst_object_unref(queue_sink);

  gst_element_sync_state_with_parent(ctx->branch_queue);
  gst_element_sync_state_with_parent(ctx->branch_sink);

  ctx->branch_added = true;
  fmt::print(stdout, "[t=1s] Second branch active.\n");

  return GST_PAD_PROBE_REMOVE;
}

// Probe callback: called once when the tee pad is blocked for removal.
GstPadProbeReturn remove_branch_probe(GstPad* pad, GstPadProbeInfo* /*info*/, gpointer user_data) {
  auto* ctx = static_cast<DynCtx*>(user_data);

  GstPad* queue_sink = gst_element_get_static_pad(ctx->branch_queue, "sink");
  gst_pad_unlink(pad, queue_sink);
  gst_object_unref(queue_sink);

  gst_element_set_state(ctx->branch_sink,  GST_STATE_NULL);
  gst_element_set_state(ctx->branch_queue, GST_STATE_NULL);
  gst_bin_remove(GST_BIN(ctx->pipeline), ctx->branch_sink);
  gst_bin_remove(GST_BIN(ctx->pipeline), ctx->branch_queue);

  gst_element_release_request_pad(ctx->tee, ctx->tee_src_pad);
  gst_object_unref(ctx->tee_src_pad);
  ctx->tee_src_pad  = nullptr;
  ctx->branch_queue = nullptr;
  ctx->branch_sink  = nullptr;
  ctx->branch_added = false;
  fmt::print(stdout, "[t=3s] Second branch removed.\n");

  return GST_PAD_PROBE_REMOVE;
}

gboolean on_timer(gpointer user_data) {
  auto* ctx = static_cast<DynCtx*>(user_data);

  if(!ctx->branch_added) {
    fmt::print(stdout, "[t=1s] Adding second branch...\n");
    ctx->tee_src_pad = gst_element_request_pad_simple(ctx->tee, "src_%u");
    gst_pad_add_probe(ctx->tee_src_pad,
        static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER),
        add_branch_probe, ctx, nullptr);
  } else {
    fmt::print(stdout, "[t=3s] Removing second branch...\n");
    gst_pad_add_probe(ctx->tee_src_pad,
        static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER),
        remove_branch_probe, ctx, nullptr);
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

gboolean on_bus_msg(GstBus* /*bus*/, GstMessage* msg, GMainLoop* loop) {
  switch(GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::print(stderr, "Error: {}\n", err->message);
      g_error_free(err);
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_EOS:
      fmt::print(stdout, "End of stream reached.\n");
      g_main_loop_quit(loop);
      break;
    default:
      break;
  }
  return TRUE;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("dynamic-pipeline");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* tee      = gst_element_factory_make("tee",          "tee");
  auto* queue_a  = gst_element_factory_make("queue",        "queue-a");
  auto* sink_a   = gst_element_factory_make("autovideosink","sink-a");

  if(nullptr == pipeline || nullptr == source || nullptr == convert ||
     nullptr == tee || nullptr == queue_a || nullptr == sink_a) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", 200, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, convert, tee, queue_a, sink_a, nullptr);
  if(TRUE != gst_element_link_many(source, convert, tee, nullptr)) {
    fmt::print(stderr, "Failed to link source chain.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  GstPad* tee_src_a  = gst_element_request_pad_simple(tee, "src_%u");
  GstPad* queue_sink = gst_element_get_static_pad(queue_a, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_src_a, queue_sink)) {
    fmt::print(stderr, "Failed to link tee to display queue.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  gst_object_unref(queue_sink);

  if(TRUE != gst_element_link(queue_a, sink_a)) {
    fmt::print(stderr, "Failed to link display branch.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Pipeline running...\n");

  auto* loop = g_main_loop_new(nullptr, FALSE);
  auto* bus  = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, reinterpret_cast<GstBusFunc>(on_bus_msg), loop);
  gst_object_unref(bus);

  DynCtx ctx{pipeline, tee, nullptr, nullptr, nullptr};
  // Add branch at t=1s, remove at t=3s.
  g_timeout_add_seconds(1, on_timer, &ctx);
  g_timeout_add_seconds(3, on_timer, &ctx);

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_element_release_request_pad(tee, tee_src_a);
  gst_object_unref(tee_src_a);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
