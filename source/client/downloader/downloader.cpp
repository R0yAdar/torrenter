#include <variant>
#include <iostream>

#include "downloader.hpp"

#include "auxiliary/variant_aux.hpp"

namespace btr
{
Downloader::Downloader(std::shared_ptr<const InternalContext> context,
                       std::shared_ptr<Peer> peer,
                       DownloaderPolicy policy)
    : m_application_context {std::move(context)}
    , m_peer {std::move(peer)}
    , m_policy {policy}
    , m_callback {std::make_shared<message_callback>(
          [this](TorrentMessage m) -> boost::asio::awaitable<void>
          { co_await triggered_on_received_message(m); })}
{
  m_peer->add_callback({m_callback});
}

const ExternalPeerContext& Downloader::get_context() const
{
  return m_peer->get_context();
}

const PeerActivity& Downloader::get_activity() const
{
  return m_peer->get_activity();
}


boost::asio::awaitable<bool> Downloader::download_piece(uint32_t index)
{
  std::cout << "Downloader - " << m_peer->get_context().contact_info.address
            << ": " << index << "(piece)"
            << std::endl;
  if (!m_peer->get_context().status.remote_bitfield.get(index)) {
    co_return false;
  }

  m_pieces[index] = FilePiece {
      .index = index,
      .status = PieceStatus::Pending,
      .data =
          std::vector<uint8_t>(m_application_context->get_piece_size(index)),
      .bytes_downloaded = 0};

  if (m_peer->get_context().status.self_choked) {
    co_await m_peer->send_async(TorrentMessage {Interested {}});
  }

  auto piece_length = m_application_context->get_piece_size(index);

  for (uint32_t offset = 0; offset < piece_length;
       offset += m_policy.max_block_bytes)
  {
    auto req = Request {};
    req.piece_index = index;
    req.offset_within_piece = offset;
    req.length = std::min(piece_length - offset, m_policy.max_block_bytes);

    m_pending_requests.emplace_back(req);
  }

  co_await send_buffered_messages();

  co_return true;
}

void Downloader::handle_piece(const Piece& piece)
{
  if (!m_pieces.contains(piece.get_metadata().piece_index)) {
    return;
  }

  auto data = piece.get_payload();
  m_active_requests.erase(
      RequestIdentifier {piece.get_metadata().piece_index,
                         piece.get_metadata().offset_within_piece,
                         static_cast<uint32_t>(data.size())});

  FilePiece& current_piece = m_pieces[piece.get_metadata().piece_index];

  std::ranges::copy(
      data,
      current_piece.data.begin() + piece.get_metadata().offset_within_piece);

  current_piece.bytes_downloaded += data.size();

  if (current_piece.bytes_downloaded == current_piece.data.size()) {
    current_piece.status = PieceStatus::Complete;
  } else {
    current_piece.status = PieceStatus::Active;
  }
}

boost::asio::awaitable<void> Downloader::triggered_on_received_message(
    TorrentMessage trigger)
{
  bool should_try_sending_messages = false;
  bool should_choke_active_requests = false;
  bool should_handle_piece = false;

  std::visit(
      overloaded {
          [](auto&) {},
          [&should_try_sending_messages](Unchoke)
          { should_try_sending_messages = true; },
          [&should_try_sending_messages, &should_choke_active_requests](Choke)
          {
            should_choke_active_requests = true;
            should_try_sending_messages = true;
          },
          [&should_try_sending_messages, &should_handle_piece](Piece)
          {
            should_handle_piece = true;
            should_try_sending_messages = true;
          }},
      trigger);

  if (should_handle_piece) {
    handle_piece(std::get<Piece>(trigger));
  }

  if (should_choke_active_requests) {
    for (auto& identifier : m_active_requests) {
      m_pending_requests.emplace_front(identifier.piece_index,
                                       identifier.piece_offset,
                                       identifier.block_length);
    }

    m_active_requests.clear();
  }

  if (should_try_sending_messages) {
    co_await send_buffered_messages();
  }
}

boost::asio::awaitable<void> Downloader::send_buffered_messages()
{
  while (
      !m_peer->get_context().status.self_choked && !m_pending_requests.empty()
      && m_active_requests.size() < m_policy.max_concurrent_outgoing_requests)
  {
    auto req = m_pending_requests.front();

    m_active_requests.emplace(
        req.piece_index, req.offset_within_piece, req.length);
    m_pending_requests.pop_front();

    co_await m_peer->send_async(TorrentMessage {req});
  }
}

boost::asio::awaitable<std::optional<FilePiece>> Downloader::retrieve_piece(
    size_t index)
{
  if (!m_pieces.contains(index)) {
    co_return std::nullopt;
  }

  if (m_pieces[index].status == PieceStatus::Complete) {
    co_return m_pieces.extract(m_pieces.find(index)).mapped();
  }

  co_return FilePiece {.index = index,
                       .status = m_pieces[index].status,
                       .bytes_downloaded = m_pieces[index].bytes_downloaded};
}

boost::asio::awaitable<void> Downloader::restart_connection()
{
  return m_peer->start_async();
}


}  // namespace btr