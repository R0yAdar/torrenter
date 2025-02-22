#pragma once

#include <vector>

#include "client/peer.hpp"

namespace btr
{
class PeerPool
{
  std::vector<Peer> m_peers;

public:
  PeerPool(std::vector<Peer> peers);
};
}  // namespace btr