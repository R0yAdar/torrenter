#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <map>
#include <set>

#include "client/peer.hpp"

namespace btr
{

struct DownloaderPolicy
{
  uint32_t max_block_bytes = 8 * 1024;
  uint16_t max_concurrent_outgoing_requests = 7;
};

struct RequestIdentifier
{
  uint32_t piece_index;
  uint32_t piece_offset;
  uint32_t block_length;
  auto operator<=>(const RequestIdentifier&) const = default;
};

class Downloader
{
  std::shared_ptr<const InternalContext> m_application_context;
  std::shared_ptr<Peer> m_peer;
  DownloaderPolicy m_policy;

  std::map<size_t, FilePiece> m_pieces;
  std::deque<Request> m_pending_requests;
  std::set<RequestIdentifier> m_active_requests;
  std::shared_ptr<message_callback> m_callback;

public:
  Downloader(std::shared_ptr<const InternalContext> context,
             std::shared_ptr<Peer> peer,
             DownloaderPolicy policy = {});

  const ExternalPeerContext& get_context() const;

  const PeerActivity& get_activity() const;

  boost::asio::awaitable<bool> download_piece(uint32_t index);

  boost::asio::awaitable<std::optional<FilePiece>> retrieve_piece(size_t index);

  boost::asio::awaitable<void> restart_connection() const;

private:
  boost::asio::awaitable<void> triggered_on_received_message(
      TorrentMessage trigger);

  boost::asio::awaitable<void> send_buffered_messages();

  void handle_piece(const Piece& piece);
};
}  // namespace btr