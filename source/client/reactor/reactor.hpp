#pragma once

#include <fstream>
#include <memory>
#include <queue>
#include <vector>

#include <boost/asio.hpp>

#include "client/context.hpp"
#include "client/storage/storage.hpp"
#include "client/tracker/tracker.hpp"
#include "strategy/strategy.hpp"

namespace btr
{
using namespace std::chrono_literals;
class Reactor
{
  std::shared_ptr<InternalContext> m_context;
  std::shared_ptr<IStorage> m_storage_device;
  std::unique_ptr<IStrategy> m_strategy;

public:
  Reactor(std::shared_ptr<InternalContext> context,
          std::shared_ptr<IStorage> storage_device)
      : m_context {std::move(context)}
      , m_storage_device {std::move(storage_device)}
  {
    m_strategy =
        std::make_unique<RandomPieceStrategy>(m_context, m_storage_device);
  }

  boost::asio::awaitable<void> download(std::string filepath,
                                        std::vector<Tracker>& trackers) const
  {
    auto io = co_await boost::asio::this_coro::executor;

    auto shared_io = std::make_shared<decltype(io)>(io);

    boost::asio::co_spawn(
        io,
        [&]() -> boost::asio::awaitable<void>
        {
          while (!co_await m_strategy->is_done()) {
            auto swarm = std::make_shared<std::vector<udp::endpoint>>();

            for (auto& tracker : trackers) {
              boost::asio::co_spawn(io,
                                    tracker.fetch_udp_swarm(io, swarm),
                                    boost::asio::detached);
            }

            boost::asio::steady_timer timer(io);
            timer.expires_from_now(8s);

            co_await timer.async_wait(boost::asio::use_awaitable);

            std::vector<PeerContactInfo> peers;

            for (auto& ep : *swarm) {
              peers.emplace_back(ep.address(), ep.port());
            }

            co_await m_strategy->include(peers);
          }
        },
        boost::asio::detached);

    try {
      {
        boost::asio::steady_timer timer(
            co_await boost::asio::this_coro::executor);
        timer.expires_from_now(10s);

        co_await timer.async_wait(boost::asio::use_awaitable);

        co_await m_strategy->assign();
      }

      std::cout << "Finished assigning\n";

      while (true) {
        boost::asio::steady_timer timer(
            co_await boost::asio::this_coro::executor);
        timer.expires_from_now(3s);

        co_await timer.async_wait(boost::asio::use_awaitable);

        co_await m_strategy->revoke();
        co_await m_strategy->assign();

        if (co_await m_strategy->is_done()) {
          std::cout << "I'm done!\n";
          break;
        }
      }

      auto info_hash = m_context->info_hash_as_string();

      boost::asio::stream_file output_file {
          co_await boost::asio::this_coro::executor,
          filepath,
          boost::asio::stream_file::flags::write_only
              | boost::asio::stream_file::flags::create};

      uint32_t index = 0;

      std::vector<uint8_t> buffer(m_context->get_piece_size(0));
      std::cout << "Finished downloading -> merging pieces\n";
      while (m_storage_device->exists(info_hash, index)) {
        auto piece_size = m_context->get_piece_size(index);

        co_await m_storage_device->pull_piece(
            info_hash, index, 0, piece_size, buffer);

        co_await boost::asio::async_write(
            output_file,
            boost::asio::buffer(buffer, piece_size),
            boost::asio::use_awaitable);
        ++index;
      }
    } catch (std::exception& ex) {
      std::cerr << ex.what() << '\n';
    }
  }
};
}  // namespace btr