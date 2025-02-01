#pragma once
#include <filesystem>
#include <vector>

#include "client/peer.hpp"
#include "auxiliary/peer_id.hpp"
#include "bitTorrent/metadata/torrentfile.hpp"

namespace btr
{

class Torrenter
{
public:
  const TorrentFile m_torrent;
  const PeerId m_peer_id;

private:
  std::vector<Peer> m_peers;

public:
  Torrenter(TorrentFile torrent);

  void download_file(std::filesystem::path at);
};

}  // namespace btr