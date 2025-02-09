#pragma once
#include <array>

#include "auxiliary/peer_id.hpp"
#include "torrent/bitfield/bitfield.hpp"

namespace btr
{
using InfoHash = std::array<uint8_t, 20>;

class InternalContext
{
public:
  PeerId client_id;
  InfoHash infohash;
  uint64_t filesize;
};

class ExternalPeerContext
{
  BitField bitfield;
};

}  // namespace btr