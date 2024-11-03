#include <catch2/catch_test_macros.hpp>
#include "bitTorrent/bencode.hpp"

TEST_CASE("Decodes int correctly", "[library]")
{
  bencode::BDecoder decoder;
  REQUIRE(boost::get<std::int64_t>(decoder("i244321313e")) == 244321313);
}