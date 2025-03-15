#pragma once

#include <cstddef>
#include <vector>

namespace aux
{
class BitField
{
  std::vector<uint8_t> m_bitfield;

public:
  BitField() = default;

  BitField(size_t piece_count);

  BitField(std::vector<uint8_t> bitfield);

  void mark(size_t piece_index, bool value);

  bool get(size_t piece_index) const;

  bool is_empty() const;

  std::vector<uint8_t> as_raw();
};
}  // namespace aux