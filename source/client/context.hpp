#pragma once
#include <vector>
#include <chrono>
#include "auxiliary/peer_id.hpp"
#include "torrent/bitfield/bitfield.hpp"

namespace btr
{
using InfoHash = std::vector<uint8_t>;

enum class PieceStatus : uint8_t
{
  Pending,
  Active,
  Complete
};

struct FilePiece
{
  size_t index;
  PieceStatus status;
  std::vector<uint8_t> data;
  uint32_t bytes_downloaded;
};

class InternalContext
{
public:
  PeerId client_id;
  InfoHash info_hash;
  uint64_t file_size;
  uint32_t piece_size;
  uint32_t piece_count;

  std::string info_hash_as_string() const
  {
    std::string result;
    for (uint8_t byte : info_hash) {
      result += std::format("{:02x}", byte);
    }

    return result;
  }

  uint32_t get_piece_size(size_t index) const
  {
    auto rounded_max_index = file_size / piece_size;
    if (index < rounded_max_index) {
      return piece_size;
    }

    return file_size - rounded_max_index * piece_size;
  }
};

struct PeerStatus
{
  bool self_choked = true;

  bool remote_choked;
  bool remote_interested;

  aux::BitField remote_bitfield;

  std::chrono::steady_clock::time_point last_message_timestamp;

  bool connection_down;
};

struct ExternalPeerContext
{
  ExternalPeerContext()
      : status {}
  {
  }
  std::string peer_id;
  PeerStatus status;
};

}  // namespace btr