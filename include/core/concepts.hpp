#pragma once
#include <concepts>
#include <type_traits>

#include <gst/gst.h>

namespace gst {

// GstHandlePointer<T>: T must be a class/struct type.
// All GObject/GstObject pointee types (GstElement, GstPad, GstBus, …) are
// C structs, which map to C++ class types. This prevents Handle<int>,
// Handle<void*>, and other meaningless instantiations.
template <typename T>
concept GstHandlePointer = std::is_class_v<T>;

// FlagEnum<Bits>: Bits must be a scoped or unscoped enum type, making it
// a valid underlying bit-field for Flags<Bits>.
template <typename Bits>
concept FlagEnum = std::is_enum_v<Bits>;

// ArrayElement<T>: T must satisfy std::copyable so ArrayProxy<T> can hold
// a non-owning pointer to elements and hand them out by value.
template <typename T>
concept ArrayElement = std::copyable<T>;

}  // namespace gst

namespace ds {

// DsElement<T>: T must expose get() → GstElement* and release() → GstElement*,
// the minimum contract required by Builder::add. Satisfied by all ds:: typed
// element wrappers and by gst::raii::Element.
template <typename T>
concept DsElement = requires(T t) {
  { t.get() }     -> std::convertible_to<GstElement*>;
  { t.release() } -> std::convertible_to<GstElement*>;
};

}  // namespace ds
