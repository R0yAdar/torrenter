#pragma once

#include <cstddef>
#include <vector>

class BitField
{
private:
  std::vector<std::byte> m_bitfield;

public:
  BitField(size_t piece_count);

  BitField(std::vector<std::byte> bitfield);

  constexpr void mark(size_t piece_index, bool value);

  constexpr bool get(size_t piece_index);

  constexpr std::vector<std::byte> as_raw();
};