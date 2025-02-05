#pragma once
#include <array>

#include "auxiliary/peer_id.hpp"

namespace btr
{
using InfoHash = std::array<uint8_t, 20>;

class PeerContext
{
public:
  PeerId client_id;
  InfoHash infohash;
};
}  // namespace btr