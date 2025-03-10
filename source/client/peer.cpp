#include <concepts>
#include <iostream>
#include <typeinfo>

#include "client/peer.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include "auxiliary/variant_aux.hpp"
#include "client/transmit/transmit.hpp"
#include "torrent/messages.hpp"

using boost::asio::awaitable;
using boost::asio::ip::address;
using boost::asio::ip::port_type;
using boost::asio::ip::tcp;
using namespace boost::asio::experimental::awaitable_operators;

namespace btr
{
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

Peer::Peer(std::shared_ptr<const InternalContext> context,
           PeerContactInfo peer_info,
           boost::asio::any_io_executor& io,
           size_t max_outgoing_messages)
    : m_contact_info {std::move(peer_info)}
    , m_application_context {std::move(context)}
    , m_context {std::make_shared<ExternalPeerContext>()}
    , m_send_queue {io, max_outgoing_messages}
    , m_socket {io}
{
}

const ExternalPeerContext& Peer::get_context() const
{
  return *m_context;
}

const PeerActivity& Peer::get_activity() const
{
  return m_activity;
}

awaitable<void> Peer::start_async()
{
  if (!m_activity.is_active) {
    m_activity.is_active = true;
    m_is_stopping = false;

    co_await connect_async();
    co_await (receive_loop_async() && send_loop_async());
  }

  std::cout << m_context->peer_id << " Exited\n";

  m_activity.is_active = false;
}

awaitable<void> Peer::connect_async()
{
  co_await m_socket.async_connect(tcp::endpoint(m_contact_info.address, m_contact_info.port),
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
  m_activity.is_receiver_active = true;

  while (!m_is_stopping) {
    try {
      if (auto message = co_await read_message(m_socket)) {

        //std::cout << m_context->peer_id << " RType" << message->index() << '\n';

        handle_message(*message);

        for (auto const& callback : m_callbacks) {
          if (auto spt = callback.lock()) {
            co_await (*spt)(*message);
          }
        }
      } else {
        m_activity.latest_parse_errors.push_back(message.error());
      }

    } catch (const std::exception& e) {
      m_activity.receiver_exit_message = e.what();
      break;
    }
  }

  if (m_activity.is_sender_active) {
    co_await internal_stop_sender();
  }

  m_activity.is_receiver_active = false;
}

awaitable<void> Peer::send_loop_async()
{
  m_activity.is_sender_active = true;

  while (!m_is_stopping) {
    try {
      auto message =
          co_await m_send_queue.async_receive(boost::asio::use_awaitable);

      // std::cout << m_context->peer_id << " SType" << message.index() << '\n';

      if (m_is_stopping) {
        break;
      }

      co_await send_message(m_socket, message);

    } catch (const std::exception& e) {
      m_activity.sender_exit_message = e.what();
      break;
    }
  }

  m_send_queue.reset();
  m_activity.is_sender_active = false;
}

awaitable<void> Peer::send_async(TorrentMessage message)
{
  co_await m_send_queue.async_send(
      boost::system::error_code {}, message, boost::asio::use_awaitable);
}

awaitable<void> Peer::internal_stop_sender()
{
  m_is_stopping = true;
  co_await m_send_queue.async_send(
      boost::system::error_code {}, {Keepalive {}}, boost::asio::use_awaitable);
}
}  // namespace btr