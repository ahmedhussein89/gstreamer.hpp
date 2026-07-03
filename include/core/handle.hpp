#pragma once
#include <compare>
#include <cstddef>
#include <type_traits>

#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstbus.h>
#include <gst/gstcaps.h>
#include <gst/gstmessage.h>
#include <gst/gstpad.h>
#include <gst/gststructure.h>

namespace gst {

// Trivially-copyable, non-owning typed handle over a raw GObject/GstObject pointer.
// Exactly sizeof(T*) — zero overhead over the raw C pointer.
template <typename T>
struct Handle {
  constexpr Handle() noexcept = default;
  constexpr Handle(T* ptr) noexcept : m_ptr(ptr) {}  // NOLINT(google-explicit-constructor)

  [[nodiscard]] constexpr T* get() const noexcept { return m_ptr; }
  constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }

  constexpr auto operator<=>(const Handle&) const noexcept = default;

  // Implicit conversion back to raw pointer so every C API call works unchanged.
  constexpr operator T*() const noexcept { return m_ptr; }  // NOLINT(google-explicit-constructor)

private:
  T* m_ptr = nullptr;
};

static_assert(sizeof(Handle<GstElement>) == sizeof(GstElement*));
static_assert(std::is_trivially_copyable_v<Handle<GstElement>>);
static_assert(std::is_trivially_destructible_v<Handle<GstElement>>);

}  // namespace gst
