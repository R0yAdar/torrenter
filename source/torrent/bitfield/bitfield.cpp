#include <algorithm>

#include "torrent/bitfield/bitfield.hpp"

BitField::BitField(size_t piece_count)
    : m_bitfield(piece_count)
{
}

BitField::BitField(std::vector<std::byte> bitfield)
    : m_bitfield {}
{
  m_bitfield.reserve(bitfield.size());
  std::copy(bitfield.cbegin(), bitfield.cend(), m_bitfield.begin());
}

constexpr void BitField::mark(size_t piece_index, bool value)
{
  size_t container_index =
      piece_index / sizeof(decltype(m_bitfield)::value_type);

  size_t value_index = piece_index % sizeof(decltype(m_bitfield)::value_type);

  std::byte mask = std::byte {1} >> value_index;

  if (value) {
    m_bitfield[container_index] |= mask;
  } else {
    m_bitfield[container_index] &= (~(mask));
  }
}

constexpr bool BitField::get(size_t piece_index)
{
  size_t container_index =
      piece_index / sizeof(decltype(m_bitfield)::value_type);

  size_t value_index = piece_index % sizeof(decltype(m_bitfield)::value_type);

  std::byte mask = static_cast<std::byte>(1 >> value_index);

  return (mask & m_bitfield[container_index]) > std::byte {0};
}

constexpr std::vector<std::byte> BitField::as_raw() 
{
  return m_bitfield;
}
