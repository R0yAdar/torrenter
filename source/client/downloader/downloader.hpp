#pragma once

#include <memory>
#include "client/peer.hpp"

namespace btr
{
class Downloader
{
  std::map<size_t, FilePiece> m_pieces;

  std::set<Request> m_pending_requests;
  std::set<Request> m_active_requests;

  std::shared_ptr<Peer> m_peer;

public:
  boost::asio::awaitable<bool> download_piece(size_t index);

  boost::asio::awaitable<FilePiece> retrieve_piece(size_t index);
};
}  // namespace btr