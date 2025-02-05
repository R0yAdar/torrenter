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

enum BeValueTypeIndex : uint8_t
{
    IInt64 = 0,
    IString = 1,
    IDict = 2,
    IList = 3,
};

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
  size_t used_chars;
};

class BDecoder
{
public:
  static const int DEFAULT_MAX_DEPTH = 10;

  BeValue operator()(std::string_view value, int maxDepth = BDecoder::DEFAULT_MAX_DEPTH) const;

private:
  DecodeResult decode_int(std::string_view value) const;
  DecodeResult decode_string(std::string_view value) const;
  DecodeResult decode_dict(std::string_view value, int maxDepth) const;
  DecodeResult decode_list(std::string_view value, int maxDepth) const;
  DecodeResult decode(std::string_view value, int maxDepth) const;
};

}  // namespace bencode