#include "app.hpp"
#include <openssl/opensslv.h>
#include <fmt/color.h>
#include <iostream>
#include <boost/version.hpp>
#include <boost/asio.hpp>

#include "bitTorrent/bencode.hpp"

using namespace boost;

App::App()
    : name {fmt::format("{}", "torrenter")}
{
  fmt::print(fg(fmt::color::aqua) | fmt::emphasis::bold | fmt::emphasis::italic,
             "Welcome to torrenter!\n");
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold
                 | fmt::emphasis::italic,
             "Built with:\n");
  fmt::print(fg(fmt::color::dark_red) | fmt::emphasis::italic, " *{}\n",
      OPENSSL_VERSION_TEXT);
  fmt::print(fg(fmt::color::orange) | fmt::emphasis::italic, 
      " *Boost Version: {}\n", BOOST_LIB_VERSION);
  fmt::print(fg(fmt::color::rebecca_purple) | fmt::emphasis::italic, 
      " *FMT Version: {}\n",
             FMT_VERSION);
}

void App::Run()
{
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold | fmt::emphasis::italic,
             "\nStarted running...\n");

  std::string check = "checku";
  int value = 2834;
  bencode::BEncoder encoder;
  bencode::BDecoder decoder;
  bencode::Dict dict {{"hellp", "brah"}};
  bencode::List vikk {28, check, dict};

  std::cout << encoder(check) << '\n'
            << encoder(value) << '\n'
            << encoder(dict) << '\n'
            << encoder(vikk) << '\n';

  std::cout << encoder(decoder(encoder(check))) << '\n'
  << encoder(decoder(encoder(value))) << '\n'
  << encoder(decoder(encoder(dict))) << '\n'
  << encoder(decoder(encoder(vikk))) << '\n';

}
