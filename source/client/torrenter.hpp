#pragma once

#include "auxiliary/peer_id.hpp"
#include "torrent/metadata/torrentfile.hpp"
#include "client/storage/storage.hpp"

namespace btr
{

class Torrenter
{
  const TorrentFile m_torrent;
  const PeerId m_peer_id;

public:
  Torrenter(TorrentFile torrent);

  void download_file(std::filesystem::path at,
                     std::shared_ptr<IStorage> storage_device);
};

}  // namespace btr