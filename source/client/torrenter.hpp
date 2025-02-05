#pragma once
#include <filesystem>
#include <vector>

#include "auxiliary/peer_id.hpp"
#include "torrent/metadata/torrentfile.hpp"

namespace btr
{

class Torrenter
{
public:
  const TorrentFile m_torrent;
  const PeerId m_peer_id;

public:
  Torrenter(TorrentFile torrent);

  void download_file(std::filesystem::path at);
};

}  // namespace btr