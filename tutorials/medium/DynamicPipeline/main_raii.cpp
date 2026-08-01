#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer_raii.hpp"

namespace {

struct DynCtx {
  GstElement* pipeline;
  GstElement* tee;
  GstElement* branch_queue{nullptr};
  GstElement* branch_sink{nullptr};
  GstPad*     tee_src_pad{nullptr};
  bool        branch_added{false};
};

gst::PadProbeReturn add_branch_probe(gst::Pad pad, GstPadProbeInfo* /*info*/, DynCtx* ctx) {
  auto bq = gst::raii::element_factory_make("queue",    "branch-queue");
  auto bs = gst::raii::element_factory_make("fakesink", "branch-sink");
  if(!bq || !bs) {
    fmt::print(stderr, "Failed to create branch elements.\n");
    return gst::PadProbeReturn::Remove;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(bs->get()), "sync", FALSE, nullptr);

  ctx->branch_queue = bq->release();
  ctx->branch_sink  = bs->release();
  if(auto r = gst::bin_add_many(gst::Pipeline{ctx->pipeline},
        {gst::Element{ctx->branch_queue}, gst::Element{ctx->branch_sink}}); !r) {
    fmt::print(stderr, "Failed to add branch elements: {}\n", r.error());
    return gst::PadProbeReturn::Remove;
  }
  std::ignore = gst::element_link(gst::Element{ctx->branch_queue}, gst::Element{ctx->branch_sink});

  auto queue_sink = gst::element_get_static_pad(gst::Element{ctx->branch_queue}, "sink");
  if(!queue_sink) { return gst::PadProbeReturn::Remove; }
  std::ignore = gst::pad_link(pad, gst::Pad{queue_sink->get()});

  std::ignore = gst::element_sync_state_with_parent(gst::Element{ctx->branch_queue});
  std::ignore = gst::element_sync_state_with_parent(gst::Element{ctx->branch_sink});

  ctx->branch_added = true;
  fmt::print(stdout, "[t=1s] Second branch active.\n");
  return gst::PadProbeReturn::Remove;
}

gst::PadProbeReturn remove_branch_probe(gst::Pad pad, GstPadProbeInfo* /*info*/, DynCtx* ctx) {
  auto queue_sink = gst::element_get_static_pad(gst::Element{ctx->branch_queue}, "sink");
  if(queue_sink) { std::ignore = gst::pad_unlink(pad, gst::Pad{queue_sink->get()}); }

  std::ignore = gst::element_set_state(gst::Element{ctx->branch_sink},  GST_STATE_NULL);
  std::ignore = gst::element_set_state(gst::Element{ctx->branch_queue}, GST_STATE_NULL);
  std::ignore = gst::bin_remove(gst::Pipeline{ctx->pipeline}, gst::Element{ctx->branch_sink});
  std::ignore = gst::bin_remove(gst::Pipeline{ctx->pipeline}, gst::Element{ctx->branch_queue});

  gst::element_release_request_pad(gst::Element{ctx->tee}, gst::Pad{ctx->tee_src_pad});
  { gst::PadPtr p{ctx->tee_src_pad}; }    // unrefs
  ctx->tee_src_pad  = nullptr;
  ctx->branch_queue = nullptr;
  ctx->branch_sink  = nullptr;
  ctx->branch_added = false;
  fmt::print(stdout, "[t=3s] Second branch removed.\n");
  return gst::PadProbeReturn::Remove;
}

gboolean on_timer(gpointer user_data) {
  auto* ctx = static_cast<DynCtx*>(user_data);
  if(!ctx->branch_added) {
    fmt::print(stdout, "[t=1s] Adding second branch...\n");
    auto pad_res = gst::element_request_pad_simple(gst::Element{ctx->tee}, "src_%u");
    if(!pad_res) { return G_SOURCE_CONTINUE; }
    ctx->tee_src_pad = pad_res->release();
    gst::pad_add_probe(gst::Pad{ctx->tee_src_pad},
        gst::PadProbeType::Block | gst::PadProbeType::Buffer,
        [ctx](gst::Pad pad, GstPadProbeInfo* info) { return add_branch_probe(pad, info, ctx); });
  } else {
    fmt::print(stdout, "[t=3s] Removing second branch...\n");
    gst::pad_add_probe(gst::Pad{ctx->tee_src_pad},
        gst::PadProbeType::Block | gst::PadProbeType::Buffer,
        [ctx](gst::Pad pad, GstPadProbeInfo* info) { return remove_branch_probe(pad, info, ctx); });
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::raii::pipeline_new("dynamic-pipeline");
  auto source   = gst::raii::element_factory_make("videotestsrc", "source");
  auto convert  = gst::raii::element_factory_make("videoconvert", "convert");
  auto tee      = gst::raii::element_factory_make("tee",          "tee");
  auto queue_a  = gst::raii::element_factory_make("queue",        "queue-a");
  auto sink_a   = gst::raii::element_factory_make("autovideosink","sink-a");

  if(!pipeline || !source || !convert || !tee || !queue_a || !sink_a) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", 200, nullptr);

  auto raw_source  = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_convert = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_tee     = gst::raii::bin_add(*pipeline, std::move(*tee));
  auto raw_queue_a = gst::raii::bin_add(*pipeline, std::move(*queue_a));
  auto raw_sink_a  = gst::raii::bin_add(*pipeline, std::move(*sink_a));
  if(!raw_source || !raw_convert || !raw_tee || !raw_queue_a || !raw_sink_a) {
    fmt::print(stderr, "Failed to add elements.\n"); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_convert); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_convert, *raw_tee); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }

  auto tee_src_a = gst::element_request_pad_simple(*raw_tee, "src_%u");
  if(!tee_src_a) { fmt::print(stderr, "Failed to get tee src pad.\n"); return EXIT_FAILURE; }
  auto queue_sink = gst::element_get_static_pad(*raw_queue_a, "sink");
  if(!queue_sink) { fmt::print(stderr, "Failed to get queue sink pad.\n"); return EXIT_FAILURE; }
  if(auto l = gst::pad_link(gst::Pad{tee_src_a->get()}, gst::Pad{queue_sink->get()}); !l) {
    fmt::print(stderr, "Failed to link tee to display queue.\n"); return EXIT_FAILURE;
  }

  if(auto l = gst::element_link(*raw_queue_a, *raw_sink_a); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error()); return EXIT_FAILURE;
  }
  fmt::print(stdout, "Pipeline running...\n");

  auto* loop = g_main_loop_new(nullptr, FALSE);
  auto bus   = gst::raii::element_get_bus(*pipeline);
  if(!bus) { fmt::print(stderr, "Failed to get bus.\n"); return EXIT_FAILURE; }
  gst::bus_add_watch(*bus, [loop](gst::Message msg) -> bool {
    switch(gst::message_type(msg)) {
      case gst::MessageType::Error: {
        if(auto err = gst::message_parse_error(msg)) {
          fmt::print(stderr, "Error: {}\n", err->first);
        }
        g_main_loop_quit(loop);
        break;
      }
      case gst::MessageType::EOS:
        fmt::print(stdout, "End of stream reached.\n");
        g_main_loop_quit(loop);
        break;
      default:
        break;
    }
    return true;
  });

  DynCtx ctx{pipeline->get(), raw_tee->get()};
  g_timeout_add_seconds(1, on_timer, &ctx);
  g_timeout_add_seconds(3, on_timer, &ctx);

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  gst::element_release_request_pad(*raw_tee, gst::Pad{tee_src_a->get()});
  return EXIT_SUCCESS;
}
