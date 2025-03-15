#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>
#include <array>
#include <expected>

#include "bencode.hpp"

using bencode::BeValue;
using bencode::BeValueTypeIndex;

namespace btr
{
struct FileItem
{
  FileItem(std::string p_path, uint64_t p_size, uint64_t p_offset)
      : path {std::move(p_path)}
      , size {p_size}
      , offset {p_offset}
  {
  }

  std::string path;
  uint64_t size;
  uint64_t offset;
};

struct TorrentFileMetadata
{
  std::string directory_name;
  bool is_private;
  std::string comment;
  std::string created_by;
  std::chrono::duration<uint64_t> creation_date;
  std::string encoding_type;
};

struct TorrentFile
{
  TorrentFileMetadata metadata;

  std::array<uint8_t, 20> info_hash;
  std::vector<FileItem> files;
  std::vector<std::string> trackers;
  std::vector<std::vector<uint8_t>> piece_hashes;

  int64_t file_length;
  int64_t piece_length;
};

enum class TorrentFileParseError
{
    MissingField,
    InvalidField,
    Overflow
};

std::expected<TorrentFile, TorrentFileParseError> load_torrent_file(BeValue file);

}  // namespace btr