#pragma once

#include "strategy.hpp"

#include "auxiliary/log.hpp"

namespace btr
{
RandomPieceStrategy::RandomPieceStrategy(
    std::shared_ptr<InternalContext> app_context,
    std::shared_ptr<IStorage> storage_device)
    : m_app_context {std::move(app_context)}
    , m_storage_device {std::move(storage_device)}
{
  for (uint32_t i = 0; i < m_app_context->piece_count; i++) {
    if (!m_storage_device->exists(m_app_context->info_hash_as_string(), i)) {
      m_missing_pieces.insert(i);
    }
  }
}

boost::asio::awaitable<void> RandomPieceStrategy::include(
    const std::vector<PeerContactInfo>& potential_peers)
{
  auto io = co_await boost::asio::this_coro::executor;
  Log::Info("Adding Potential Peers:");

  for (const auto& contact : potential_peers) {
    if (!m_active_connections.contains(contact)) {
      Log::Info("New Peer: ({},{})", contact.address.to_string(), contact.port);

      m_active_connections.insert(contact);
      auto peer = std::make_shared<Peer>(m_app_context, contact, io);
      auto downloader = std::make_shared<Downloader>(m_app_context, peer);

      m_peer_pool[downloader] = {};

      boost::asio::co_spawn(io, peer->start_async(), boost::asio::detached);
    }
  }
}

boost::asio::awaitable<void> RandomPieceStrategy::assign()
{
  constexpr auto MAX_PIECES_PER_DOWNLOADER = 5;
  constexpr auto DEFAULT_REDUNDANCY_PER_PIECE = 2;
  constexpr auto DEFAULT_ENDGAME_PIECE_COUNT = 50;

  std::vector missing_pieces(m_missing_pieces.cbegin(),
                             m_missing_pieces.cend());
  static std::random_device rd;
  static std::mt19937 rand_generator(rd());

  std::ranges::shuffle(
      missing_pieces.begin(), missing_pieces.end(), rand_generator);

  auto max_redundancy_per_piece =
      m_missing_pieces.size() <= DEFAULT_ENDGAME_PIECE_COUNT
      ? m_peer_pool.size() / missing_pieces.size()
      : DEFAULT_REDUNDANCY_PER_PIECE;

  for (size_t i = 0; i < max_redundancy_per_piece; i++) {
    for (auto piece_index : missing_pieces) {
      if (m_piece_downloaders[piece_index].size() >= max_redundancy_per_piece)
      {
        continue;
      }

      for (auto& [downloader, assigned_pieces] : m_peer_pool) {
        if (!std::ranges::contains(m_piece_downloaders[piece_index], downloader)
            && downloader->get_context().status.remote_bitfield.get(piece_index)
            && assigned_pieces.size() < MAX_PIECES_PER_DOWNLOADER)
        {
          if (co_await downloader->download_piece(piece_index)) {
            assigned_pieces.push_back(piece_index);
            m_piece_downloaders[piece_index].push_back(downloader);
            break;
          }
        }
      }
    }
  }
}

boost::asio::awaitable<void> RandomPieceStrategy::revoke()
{
  std::vector<std::shared_ptr<Downloader>> downloaders_to_remove {};

  for (auto& [downloader, pieces] : m_peer_pool) {
    if (!downloader->get_activity().is_active) {
      for (auto piece : pieces) {
        auto& piece_downloaders = m_piece_downloaders[piece];

        piece_downloaders.erase(
            std::remove(
                piece_downloaders.begin(), piece_downloaders.end(), downloader),
            piece_downloaders.end());

        downloaders_to_remove.push_back(downloader);
      }
    }
  }

  for (auto& downloader : downloaders_to_remove) {
    m_peer_pool.erase(downloader);
    m_active_connections.erase(downloader->get_context().contact_info);
  }

  co_return;
}

boost::asio::awaitable<bool> RandomPieceStrategy::is_done()
{
  std::vector<uint32_t> completed_indexes {};

  for (auto& [index, downloaders] : m_piece_downloaders) {
    for (auto downloader : downloaders) {
      if (auto piece = co_await downloader->retrieve_piece(index)) {
        switch (piece->status) {
          case PieceStatus::Complete:
            Log::Success("Completed piece: {}", index);
            co_await m_storage_device->push_piece(
                m_app_context->info_hash_as_string(), index, piece->data);

            m_peer_pool[downloader].erase(
                std::remove(m_peer_pool[downloader].begin(),
                            m_peer_pool[downloader].end(),
                            index));
            completed_indexes.push_back(index);

            m_missing_pieces.erase(index);
            break;

          case PieceStatus::Corrupt:
            co_await downloader->download_piece(index);
            Log::Warn("Coruptted piece: {}", index);
            break;

          default:
            break;
        }
      }
    }
  }

  for (auto index : completed_indexes) {
    m_piece_downloaders.erase(index);
  }

  Log::Info("Remaining: {} pieces", m_missing_pieces.size());

  co_return m_missing_pieces.empty();
}
}  // namespace btr