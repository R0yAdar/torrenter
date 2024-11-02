#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/variant.hpp>

namespace bencode
{
using BeValue = boost::make_recursive_variant<
    std::int64_t,
    std::string,
    std::map<std::string, boost::recursive_variant_>,
    std::vector<boost::recursive_variant_>>::type;

using List = std::vector<BeValue>;

using Dict = std::map<std::string, BeValue>;

class BEncoder : public boost::static_visitor<std::string>
{
public:
  std::string operator()(BeValue value) const;
  std::string operator()(std::int64_t value) const;
  std::string operator()(std::string value) const;
  std::string operator()(const List& values) const;
  std::string operator()(const Dict& values) const;
};

struct DecodeResult
{
  BeValue result;
  size_t usedChars;
};

class BDecoder
{
public:
  BeValue operator()(std::string_view value) const;

private:
  DecodeResult decodeInt(std::string_view value) const;
  DecodeResult decodeString(std::string_view value) const;
  DecodeResult decodeDict(std::string_view value) const;
  DecodeResult decodeList(std::string_view value) const;
  DecodeResult decode(std::string_view value) const;
};

}  // namespace bencode