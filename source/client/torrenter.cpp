#include <filesystem>
#include <memory>
#include <regex>

#include <boost/asio.hpp>

#include "client/torrenter.hpp"
#include "client/peer.hpp"
#include "client/tracker/tracker.hpp"
#include "client/manager/composer.hpp"
#include "auxiliary/log.hpp"

namespace btr
{
Torrenter::Torrenter(TorrentFile torrent)
    : m_torrent {std::move(torrent)}
{
}

void Torrenter::download_file(std::filesystem::path download_path, std::shared_ptr<IStorage> storage_device) const
{
  std::regex udp_tracker_uri_regex(R"(udp://([a-zA-Z0-9.-]+):([0-9]+)/announce)");

  std::set<boost::asio::ip::udp::endpoint> endpoints {};
  std::smatch udp_tracker_uri;

  io_context resolve_io {};

  for (auto tracker : m_torrent.trackers) {
    if (std::regex_search(tracker, udp_tracker_uri, udp_tracker_uri_regex)) {
      std::string address = udp_tracker_uri[1];
      int16_t port = static_cast<int16_t>(std::stoi(udp_tracker_uri[2]));

      boost::asio::co_spawn(
          resolve_io,
          [&resolve_io, &endpoints, address, port]()
              -> boost::asio::awaitable<void>
          {
            boost::asio::ip::udp::resolver resolver(resolve_io);
            boost::asio::ip::udp::resolver::query dns_query(
                boost::asio::ip::udp::v4(), address, std::to_string(port));
            boost::asio::ip::udp::endpoint endpoint =
                *(co_await resolver.async_resolve(dns_query));
            endpoints.insert(endpoint);
          },
          boost::asio::detached);
    }
  }

  resolve_io.run();

  std::vector<PeerContactInfo> peers;

  io_context io {};

  auto context = std::make_shared<InternalContext>();

  context->info_hash = std::vector(m_torrent.info_hash.cbegin(),
                                            m_torrent.info_hash.cend());
  context->piece_hashes = m_torrent.piece_hashes;
  context->file_size = m_torrent.file_length;
  context->piece_size = m_torrent.piece_length;
  context->piece_count = static_cast<uint32_t>(m_torrent.piece_hashes.size());

  Log::Info("Torrent file-size is: {}bytes", context->file_size);
  Log::Info("Torrent piece-size is: {}bytes", context->piece_size);
  Log::Info("Torrent piece-count is: {}", context->piece_count);

  Log::Info("Parsed the following udp trackers:");

  std::vector<Tracker> trackers;
  for (auto& endpoint : endpoints) {
    trackers.emplace_back(context, endpoint.address(), endpoint.port());
    Log::Info("UDP:({}, {})", endpoint.address().to_string(), endpoint.port());
  }

  Composer composer {context, std::move(storage_device)};

  boost::asio::co_spawn(io,
                        composer.download(download_path.generic_string(), trackers),
                        boost::asio::detached);

  io.run();
}
}  // namespace btr