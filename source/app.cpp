#include <cstdlib>
#include <fstream>
#include <iostream>
#include <print>
#include <regex>

#include "app.hpp"

#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <fmt/color.h>

#include "torrent/metadata/bencode.hpp"
#include "torrent/metadata/torrentfile.hpp"
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
      "C:\\Users\\royad\\Downloads\\sever5.torrent";

  auto file = std::ifstream {torrent_file_path, std::ios::binary};
  std::stringstream file_contents {};
  file_contents << file.rdbuf();
  bencode::BDecoder decoder {};
  auto bencoded_torrent = decoder(file_contents.str());
  auto torrent = btr::load_torrent_file(bencoded_torrent);
  
  if (torrent.has_value()) {
    btr::Torrenter torrenter {*torrent};
    torrenter.download_file("C:\\Users\\royad\\Downloads\\s3d.mp4");

  } else {
    fmt::print("couldn't decode");
  }
}
