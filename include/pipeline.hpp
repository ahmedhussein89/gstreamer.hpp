#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include <gst/gst.h>
#include <gstreamer.hpp>

#include <nonstd/expected.hpp>

namespace gst {

using PropertyValue = std::variant<bool, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, double, std::string>;

struct Node {
  std::string factory;
  std::string name;
  std::vector<std::pair<std::string, PropertyValue>> properties;

  explicit Node(std::string factory_, std::string name_ = {}) : factory{std::move(factory_)}, name{std::move(name_)} {}

  template <typename T>
  [[nodiscard]] Node prop(std::string key, T value) && {
    properties.emplace_back(std::move(key), PropertyValue{std::move(value)});
    return std::move(*this);
  }

  [[nodiscard]] Node prop(std::string key, const char* value) && {
    properties.emplace_back(std::move(key), PropertyValue{std::string{value}});
    return std::move(*this);
  }
};

struct PipelineDesc {
  std::vector<Node> elements;

  template <typename... Nodes>
  explicit PipelineDesc(Nodes&&... nodes) : elements{std::forward<Nodes>(nodes)...} {}
};

namespace detail {

inline void apply_property(GstElement* elem, const std::string& key, const PropertyValue& val) {
  std::visit(
      [&](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, std::string>) {
          g_object_set(G_OBJECT(elem), key.c_str(), v.c_str(), nullptr);
        } else {
          g_object_set(G_OBJECT(elem), key.c_str(), v, nullptr);
        }
      },
      val);
}

}    // namespace detail

inline nonstd::expected<Pipeline, std::string> build(const PipelineDesc& desc) {
  if(desc.elements.empty()) {
    return nonstd::make_unexpected(std::string("Pipeline must contain at least one element"));
  }

  GstElement* raw_pipeline = gst_pipeline_new(nullptr);
  if(raw_pipeline == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to create GstPipeline"));
  }

  std::vector<GstElement*> raw_elements;
  raw_elements.reserve(desc.elements.size());

  for(const auto& node : desc.elements) {
    GstElement* elem = gst_element_factory_make(node.factory.c_str(), node.name.empty() ? nullptr : node.name.c_str());
    if(elem == nullptr) {
      gst_object_unref(raw_pipeline);
      return nonstd::make_unexpected(fmt::format("Failed to create element '{}'", node.factory));
    }

    for(const auto& [key, val] : node.properties) {
      detail::apply_property(elem, key, val);
    }

    gst_bin_add(GST_BIN(raw_pipeline), elem);
    raw_elements.push_back(elem);
  }

  for(std::size_t i = 0; i + 1 < raw_elements.size(); ++i) {
    if(gst_element_link(raw_elements[i], raw_elements[i + 1]) == FALSE) {
      gst_object_unref(raw_pipeline);
      return nonstd::make_unexpected(
          fmt::format("Failed to link '{}' to '{}'", desc.elements[i].factory, desc.elements[i + 1].factory));
    }
  }

  return Pipeline{raw_pipeline};
}

template <typename T>
inline nonstd::expected<void, std::string> element_set_state(const T& element, GstState state) {
  if(!element) {
    return nonstd::make_unexpected("Element is null");
  }
  if(GstStateChangeReturn::GST_STATE_CHANGE_FAILURE == gst_element_set_state(element.get(), state)) {
    return nonstd::make_unexpected("Failed to change element state");
  }
  return {};
}

}    // namespace gst
