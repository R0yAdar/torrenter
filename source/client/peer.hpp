#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/circular_buffer.hpp>

#include "client/context.hpp"
#include "torrent/messages.hpp"
#include "transmit/transmit.hpp"

namespace btr
{
using channel_t = boost::asio::experimental::channel<void(
    boost::system::error_code, TorrentMessage)>;

using message_callback =
    std::function<boost::asio::awaitable<void>(TorrentMessage)>;

using boost::asio::ip::address;
using boost::asio::ip::port_type;

struct PeerActivity
{
  bool is_active = false;
  bool is_sender_active = false;
  bool is_receiver_active = false;

  boost::circular_buffer<ParseError> latest_parse_errors {3};

  std::string receiver_exit_message;
  std::string sender_exit_message;
};

struct PeerContactInfo
{
  address address;
  port_type port;

  bool operator==(const PeerContactInfo& o) const
  {
    return address == o.address && port == o.port;
  }

  bool operator<(const PeerContactInfo& o) const { return address < o.address; }
};

class Peer
{
  PeerContactInfo m_contact_info;

  std::shared_ptr<const InternalContext> m_application_context;
  std::shared_ptr<ExternalPeerContext> m_context;

  channel_t m_send_queue;

  tcp::socket m_socket;

  PeerActivity m_activity;
  bool m_is_stopping = false;

  std::vector<std::weak_ptr<message_callback>> m_callbacks;

public:
  Peer(std::shared_ptr<const InternalContext> context,
       PeerContactInfo peer_info,
       boost::asio::any_io_executor& io,
       size_t max_outgoing_messages = 10);

  const ExternalPeerContext& get_context() const;

  const PeerActivity& get_activity() const;

  void add_callback(std::weak_ptr<message_callback>);

  boost::asio::awaitable<void> start_async();

  boost::asio::awaitable<void> send_async(TorrentMessage message);

private:
  boost::asio::awaitable<void> connect_async();

  boost::asio::awaitable<void> send_loop_async();

  boost::asio::awaitable<void> receive_loop_async();

  void handle_message(TorrentMessage message);

  boost::asio::awaitable<void> internal_stop_sender();
};
}  // namespace btr