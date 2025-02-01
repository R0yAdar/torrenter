#include <cstdlib>
#include <fstream>
#include <iostream>
#include <print>
#include <regex>

#include "app.hpp"

#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <fmt/color.h>

#include "bitTorrent/metadata/bencode.hpp"
#include "bitTorrent/metadata/torrentfile.hpp"
#include "client/tracker/tracker.hpp"
#include "utp/packet.h"

using namespace boost;

App::App()
    : name {fmt::format("{}", "torrenter")}
{
  fmt::print(fg(fmt::color::aqua) | fmt::emphasis::bold | fmt::emphasis::italic,
             "Welcome to torrenter!\n");
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold
                 | fmt::emphasis::italic,
             "Built with:\n");
  fmt::print(fg(fmt::color::orange) | fmt::emphasis::italic,
             " *Boost Version: {}\n",
             BOOST_LIB_VERSION);
  fmt::print(fg(fmt::color::rebecca_purple) | fmt::emphasis::italic,
             " *FMT Version: {}\n",
             FMT_VERSION);
}

void App::Run()
{
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold
                 | fmt::emphasis::italic,
             "\nStarted running...\n");

  std::string torrent_file_path =
      "C:\\Users\\royad\\Downloads\\1929081.torrent";

  auto file = std::ifstream {torrent_file_path, std::ios::binary};
  std::stringstream file_contents {};
  file_contents << file.rdbuf();
  bencode::BDecoder decoder {};
  auto bencoded_torrent = decoder(file_contents.str());
  auto torrent = btr::load_torrent_file(bencoded_torrent);

  std::regex rgx(R"(udp://([a-zA-Z0-9.-]+):([0-9]+)/announce)");

  const btr::Torrenter torrenter {*torrent};

  if (torrent.has_value()) {
    for (auto tracker : torrent->trackers) {
      std::println("{}", tracker);

      std::smatch match;
      if (std::regex_search(tracker, match, rgx)) {
        std::string address = match[1];
        int16_t port = static_cast<int16_t>(std::stoi(match[2]));

        boost::asio::io_service io_service;
        boost::asio::ip::udp::resolver resolver(io_service);
        boost::asio::ip::udp::resolver::query query(
            boost::asio::ip::udp::v4(), address, std::to_string(port));
        boost::asio::ip::udp::resolver::iterator endpoints =
            resolver.resolve(query);
        boost::asio::ip::udp::endpoint endpoint = *endpoints;

        auto swarm = btr::udp_fetch_swarm(torrenter, endpoint.address(), port);

        auto count = 0;

        if (swarm) {
          for (const auto& peer : *swarm) {
            std::cout << peer << "  \n";
            boost::asio::ip::tcp::socket socket(io_service);

            try {
              socket.connect(
                  boost::asio::ip::tcp::endpoint(peer.address(), peer.port()));
              std::cout << "Connected \n";
              ++count;
              socket.close();
            } catch (std::exception) {
              std::cout << "Failed!\n";
            }
          }
        }
        std::println("total TCP client: {}", count);
      }

      char c;
      std::cin >> c;
    }


  } else {
    fmt::print("couldn't decode");
  }
}
