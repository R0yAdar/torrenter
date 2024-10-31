#include "lib.hpp"
#include <openssl/opensslv.h>
#include <fmt/color.h>
#include <iostream>
#include <boost/version.hpp>
#include <boost/asio.hpp>

using namespace boost;

Library::Library()
    : name {fmt::format("{}", "torrenter")}
{
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold,
             "Built with:\n");
  fmt::print(fg(fmt::color::dark_red) | fmt::emphasis::italic, " *{}\n",
      OPENSSL_VERSION_TEXT);
  fmt::print(fg(fmt::color::orange) | fmt::emphasis::italic, 
      " *Boost Version: {}\n", BOOST_LIB_VERSION);
  fmt::print(fg(fmt::color::rebecca_purple) | fmt::emphasis::italic, 
      " *FMT Version: {}\n",
             FMT_VERSION);
}

void Library::Run() {
  fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold,
             "\nStarted running...\n");
}
