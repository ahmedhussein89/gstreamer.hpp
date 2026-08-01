#pragma once
// gst::rtsp_* — thin wrapper over GstRTSPServer/GstRTSPMediaFactory/GstRTSPMountPoints
// (gst/rtsp-server/rtsp-server.h et al). gst-rtsp-server is a separate library
// (gstreamer-rtsp-server-1.0) from core GStreamer, not installed on every host
// (mirrors tutorials/hard/RTSPServer/CMakeLists.txt's own
// `if(NOT TARGET GStreamer::RtspServer) ... return()` self-skip).
//
// This header is NOT part of the gstreamer_hpp INTERFACE target's default
// sources (see include/CMakeLists.txt), so including gstreamer.hpp never
// pulls in gst-rtsp-server. Callers that need it include this header
// directly and link GStreamer::RtspServer themselves.
//
// __has_include guards the whole file so a host without gst-rtsp-server
// headers installed still compiles this header as an empty translation unit.
// UNVERIFIED ON THIS HOST: gstreamer-rtsp-server-1.0 is not installed here,
// so this section has not been compile-tested. Written against the
// documented GstRTSPServer/GstRTSPMediaFactory/GstRTSPMountPoints API.
#if __has_include(<gst/rtsp-server/rtsp-server.h>)

#include <memory>
#include <string_view>

#include <fmt/format.h>

#include <gst/rtsp-server/rtsp-server.h>

#include <core/core.hpp>
#include <nonstd/expected.hpp>

namespace gst {

struct RTSPMediaFactory : Handle<GstRTSPMediaFactory> {
  using Handle::Handle;
};
struct RTSPMountPoints : Handle<GstRTSPMountPoints> {
  using Handle::Handle;
};
struct RTSPServer : Handle<GstRTSPServer> {
  using Handle::Handle;
};

struct GstRTSPMediaFactoryDeleter final {
  void operator()(GstRTSPMediaFactory* factory) const noexcept {
    if(factory != nullptr) {
      g_object_unref(factory);
    }
  }
};
using RTSPMediaFactoryPtr = std::unique_ptr<GstRTSPMediaFactory, GstRTSPMediaFactoryDeleter>;

struct GstRTSPMountPointsDeleter final {
  void operator()(GstRTSPMountPoints* mounts) const noexcept {
    if(mounts != nullptr) {
      g_object_unref(mounts);
    }
  }
};
using RTSPMountPointsPtr = std::unique_ptr<GstRTSPMountPoints, GstRTSPMountPointsDeleter>;

struct GstRTSPServerDeleter final {
  void operator()(GstRTSPServer* server) const noexcept {
    if(server != nullptr) {
      g_object_unref(server);
    }
  }
};
using RTSPServerPtr = std::unique_ptr<GstRTSPServer, GstRTSPServerDeleter>;

inline RTSPServerPtr rtsp_server_new() noexcept {
  return RTSPServerPtr{gst_rtsp_server_new()};
}

inline nonstd::expected<RTSPMountPointsPtr, std::string> rtsp_server_get_mount_points(RTSPServer server) {
  GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server.get());
  if(mounts == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to get RTSP server mount points"));
  }
  return RTSPMountPointsPtr{mounts};
}

inline RTSPMediaFactoryPtr rtsp_media_factory_new() noexcept {
  return RTSPMediaFactoryPtr{gst_rtsp_media_factory_new()};
}

inline void rtsp_media_factory_set_launch(RTSPMediaFactory factory, std::string_view launch) {
  std::string launch_str(launch);
  gst_rtsp_media_factory_set_launch(factory.get(), launch_str.c_str());
}

// Transfers the factory into the mount points (transfer-full), matching
// gst_rtsp_mount_points_add_factory semantics.
inline void rtsp_mount_points_add_factory(RTSPMountPoints mounts, std::string_view path, RTSPMediaFactoryPtr factory) {
  std::string path_str(path);
  gst_rtsp_mount_points_add_factory(mounts.get(), path_str.c_str(), factory.release());
}

inline nonstd::expected<guint, std::string> rtsp_server_attach(RTSPServer server, GMainContext* context = nullptr) {
  const guint source_id = gst_rtsp_server_attach(server.get(), context);
  if(source_id == 0) {
    return nonstd::make_unexpected(std::string("Failed to attach RTSP server to main context"));
  }
  return source_id;
}

}    // namespace gst

#endif    // __has_include(<gst/rtsp-server/rtsp-server.h>)
