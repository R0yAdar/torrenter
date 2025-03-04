#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>

#include "client/context.hpp"
#include "torrent/messages.hpp"

namespace btr
{
using channel_t = boost::asio::experimental::channel<void(
    boost::system::error_code, TorrentMessage)>;

using message_callback =
    std::function<boost::asio::awaitable<void>(TorrentMessage)>;

using boost::asio::ip::address;
using boost::asio::ip::port_type;

class Peer
{
  address m_address;
  port_type m_port;

  std::shared_ptr<const InternalContext> m_application_context;
  std::shared_ptr<ExternalPeerContext> m_context;

  channel_t m_send_queue;

  boost::asio::ip::tcp::socket m_socket;
  bool m_is_running;

  std::vector<std::weak_ptr<message_callback>> m_callbacks;

public:
  Peer(std::shared_ptr<const InternalContext> context,
       address address,
       port_type port,
       boost::asio::io_context& io,
       size_t max_outgoing_messages = 10);

  const ExternalPeerContext& get_context() const;

  void add_callback(std::weak_ptr<message_callback>);

  boost::asio::awaitable<void> start_async();

  boost::asio::awaitable<void> send_async(TorrentMessage message);

  void stop();

private:
  boost::asio::awaitable<void> connect_async();

  boost::asio::awaitable<void> send_loop_async();

  boost::asio::awaitable<void> receive_loop_async();

  void handle_message(TorrentMessage message);
};
}  // namespace btr