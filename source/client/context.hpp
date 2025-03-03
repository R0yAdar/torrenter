#pragma once
#include <array>
#include <chrono>
#include <iostream>
#include "auxiliary/peer_id.hpp"
#include "torrent/bitfield/bitfield.hpp"

namespace btr
{
using InfoHash = std::array<uint8_t, 20>;

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
  InfoHash infohash;
  uint64_t filesize;
  uint32_t piece_size;
  uint32_t piece_count;

  uint32_t get_piece_size(size_t index) const
  {
    auto rounded_max_index = filesize / piece_size;
    if (index < rounded_max_index) {
      return piece_size;
    }

    return filesize - rounded_max_index * piece_size;
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