#include <catch2/catch_test_macros.hpp>
#include "bitTorrent/bencode.hpp"

TEST_CASE("Decodes int correctly", "[library]")
{
  bencode::BDecoder decoder;
  REQUIRE(boost::get<std::int64_t>(decoder("i244321313e")) == 244321313);
  REQUIRE(boost::get<std::int64_t>(decoder("i723394819e")) == 723394819);
  REQUIRE_THROWS_AS(decoder("i32984"), std::invalid_argument);
}

TEST_CASE("Decodes string correctly", "[library]")
{
  bencode::BDecoder decoder;
  REQUIRE(boost::get<std::string>(decoder("5:hello")) == "hello");
  REQUIRE(boost::get<std::string>(decoder("1:a")) == "a");
  REQUIRE_THROWS_AS(decoder("10:"), std::invalid_argument);
}