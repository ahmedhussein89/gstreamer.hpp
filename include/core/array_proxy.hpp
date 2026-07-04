#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <vector>

#include <core/concepts.hpp>

namespace gst {

// ArrayProxy<T>: accepts a single value, an initializer_list, a std::array,
// a std::vector, or a raw (data, size) pair as a unified (count, ptr) argument.
// Non-owning — the referenced container must outlive the proxy.
template <ArrayElement T>
class ArrayProxy {
public:
  constexpr ArrayProxy() noexcept : m_ptr(nullptr), m_count(0) {}

  // Single element (stored by address — caller ensures lifetime).
  constexpr ArrayProxy(const T& value) noexcept : m_ptr(&value), m_count(1) {}    // NOLINT

  // Initializer list. Safe only as a function-call argument — the backing array
  // lives for the full expression containing the call. Do not store an
  // ArrayProxy built from an initializer_list beyond that scope.
  // GCC's -Winit-list-lifetime fires here even for the safe function-argument
  // pattern; suppress it locally (same approach as vulkan.hpp).
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Winit-list-lifetime"
#endif
  constexpr ArrayProxy(std::initializer_list<T> list) noexcept    // NOLINT
      : m_ptr(list.begin()), m_count(static_cast<std::uint32_t>(list.size())) {}
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

  // std::array.
  template <std::size_t N>
  constexpr ArrayProxy(const std::array<T, N>& arr) noexcept    // NOLINT
      : m_ptr(arr.data()), m_count(static_cast<std::uint32_t>(N)) {}

  // std::vector.
  ArrayProxy(const std::vector<T>& vec) noexcept    // NOLINT
      : m_ptr(vec.data()), m_count(static_cast<std::uint32_t>(vec.size())) {}

  // Raw pointer + size.
  constexpr ArrayProxy(const T* ptr, std::uint32_t count) noexcept : m_ptr(ptr), m_count(count) {}

  [[nodiscard]] constexpr const T* data() const noexcept {
    return m_ptr;
  }
  [[nodiscard]] constexpr std::uint32_t size() const noexcept {
    return m_count;
  }
  [[nodiscard]] constexpr bool empty() const noexcept {
    return m_count == 0;
  }

  [[nodiscard]] constexpr const T* begin() const noexcept {
    return m_ptr;
  }
  [[nodiscard]] constexpr const T* end() const noexcept {
    return m_ptr + m_count;
  }

  [[nodiscard]] constexpr const T& operator[](std::uint32_t i) const noexcept {
    assert(i < m_count);
    return m_ptr[i];
  }

private:
  const T* m_ptr;
  std::uint32_t m_count;
};

}    // namespace gst
