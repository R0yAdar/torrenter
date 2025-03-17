#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>

#include "app.hpp"

#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <fmt/color.h>
#include <openssl/opensslv.h>

#include "torrent/metadata/bencode.hpp"
#include "torrent/metadata/torrentfile.hpp"
#include "client/torrenter.hpp"

App::App(std::filesystem::path piece_vault_root)
    : m_piece_vault {std::make_shared<FileDirectoryStorage>(std::move(piece_vault_root))}
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

  fmt::print(fg(fmt::color::medium_violet_red) | fmt::emphasis::italic,
             " *OPENSSL Version: {}\n",
             OPENSSL_VERSION_STR);
}

void App::run(const std::string& torrent_file_path,
              const std::string& download_path)
{
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold
                 | fmt::emphasis::italic,
             "\nStarted running...\n");

  auto file = std::ifstream {torrent_file_path, std::ios::binary};

  std::stringstream file_contents {};
  file_contents << file.rdbuf();
  bencode::BDecoder decoder {};

  auto bencoded_torrent = decoder(file_contents.str());

  auto torrent = btr::load_torrent_file(bencoded_torrent);

  if (torrent.has_value()) {
    btr::Torrenter torrenter {*torrent};

    try {
      torrenter.download_file(download_path, m_piece_vault);
    } catch (std::exception& e) {
      std::cout << e.what() << std::endl;
    }

  } else {
    fmt::print("couldn't decode");
  }
}
