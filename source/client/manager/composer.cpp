#pragma once

#include "composer.hpp"

#include "auxiliary/log.hpp"

namespace btr
{
using namespace std::chrono_literals;

Composer::Composer(std::shared_ptr<InternalContext> context,
                   std::shared_ptr<IStorage> storage_device)
    : m_context {std::move(context)}
    , m_storage_device {std::move(storage_device)}
{
  m_strategy =
      std::make_unique<RandomPieceStrategy>(m_context, m_storage_device);
}

boost::asio::awaitable<void> Composer::download(
    std::string filepath, std::vector<Tracker>& trackers) const
{
  constexpr auto TIMEOUT_UDP_SWARM_FETCH = 5s;
  constexpr auto TIME_UDP_SWARM_WAIT = 6s;
  constexpr auto TIME_UDP_SWARM_INTERVAL = 15s;
  constexpr auto TIME_PEER_ASSIGNING_INTERVAL = 8s;

  auto io = co_await boost::asio::this_coro::executor;

  boost::asio::co_spawn(
      io,
      [&]() -> boost::asio::awaitable<void>
      {
        while (!co_await m_strategy->is_done()) {
          boost::asio::steady_timer timer(io);
          auto swarm = std::make_shared<std::vector<udp::endpoint>>();

          for (auto& tracker : trackers) {
            boost::asio::co_spawn(
                io,
                tracker.fetch_udp_swarm(io, swarm, TIMEOUT_UDP_SWARM_FETCH),
                boost::asio::detached);
          }

          timer.expires_from_now(TIME_UDP_SWARM_WAIT);
          co_await timer.async_wait(boost::asio::use_awaitable);

          std::vector<PeerContactInfo> peers;

          for (auto& ep : *swarm) {
            peers.emplace_back(ep.address(), ep.port());
          }

          co_await m_strategy->include(peers);
          
          timer.expires_from_now(TIME_UDP_SWARM_INTERVAL);
          co_await timer.async_wait(boost::asio::use_awaitable);
        }
      },
      boost::asio::detached);

  try {
    while (!co_await m_strategy->is_done()) {
      boost::asio::steady_timer timer(
          co_await boost::asio::this_coro::executor);
      timer.expires_from_now(TIME_PEER_ASSIGNING_INTERVAL);
      co_await timer.async_wait(boost::asio::use_awaitable);

      co_await m_strategy->revoke();
      co_await m_strategy->assign();
    }

    auto info_hash = m_context->info_hash_as_string();

    boost::asio::stream_file output_file {
        co_await boost::asio::this_coro::executor,
        filepath,
        boost::asio::stream_file::flags::write_only
            | boost::asio::stream_file::flags::create};

    uint32_t index = 0;

    std::vector<uint8_t> buffer(m_context->get_piece_size(0));
    
    while (m_storage_device->exists(info_hash, index)) {
      auto piece_size = m_context->get_piece_size(index);

      co_await m_storage_device->pull_piece(
          info_hash, index, 0, piece_size, buffer);

      co_await boost::asio::async_write(output_file,
                                        boost::asio::buffer(buffer, piece_size),
                                        boost::asio::use_awaitable);
      ++index;
    }
    Log::Success("Finished Writing Complete File To Disk");
  } catch (std::exception& ex) {
    Log::Error("Failed miserably: {}", ex.what());
  }
}
}  // namespace btr