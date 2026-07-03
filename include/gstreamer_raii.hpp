#pragma once
// RAII owning layer — gst::raii::*
// Mirrors vulkan_raii.hpp: each type owns its resource and releases in the
// destructor. Implicitly converts to the matching non-owning gst:: handle so
// every enhanced-layer free function works on a RAII object unchanged.
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <gst/gst.h>
#include <nonstd/expected.hpp>

#include <gstreamer.hpp>

namespace gst::raii {

// ============================================================================
// Element — owning, move-only, unrefs in destructor
// ============================================================================

class Element {
public:
  Element() noexcept = default;
  explicit Element(GstElement* raw) noexcept : m_ptr(raw) {}

  ~Element() = default;
  Element(Element&&) noexcept = default;
  Element& operator=(Element&&) noexcept = default;
  Element(const Element&)            = delete;
  Element& operator=(const Element&) = delete;

  [[nodiscard]] GstElement* get() const noexcept { return m_ptr.get(); }
  [[nodiscard]] GstElement* release() noexcept { return m_ptr.release(); }
  explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  // Implicit conversion to non-owning handle — passes through to every enhanced-layer free function.
  operator gst::Element() const noexcept { return gst::Element{m_ptr.get()}; }  // NOLINT

private:
  gst::ElementPtr m_ptr;
};

static_assert(!std::is_copy_constructible_v<Element>);
static_assert(!std::is_copy_assignable_v<Element>);
static_assert(std::is_move_constructible_v<Element>);
static_assert(std::is_move_assignable_v<Element>);

// ============================================================================
// Pipeline — owning, move-only
// ============================================================================

class Pipeline {
public:
  Pipeline() noexcept = default;
  explicit Pipeline(GstElement* raw) noexcept : m_ptr(raw) {}

  ~Pipeline() = default;
  Pipeline(Pipeline&&) noexcept = default;
  Pipeline& operator=(Pipeline&&) noexcept = default;
  Pipeline(const Pipeline&)            = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  [[nodiscard]] GstElement* get() const noexcept { return m_ptr.get(); }
  [[nodiscard]] GstElement* release() noexcept { return m_ptr.release(); }
  explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  operator gst::Pipeline() const noexcept { return gst::Pipeline{m_ptr.get()}; }  // NOLINT
  operator gst::Element() const noexcept { return gst::Element{m_ptr.get()}; }    // NOLINT

private:
  gst::ElementPtr m_ptr;
};

static_assert(!std::is_copy_constructible_v<Pipeline>);
static_assert(std::is_move_constructible_v<Pipeline>);

// ============================================================================
// Bus — owning, move-only
// ============================================================================

class Bus {
public:
  Bus() noexcept = default;
  explicit Bus(GstBus* raw) noexcept : m_ptr(raw) {}
  explicit Bus(gst::BusPtr ptr) noexcept : m_ptr(std::move(ptr)) {}

  ~Bus() = default;
  Bus(Bus&&) noexcept = default;
  Bus& operator=(Bus&&) noexcept = default;
  Bus(const Bus&)            = delete;
  Bus& operator=(const Bus&) = delete;

  [[nodiscard]] GstBus* get() const noexcept { return m_ptr.get(); }
  [[nodiscard]] GstBus* release() noexcept { return m_ptr.release(); }
  explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  operator gst::Bus() const noexcept { return gst::Bus{m_ptr.get()}; }  // NOLINT

private:
  gst::BusPtr m_ptr;
};

// ============================================================================
// Pad — owning, move-only
// ============================================================================

class Pad {
public:
  Pad() noexcept = default;
  explicit Pad(GstPad* raw) noexcept : m_ptr(raw) {}
  explicit Pad(gst::PadPtr ptr) noexcept : m_ptr(std::move(ptr)) {}

  ~Pad() = default;
  Pad(Pad&&) noexcept = default;
  Pad& operator=(Pad&&) noexcept = default;
  Pad(const Pad&)            = delete;
  Pad& operator=(const Pad&) = delete;

  [[nodiscard]] GstPad* get() const noexcept { return m_ptr.get(); }
  [[nodiscard]] GstPad* release() noexcept { return m_ptr.release(); }
  explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  operator gst::Pad() const noexcept { return gst::Pad{m_ptr.get()}; }  // NOLINT

private:
  gst::PadPtr m_ptr;
};

// ============================================================================
// Caps — owning, move-only
// ============================================================================

class Caps {
public:
  Caps() noexcept = default;
  explicit Caps(GstCaps* raw) noexcept : m_ptr(raw) {}
  explicit Caps(gst::CapsPtr ptr) noexcept : m_ptr(std::move(ptr)) {}

  ~Caps() = default;
  Caps(Caps&&) noexcept = default;
  Caps& operator=(Caps&&) noexcept = default;
  Caps(const Caps&)            = delete;
  Caps& operator=(const Caps&) = delete;

  [[nodiscard]] GstCaps* get() const noexcept { return m_ptr.get(); }
  [[nodiscard]] GstCaps* release() noexcept { return m_ptr.release(); }
  explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  operator gst::Caps() const noexcept { return gst::Caps{m_ptr.get()}; }  // NOLINT

private:
  gst::CapsPtr m_ptr;
};

// ============================================================================
// Message — owning, move-only
// ============================================================================

class Message {
public:
  Message() noexcept = default;
  explicit Message(GstMessage* raw) noexcept : m_ptr(raw) {}
  explicit Message(gst::MessagePtr ptr) noexcept : m_ptr(std::move(ptr)) {}

  ~Message() = default;
  Message(Message&&) noexcept = default;
  Message& operator=(Message&&) noexcept = default;
  Message(const Message&)            = delete;
  Message& operator=(const Message&) = delete;

  [[nodiscard]] GstMessage* get() const noexcept { return m_ptr.get(); }
  [[nodiscard]] GstMessage* release() noexcept { return m_ptr.release(); }
  explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }

  operator gst::Message() const noexcept { return gst::Message{m_ptr.get()}; }  // NOLINT

private:
  gst::MessagePtr m_ptr;
};

// ============================================================================
// Factory functions — return owning RAII types
// ============================================================================

inline nonstd::expected<Element, gst::ErrorPtr> parse_launch(std::string_view description) {
  auto result = gst::parse_launch(description);
  if(!result) {
    return nonstd::make_unexpected(std::move(result.error()));
  }
  return Element{result->get()};
}

inline nonstd::expected<Pipeline, std::string> pipeline_new(std::string_view name = {}) {
  GstElement* p = gst_pipeline_new(name.empty() ? nullptr : std::string{name}.c_str());
  if(p == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to create pipeline"));
  }
  return Pipeline{p};
}

inline nonstd::expected<Element, std::string> element_factory_make(
    std::string_view factory, std::string_view name = {}) {
  GstElement* elem = gst_element_factory_make(
      std::string{factory}.c_str(), name.empty() ? nullptr : std::string{name}.c_str());
  if(elem == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create element '{}'", factory));
  }
  return Element{elem};
}

// bin_add: transfers ownership of the element into the pipeline bin.
// The bin sinks the floating reference. Returns a non-owning gst::Element
// handle (valid for the lifetime of the pipeline) for subsequent linking.
inline nonstd::expected<gst::Element, std::string> bin_add(const Pipeline& pipeline, Element element) {
  GstElement* raw = element.release();
  if(gst_bin_add(GST_BIN(pipeline.get()), raw) != TRUE) {
    gst_object_unref(raw);
    return nonstd::make_unexpected(std::string("Failed to add element to pipeline"));
  }
  return gst::Element{raw};
}

// element_get_bus: returns an owning Bus (bus ref++ must be unref'd by caller).
inline nonstd::expected<Bus, std::string> element_get_bus(gst::Element element) {
  GstBus* bus = gst_element_get_bus(element.get());
  if(bus == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to get bus from element"));
  }
  return Bus{bus};
}

// element_get_static_pad: returns an owning Pad (pad ref++ must be unref'd).
inline nonstd::expected<Pad, std::string> element_get_static_pad(gst::Element element, std::string_view name) {
  std::string name_str(name);
  GstPad* pad = gst_element_get_static_pad(element.get(), name_str.c_str());
  if(pad == nullptr) {
    return nonstd::make_unexpected(std::string("No static pad '") + name_str + "' on element");
  }
  return Pad{pad};
}

}  // namespace gst::raii
