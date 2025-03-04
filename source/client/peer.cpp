#include <concepts>
#include <iostream>
#include <typeinfo>

#include "client/peer.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include "auxiliary/variant_aux.hpp"
#include "client/transmit/boost_transmit.hpp"
#include "torrent/messages.hpp"

using boost::asio::awaitable;
using boost::asio::ip::address;
using boost::asio::ip::port_type;
using boost::asio::ip::tcp;
using namespace boost::asio::experimental::awaitable_operators;

namespace btr
{

Peer::Peer(std::shared_ptr<const InternalContext> context,
           address address,
           port_type port,
           boost::asio::io_context& io,
           size_t max_outgoing_messages)
    : m_address {std::move(address)}
    , m_port {port}
    , m_application_context {std::move(context)}
    , m_context {std::make_shared<ExternalPeerContext>()}
    , m_send_queue {io, max_outgoing_messages}
    , m_socket {io}
    , m_is_running {true}
{
}

const ExternalPeerContext& Peer::get_context() const
{
  return *m_context;
}

awaitable<void> Peer::start_async()
{
  co_await connect_async();
  co_await (receive_loop_async() && send_loop_async());
}

awaitable<void> Peer::connect_async()
{
  co_await m_socket.async_connect(tcp::endpoint(m_address, m_port),
                                  boost::asio::use_awaitable);
  {
    btr::Handshake handshake {*m_application_context};

    co_await send_handshake(m_socket, handshake);
  }

  auto handshake = co_await read_handshake(m_socket);

  m_context->peer_id =
      std::string(handshake.peer_id, sizeof(handshake.peer_id));
}

void Peer::add_callback(std::weak_ptr<message_callback> callback)
{
  m_callbacks.push_back(std::move(callback));
}

namespace
{
void handle_keep_alive(PeerStatus& status) {}

template<typename T>
concept StatusMessage = std::same_as<T, Choke> || std::same_as<T, Unchoke>
    || std::same_as<T, Interested> || std::same_as<T, NotInterested>;

template<StatusMessage T>
void change_local_status(T, PeerStatus& status)
{
  if constexpr (std::same_as<T, Choke>) {
    status.self_choked = true;
  } else if constexpr (std::same_as<T, Unchoke>) {
    status.self_choked = false;
  } else if constexpr (std::same_as<T, Interested>) {
    status.remote_interested = true;
  } else if constexpr (std::same_as<T, NotInterested>) {
    status.remote_interested = false;
  }
}

void assign_bitfield(const std::vector<uint8_t>& raw_bitfield,
                     PeerStatus& status)
{
  status.remote_bitfield = aux::BitField {raw_bitfield};
}

void mark_bitfield(uint32_t index, PeerStatus& status)
{
  constexpr bool HAS_INDEX = true;
  status.remote_bitfield.mark(index, HAS_INDEX);
}
}  // namespace

void Peer::handle_message(TorrentMessage message)
{
  m_context->status.last_message_timestamp = std::chrono::steady_clock::now();

  std::visit(
      overloaded {
          [&](Keepalive) { handle_keep_alive(m_context->status); },
          [&](auto status_message)
          { change_local_status(status_message, m_context->status); },
          [&](const Have& have)
          { mark_bitfield(have.piece_index, m_context->status); },
          [&](const BitField& bitfield)
          { assign_bitfield(bitfield.get_payload(), m_context->status); },
          [](const Request&) {},
          [](const Piece&) {},
          [](const Cancel&) {},
          [](const Port&) {},
      },
      message);
}

awaitable<void> Peer::receive_loop_async()
{
  while (m_is_running) {
    try {
      if (auto message = co_await read_message(m_socket)) {
        std::cout << m_context->peer_id << "RType: " << message->index()
                  << std::endl;

        handle_message(*message);

        for (auto const& callback : m_callbacks) {
          if (auto spt = callback.lock()) {
            co_await (*spt)(*message);
          }
        }
      } else {
        std::cout << "Message parsing error: "
                  << static_cast<int>(message.error()) << std::endl;
      }

    } catch (const std::exception& e) {
    }
  }
}

awaitable<void> Peer::send_loop_async()
{
  while (m_is_running) {
    try {
      auto message =
          co_await m_send_queue.async_receive(boost::asio::use_awaitable);

      std::cout << m_context->peer_id << " SType" << message.index() << '\n';

      co_await send_message(m_socket, message);

    } catch (const std::exception& e) {
      std::cout << m_context->peer_id << " " << m_address
                << "EXCEPTION: " << e.what();
    }
  }
}

awaitable<void> Peer::send_async(TorrentMessage message)
{
  co_await m_send_queue.async_send(
      boost::system::error_code {}, message, boost::asio::use_awaitable);
}

void Peer::stop()
{
  m_is_running = false;
}

}  // namespace btr