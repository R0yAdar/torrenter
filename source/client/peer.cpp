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
    : m_address {address}
    , m_port {port}
    , m_application_context {context}
    , m_context {std::make_shared<ExternalPeerContext>()}
    , m_send_queue {io, max_outgoing_messages}
    , m_socket {io}
    , m_is_running {true}
{
}

boost::asio::awaitable<bool> Peer::download_piece(size_t index)
{
  std::cout << "Downloading piece: " << index << '\n';

  if (!m_context->status.remote_bitfield.get(index)) {
    co_return false;
  }

  m_pieces[index] = FilePiece {
      index,
      PieceStatus::Pending,
      std::vector<uint8_t>(m_application_context->get_piece_size(index))};

  if (m_context->status.self_choked) {
    co_await send_async(TorrentMessage {Interested {}});
  }

  auto piece_length = m_application_context->get_piece_size(index);

  const uint32_t max_request_size = 8 * 1024;

  for (uint32_t offset = 0; offset < piece_length; offset += max_request_size) {
    auto req = Request {};
    req.piece_index = index;
    req.offset_within_piece = offset;
    req.length = std::min(piece_length - offset, max_request_size);

    m_pending_requests.emplace_back(req);
  }

  std::cout << "Pending requests " << m_pending_requests.size() << std::endl;

  co_await send_requests();

  co_return true;
}

boost::asio::awaitable<FilePiece> Peer::retrieve_piece(size_t index)
{
  if (!m_pieces.contains(index)) {
    co_return FilePiece {.status = PieceStatus::Complete};
  }

  if (m_pieces[index].status == PieceStatus::Complete) {
    co_return m_pieces.extract(m_pieces.find(index)).mapped();
  }

  co_return m_pieces[index];
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
  try {
    co_await m_socket.async_connect(tcp::endpoint(m_address, m_port),
                                    boost::asio::use_awaitable);
    {
      btr::Handshake handshake {*m_application_context};

      co_await send_handshake(m_socket, handshake);
    }

    auto handshake = co_await read_handshake(m_socket);

    m_context->peer_id =
        std::string(handshake.peer_id, sizeof(handshake.peer_id));

  } catch (const std::exception& e) {
  }
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
  status.remote_bitfield.mark(index, true);
}

void handle_request(const Request& request, ...)
{
  std::cout << "Got request\n";
}

void handle_port(const Port& request, ...)
{
  std::cout << "STUB: Got port\n";
}

void handle_cancel(const Cancel& request, ...)
{
  std::cout << "STUB: Got cancel\n";
}
}  // namespace

void Peer::handle_piece(const Piece& piece)
{
  std::cout << "Got piece block " << piece.get_metadata().piece_index << "  "
            << piece.get_payload().size() << "\n";

  if (!m_pieces.contains(piece.get_metadata().piece_index)) {
    std::cout << "Got piece index: " << piece.get_metadata().piece_index
              << " without asking.\n";

    return;
  }

  auto data = piece.get_payload();
  m_active_requests.erase(
      RequestIdentifier {piece.get_metadata().piece_index,
                         piece.get_metadata().offset_within_piece,
                         static_cast<uint32_t>(data.size())});

  FilePiece& current_piece = m_pieces[piece.get_metadata().piece_index];

  std::copy(
      data.cbegin(),
      data.cend(),
      current_piece.data.begin() + piece.get_metadata().offset_within_piece);

  current_piece.bytes_downloaded += data.size();

  if (current_piece.bytes_downloaded >= current_piece.data.size()) {
    std::cout << "EXCEEDED PIECE SIZE BY (MAYBE CHOKE RELATED...) "
              << current_piece.data.size() << std::endl;
  }

  if (current_piece.bytes_downloaded == current_piece.data.size()) {
    current_piece.status = PieceStatus::Complete;
  } else {
    current_piece.status = PieceStatus::Active;
  }
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
          [&](const Request& req) { handle_request(req); },
          [&](const Piece& piece) { handle_piece(piece); },
          [&](const Cancel& cancel) { handle_cancel(cancel); },
          [&](const Port& port) { handle_port(port); },
      },
      message);
}

awaitable<void> Peer::receive_loop_async()
{
  while (m_is_running) {
    try {
      auto message = co_await read_message(m_socket);

      if (message) {
        std::cout << m_context->peer_id << "RType: " << message->index()
                  << std::endl;
        handle_message(*message);

        if (message->index() == 0) {
          co_await send_async(TorrentMessage {Keepalive {}});
        } else if (message->index() == ID_UNCHOKE + 1) {
          std::cout << "UNCHOKE --> SENDING REQUESTS\n";
          co_await send_requests();
        } else if (message->index() == ID_CHOKE + 1) {
          std::cout << "CHOKE --> CANCELING REQUESTS\n";

          for (auto& reqi : m_active_requests) {
            auto req = Request {};
            req.piece_index = reqi.piece_index;
            req.offset_within_piece = reqi.piece_offset;
            req.length = reqi.block_length;
            m_pending_requests.push_front(req);
          }

          m_active_requests.clear();
        } else if (message->index() == ID_PIECE + 1) {
          co_await send_requests();
        }
      } else {
        std::cout << "Message parsing error: " << (int)message.error()
                  << std::endl;
      }

    } catch (const std::exception& e) {
    }
  }
}

boost::asio::awaitable<void> Peer::send_requests()
{
  while (!m_context->status.self_choked && !m_pending_requests.empty()
         && m_active_requests.size() < 7)
  {
    auto req = m_pending_requests.front();
    std::cout << "Sending request for: " << req.piece_index << " at offset "
              << req.offset_within_piece << "  and length of " << req.length
              << std::endl;

    co_await send_async(TorrentMessage {req});
    m_active_requests.emplace(
        req.piece_index, req.offset_within_piece, req.length);
    m_pending_requests.pop_front();
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