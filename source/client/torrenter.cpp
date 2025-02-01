#include <filesystem>

#include "client/torrenter.hpp"

namespace btr
{
Torrenter::Torrenter(TorrentFile torrent)
    : m_torrent {torrent}
    , m_peer_id {}
{
}
void Torrenter::download_file(std::filesystem::path at) {}
}  // namespace btr