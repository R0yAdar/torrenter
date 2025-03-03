#include "downloader.hpp"

namespace btr
{
Downloader::Downloader(std::shared_ptr<Peer> peer,
                       std::shared_ptr<const InternalContext> context)
    : m_peer {std::move(peer)}
    , m_application_context {std::move(context)}
{
  m_peer->add_callback([this](TorrentMessage m) -> boost::asio::awaitable<void>
                       { co_await triggered_on_received_message(m); });
}

const ExternalPeerContext& Downloader::get_context() const
{
  return m_peer->get_context();
}

boost::asio::awaitable<bool> Downloader::download_piece(size_t index)
{
  std::cout << "Downloading piece: " << index << '\n';

  if (!m_peer->get_context().status.remote_bitfield.get(index)) {
    co_return false;
  }

  m_pieces[index] = FilePiece {
      index,
      PieceStatus::Pending,
      std::vector<uint8_t>(m_application_context->get_piece_size(index)),
      0};

  if (m_peer->get_context().status.self_choked) {
    co_await m_peer->send_async(TorrentMessage {Interested {}});
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

  co_await send_buffered_messages();

  std::cout << "Pending requests " << m_pending_requests.size() << std::endl;

  co_return true;
}

void Downloader::handle_piece(const Piece& piece)
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

  if (current_piece.bytes_downloaded > current_piece.data.size()) {
    std::cout << "EXCEEDED PIECE SIZE BY (MAYBE CHOKE RELATED...) "
              << current_piece.data.size() << std::endl;
  }

  if (current_piece.bytes_downloaded == current_piece.data.size()) {
    current_piece.status = PieceStatus::Complete;
  } else {
    current_piece.status = PieceStatus::Active;
  }
}


boost::asio::awaitable<void> Downloader::triggered_on_received_message(
    TorrentMessage trigger)
{
  if (trigger.index() == 0) {
    co_await m_peer->send_async(TorrentMessage {Keepalive {}});
  } else if (trigger.index() == ID_UNCHOKE + 1) {
    co_await send_buffered_messages();
  } else if (trigger.index() == ID_CHOKE + 1) {
    for (auto& reqi : m_active_requests) {
      auto req = Request {};
      req.piece_index = reqi.piece_index;
      req.offset_within_piece = reqi.piece_offset;
      req.length = reqi.block_length;
      m_pending_requests.push_front(req);
    }

    m_active_requests.clear();
  } else if (trigger.index() == ID_PIECE + 1) {
    handle_piece(std::get<Piece>(trigger));
    co_await send_buffered_messages();
  }
}

boost::asio::awaitable<void> Downloader::send_buffered_messages()
{
  while (!m_peer->get_context().status.self_choked && !m_pending_requests.empty()
         && m_active_requests.size() < 7)
  {
    auto req = m_pending_requests.front();
    std::cout << "Sending request for: " << req.piece_index << " at offset "
              << req.offset_within_piece << "  and length of " << req.length
              << std::endl;

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

}  // namespace btr