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
  std::map<uint32_t, std::vector<std::shared_ptr<Downloader>>> m_piece_downloaders;
  std::map<std::shared_ptr<Downloader>, std::vector<uint32_t>> m_peer_pool;

  std::set<PeerContactInfo> m_active_connections;
  std::set<uint32_t> m_missing_pieces;

  std::shared_ptr<InternalContext> m_app_context;
  std::shared_ptr<IStorage> m_storage_device;

public:
  RandomPieceStrategy(std::shared_ptr<InternalContext> app_context,
                      std::shared_ptr<IStorage> storage_device);

  boost::asio::awaitable<void> include(
      const std::vector<PeerContactInfo>& potential_peers) override final;

  boost::asio::awaitable<void> assign() override final;

  boost::asio::awaitable<void> revoke() override final;

  boost::asio::awaitable<bool> is_done() override final;
};
}  // namespace btr