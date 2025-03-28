#pragma once

#include <algorithm>
#include <map>
#include <ranges>
#include <set>

#include "client/downloader/downloader.hpp"
#include "client/peer.hpp"
#include "client/storage/storage.hpp"

namespace btr
{

class IStrategy
{
public:
  virtual boost::asio::awaitable<void> include(
      const std::vector<PeerContactInfo>& potential_peers) = 0;

  virtual boost::asio::awaitable<void> assign() = 0;

  virtual boost::asio::awaitable<void> revoke() = 0;

  virtual boost::asio::awaitable<bool> is_done() = 0;

  virtual ~IStrategy() = default;
};

class RandomPieceStrategy : public IStrategy
{
  std::map<uint32_t, std::vector<std::shared_ptr<Downloader>>>
      m_piece_downloaders;
  std::map<std::shared_ptr<Downloader>, std::vector<uint32_t>> m_peer_pool;

  std::set<PeerContactInfo> m_active_connections;
  std::set<uint32_t> m_missing_pieces;

  std::shared_ptr<InternalContext> m_app_context;
  std::shared_ptr<IStorage> m_storage_device;

public:
  RandomPieceStrategy(std::shared_ptr<InternalContext> app_context,
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

  boost::asio::awaitable<void> include(
      const std::vector<PeerContactInfo>& potential_peers) override final
  {
    auto io = co_await boost::asio::this_coro::executor;

    for (const auto& contact : potential_peers) {
      if (!m_active_connections.contains(contact)) {
        m_active_connections.insert(contact);
        auto peer = std::make_shared<Peer>(m_app_context, contact, io);
        auto downloader = std::make_shared<Downloader>(m_app_context, peer);

        m_peer_pool[downloader] = {};

        boost::asio::co_spawn(io, peer->start_async(), boost::asio::detached);
      }
    }
  }

  boost::asio::awaitable<void> assign() override final
  {
    std::vector missing_pieces(m_missing_pieces.cbegin(),
                                         m_missing_pieces.cend());
    static std::random_device rd;
    static std::mt19937 rand_generator(rd());

    std::ranges::shuffle(missing_pieces.begin(), missing_pieces.end(), rand_generator);

    for (size_t i = 0; i < 2; i++) {
      for (auto piece_index : missing_pieces) {
        if (m_piece_downloaders[piece_index].size() < 2) {
          for (auto& [downloader, assigned_pieces] : m_peer_pool) {
            if (std::ranges::contains(m_piece_downloaders[piece_index],
                                      downloader))
            {
              continue;
            }
            if (downloader->get_context().status.remote_bitfield.get(
                    piece_index)
                && assigned_pieces.size() < 2)
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
  }

  boost::asio::awaitable<void> revoke() override final
  {
    std::vector<std::shared_ptr<Downloader>> downloaders_to_remove {};

    for (auto& [downloader, pieces] : m_peer_pool) {
      if (!downloader->get_activity().is_active) {
        for (auto piece : pieces) {
          auto& piece_downloaders = m_piece_downloaders[piece];

          piece_downloaders.erase(std::remove(piece_downloaders.begin(),
                                              piece_downloaders.end(),
                                              downloader),
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

  boost::asio::awaitable<bool> is_done() override final
  {
    std::vector<uint32_t> completed_indexes {};

    for (auto& [index, downloaders] : m_piece_downloaders) {
      for (auto downloader : downloaders) {
        if (auto piece = co_await downloader->retrieve_piece(index)) {
          switch (piece->status) {
            case PieceStatus::Complete:
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

    co_return m_missing_pieces.empty();
  }
};
}  // namespace btr