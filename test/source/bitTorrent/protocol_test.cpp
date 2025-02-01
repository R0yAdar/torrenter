#include <catch2/catch_test_macros.hpp>
#include <string>

#include "bitTorrent/messages.hpp"

TEST_CASE("Handshake", "[library]")
{
  REQUIRE(sizeof(btr::Handshake) == 60);

  btr::Handshake handshake {};

  REQUIRE(handshake.plen == sizeof(handshake.pname));

  // std::string is required to avoid null terminator at the end of the string literal
  REQUIRE(handshake.pname.m_data == std::string("BitTorrent protocol"));
}
