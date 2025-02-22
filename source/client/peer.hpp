#pragma once

#include <memory>
#include <map>
#include <set>
#include <deque>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>

#include "client/context.hpp"
#include "torrent/messages.hpp"

namespace btr
{
using channel_t = boost::asio::experimental::channel<void(
    boost::system::error_code, TorrentMessage)>;
using boost::asio::ip::address;
using boost::asio::ip::port_type;

struct RequestIdentifier
{
  size_t piece_index;
  uint32_t piece_offset;
  uint32_t block_length;
  auto operator<=>(const RequestIdentifier&) const = default;
};

class Peer
{
  address m_address;
  port_type m_port;

  std::shared_ptr<const InternalContext> m_application_context;
  std::shared_ptr<ExternalPeerContext> m_context;

  std::map<size_t, FilePiece> m_pieces;

  std::deque<Request> m_pending_requests;
  std::set<RequestIdentifier> m_active_requests;

  channel_t m_send_queue;

  boost::asio::ip::tcp::socket m_socket;
  bool m_is_running;

public:
  Peer(std::shared_ptr<const InternalContext> context,
       address address,
       port_type port,
       boost::asio::io_context& io,
       size_t max_outgoing_messages = 10);

  const ExternalPeerContext& get_context() const;

  boost::asio::awaitable<void> start_async();

  boost::asio::awaitable<bool> download_piece(size_t index);

  boost::asio::awaitable<FilePiece> retrieve_piece(size_t index);

  void stop();

private:
  boost::asio::awaitable<void> connect_async();

  boost::asio::awaitable<void> send_async(TorrentMessage message);

  boost::asio::awaitable<void> send_loop_async();

  boost::asio::awaitable<void> receive_loop_async();

  void handle_piece(const Piece& piece);

  boost::asio::awaitable<void> send_requests();

  void handle_message(TorrentMessage message);
};
}  // namespace btr