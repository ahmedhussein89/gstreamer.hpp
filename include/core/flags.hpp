#pragma once
#include <cstdint>
#include <type_traits>

#include <core/concepts.hpp>

namespace gst {

// FlagTraits: specialise to declare which bits are valid for a given Bits enum.
// Unspecialised => all bits are valid (open mask).
template <FlagEnum Bits>
struct FlagTraits {
  using MaskType = typename std::underlying_type_t<Bits>;
  static constexpr MaskType allFlags = ~MaskType{0};
};

// Flags<Bits>: type-safe bitmask over an enum Bits.
// Only OR/AND/XOR/NOT with the same Bits or Flags<Bits> are allowed — no
// accidental mixing of unrelated masks.
template <FlagEnum Bits>
struct Flags {
  using MaskType = typename FlagTraits<Bits>::MaskType;

  constexpr Flags() noexcept = default;
  constexpr Flags(Bits bit) noexcept : m_mask(static_cast<MaskType>(bit)) {}    // NOLINT
  constexpr explicit Flags(MaskType mask) noexcept : m_mask(mask) {}

  [[nodiscard]] constexpr MaskType value() const noexcept { return m_mask; }
  constexpr explicit operator bool() const noexcept { return m_mask != MaskType{0}; }
  constexpr explicit operator MaskType() const noexcept { return m_mask; }

  constexpr Flags operator|(Flags rhs) const noexcept { return Flags(m_mask | rhs.m_mask); }
  constexpr Flags operator&(Flags rhs) const noexcept { return Flags(m_mask & rhs.m_mask); }
  constexpr Flags operator^(Flags rhs) const noexcept { return Flags(m_mask ^ rhs.m_mask); }
  constexpr Flags operator~() const noexcept { return Flags(~m_mask & FlagTraits<Bits>::allFlags); }

  constexpr Flags& operator|=(Flags rhs) noexcept { m_mask |= rhs.m_mask; return *this; }
  constexpr Flags& operator&=(Flags rhs) noexcept { m_mask &= rhs.m_mask; return *this; }
  constexpr Flags& operator^=(Flags rhs) noexcept { m_mask ^= rhs.m_mask; return *this; }

  constexpr bool operator==(Flags const&) const noexcept = default;

private:
  MaskType m_mask{0};
};

// ADL operators so `BitA | BitB` works without an explicit Flags<> constructor.
template <FlagEnum Bits>
constexpr Flags<Bits> operator|(Bits lhs, Bits rhs) noexcept {
  return Flags<Bits>(lhs) | Flags<Bits>(rhs);
}

template <FlagEnum Bits>
constexpr Flags<Bits> operator&(Bits lhs, Bits rhs) noexcept {
  return Flags<Bits>(lhs) & Flags<Bits>(rhs);
}

template <FlagEnum Bits>
constexpr Flags<Bits> operator^(Bits lhs, Bits rhs) noexcept {
  return Flags<Bits>(lhs) ^ Flags<Bits>(rhs);
}

template <FlagEnum Bits>
constexpr Flags<Bits> operator~(Bits bit) noexcept {
  return ~Flags<Bits>(bit);
}

}    // namespace gst
