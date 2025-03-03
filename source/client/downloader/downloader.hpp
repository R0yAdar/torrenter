#pragma once

#include <memory>
#include <optional>
#include <deque>
#include "client/peer.hpp"

namespace btr
{

struct RequestIdentifier
{
  size_t piece_index;
  uint32_t piece_offset;
  uint32_t block_length;
  auto operator<=>(const RequestIdentifier&) const = default;
};

class Downloader
{
  std::map<size_t, FilePiece> m_pieces;

  std::deque<Request> m_pending_requests;
  std::set<RequestIdentifier> m_active_requests;

  std::shared_ptr<Peer> m_peer;
  std::shared_ptr<const InternalContext> m_application_context;

public:
  Downloader(std::shared_ptr<Peer> peer,
             std::shared_ptr<const InternalContext> context);

  const ExternalPeerContext& get_context() const;

  boost::asio::awaitable<bool> download_piece(size_t index);

  boost::asio::awaitable<std::optional<FilePiece>> retrieve_piece(size_t index);

private:
  boost::asio::awaitable<void> triggered_on_received_message(TorrentMessage trigger);

  boost::asio::awaitable<void> send_buffered_messages();

  void handle_piece(const Piece& piece);
};
}  // namespace btr