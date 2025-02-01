#pragma once

#include <bit>

#ifndef PACKED_ATTRIBUTE
#  if defined BROKEN_GCC_STRUCTURE_PACKING && defined __GNUC__
// Used for gcc tool chains accepting but not supporting pragma pack
// See http://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html
#    define PACKED_ATTRIBUTE __attribute__((__packed__))
#  else
#    define PACKED_ATTRIBUTE
#  endif  // defined BROKEN_GCC_STRUCTURE_PACKING && defined __GNUC__
#endif  // ndef PACKED_ATTRIBUTE

template<typename T>
constexpr inline T SwitchEndianOnLEMachine(T value)
{
  if constexpr (std::endian::native == std::endian::little) {
    return std::byteswap(value);
  } else {
    return value;
  }
}

#pragma pack(push, 1)

template<typename T>
class PACKED_ATTRIBUTE BigEndian
{
public:
  BigEndian() { m_beValue = 0; };

  BigEndian(T value) { m_beValue = SwitchEndianOnLEMachine(value); }

  T operator=(T value)
  {
    m_beValue = SwitchEndianOnLEMachine(value);
    return value;
  }

  T get_big_endian() { return m_beValue; }

  // Allow implicit conversion to T
  operator T() const { return SwitchEndianOnLEMachine(m_beValue); }

private:
  T m_beValue;
};

using byte = uint8_t;
using int16_big = BigEndian<int16_t>;
using int32_big = BigEndian<int32_t>;
using int64_big = BigEndian<int64_t>;

using uint16_big = BigEndian<uint16_t>;
using uint32_big = BigEndian<uint32_t>;
using uint64_big = BigEndian<uint64_t>;

#pragma pack(pop)