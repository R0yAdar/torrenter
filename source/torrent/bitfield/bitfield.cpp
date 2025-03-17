#include <algorithm>

#include "torrent/bitfield/bitfield.hpp"

namespace aux
{
BitField::BitField(size_t piece_count)
    : m_bitfield(piece_count)
{
}

BitField::BitField(std::vector<uint8_t> bitfield)
    : m_bitfield(std::move(bitfield))
{
}

void BitField::mark(size_t piece_index, bool value)
{
  size_t container_index =
      piece_index / (sizeof(decltype(m_bitfield)::value_type) * 8);

  if (container_index >= m_bitfield.size()) {
    m_bitfield.resize(container_index + 1);
  }

  size_t value_index =
      piece_index % (sizeof(decltype(m_bitfield)::value_type) * 8);

  uint8_t mask = 1 << (7 - value_index);

  if (value) {
    m_bitfield[container_index] |= mask;
  } else {
    m_bitfield[container_index] &= (~(mask));
  }
}

bool BitField::get(size_t piece_index) const
{
  size_t container_index =
      piece_index / (sizeof(decltype(m_bitfield)::value_type) * 8);

  if (container_index >= m_bitfield.size()) {
    return false;
  }

  size_t value_index =
      piece_index % (sizeof(decltype(m_bitfield)::value_type) * 8);

  uint8_t mask = 1 << (7 - value_index);

  return (mask & m_bitfield[container_index]) > 0;
}

bool BitField::is_empty() const
{
  uint8_t indicator = 0;

  for (auto u : m_bitfield) {
    indicator |= u;
  }

  return indicator == 0;
}

std::vector<std::uint8_t> BitField::as_raw()
{
  return m_bitfield;
}
}  // namespace aux