#include <algorithm>
#include <iostream>
#include <string_view>

#include "torrentfile.hpp"
#include <openssl/sha.h>

using bencode::Dict;
using bencode::List;

namespace btr
{
template<typename T, BeValueTypeIndex Index>
std::expected<T, TorrentFileParseError> parse_field(
    const Dict& dict, const std::string& field_name)
{
  if (!dict.contains(field_name))
    return std::unexpected {TorrentFileParseError::MissingField};

  const auto& value = dict.at(field_name);

  if (value.which() != Index)
    return std::unexpected {TorrentFileParseError::InvalidField};

  return boost::get<T>(value);
}

std::expected<std::string, TorrentFileParseError> parse_string_field(
    const Dict& dict, const std::string& field_name)
{
  return parse_field<std::string, BeValueTypeIndex::IString>(dict, field_name);
}

std::expected<int64_t, TorrentFileParseError> parse_int_field(
    const Dict& dict, const std::string& field_name)
{
  return parse_field<int64_t, BeValueTypeIndex::IInt64>(dict, field_name);
}

std::expected<List, TorrentFileParseError> parse_list_field(
    const Dict& dict, const std::string& field_name)
{
  return parse_field<List, BeValueTypeIndex::IList>(dict, field_name);
}

std::expected<Dict, TorrentFileParseError> parse_dict_field(
    const Dict& dict, const std::string& field_name)
{
  return parse_field<Dict, BeValueTypeIndex::IDict>(dict, field_name);
}

std::expected<std::vector<std::string>, TorrentFileParseError> parse_announcers(
    BeValue announce)
{
  if (announce.which() == BeValueTypeIndex::IString) {
    return std::vector<std::string> {boost::get<std::string>(announce)};
  }

  if (announce.which() == BeValueTypeIndex::IList) {
    auto announcers = std::vector<std::string>();
    auto list = boost::get<List>(announce);

    for (auto item : list) {
      if (item.which() != BeValueTypeIndex::IString)
        return std::unexpected {TorrentFileParseError::InvalidField};
      announcers.push_back(boost::get<std::string>(item));
    }

    return announcers;
  }

  return std::unexpected {TorrentFileParseError::InvalidField};
}

std::expected<TorrentFile, TorrentFileParseError> load_torrent_file(BeValue file)
{
  TorrentFile torrent;

  // Ensure the top-level structure is a dictionary
  if (file.which() != static_cast<int8_t>(BeValueTypeIndex::IDict)) {
    return std::unexpected {TorrentFileParseError::InvalidField};
  }
  const auto& toplevel = boost::get<Dict>(file);

  // Parse "announce" (required)
  auto announce_result = parse_string_field(toplevel, "announce");
  if (!announce_result) {
    return std::unexpected {announce_result.error()};
  }
  torrent.trackers.push_back(std::move(*announce_result));

  // Parse "announce-list" (optional)
  if (toplevel.contains("announce-list")) {
    auto announce_list_result = parse_list_field(toplevel, "announce-list");
    if (announce_list_result) {
      for (const auto& sublist : *announce_list_result) {
        if (sublist.which() != BeValueTypeIndex::IList) {
          return std::unexpected {TorrentFileParseError::InvalidField};
        }
        const auto& sublist_items = boost::get<List>(sublist);
        for (const auto& item : sublist_items) {
          if (item.which() != BeValueTypeIndex::IString) {
            return std::unexpected {TorrentFileParseError::InvalidField};
          }
          torrent.trackers.push_back(boost::get<std::string>(item));
        }
      }
    }
  }

  // Parse "info" (required)
  auto info_result = parse_dict_field(toplevel, "info");
  if (!info_result) {
    return std::unexpected {info_result.error()};
  }
  const auto& info = *info_result;

  bencode::BEncoder encoder {};
  auto encoded_info = encoder(info);
  SHA1(reinterpret_cast<const unsigned char*>(encoded_info.data()),
       encoded_info.length(),
       torrent.info_hash.data());

  // Parse "name" (required)
  auto name_result = parse_string_field(info, "name");
  if (!name_result) {
    return std::unexpected {TorrentFileParseError::MissingField};
  }
  torrent.metadata.directory_name = std::move(*name_result);

  // Parse "piece length" (required)
  auto piece_length_result = parse_int_field(info, "piece length");
  if (!piece_length_result) {
    return std::unexpected {TorrentFileParseError::MissingField};
  }
  torrent.piece_length = *piece_length_result;

  // Parse "pieces" (required)
  auto pieces_result = parse_string_field(info, "pieces");
  if (!pieces_result) {
    return std::unexpected {TorrentFileParseError::MissingField};
  }
  const std::string& pieces_str = *pieces_result;

  // Split "pieces" into 20-byte hashes
  for (size_t i = 0; i < pieces_str.size(); i += 20) {
    if (i + 20 > pieces_str.size()) {
      return std::unexpected {TorrentFileParseError::InvalidField};
    }
    std::vector<uint8_t> hash(pieces_str.data() + i,
                              pieces_str.data() + i + 20);
    torrent.piece_hashes.push_back(PieceHash(std::move(hash)));
  }

  // Parse "files" (multi-file torrents) or "length" (single-file torrents)
  if (info.contains("files")) {
    auto files_result = parse_list_field(info, "files");
    if (!files_result) {
      return std::unexpected {files_result.error()};
    }
    uint64_t offset = 0;
    for (const auto& file_entry : *files_result) {
      if (file_entry.which() != BeValueTypeIndex::IDict) {
        return std::unexpected {TorrentFileParseError::InvalidField};
      }
      const auto& file_dict = boost::get<Dict>(file_entry);

      auto length_result = parse_int_field(file_dict, "length");
      auto path_result = parse_list_field(file_dict, "path");
      if (!length_result || !path_result) {
        return std::unexpected {TorrentFileParseError::InvalidField};
      }

      std::string full_path;
      for (const auto& path_part : *path_result) {
        if (path_part.which() != BeValueTypeIndex::IString) {
          return std::unexpected {TorrentFileParseError::InvalidField};
        }
        full_path += "/" + boost::get<std::string>(path_part);
      }

      torrent.files.push_back(
          FileItem(std::move(full_path), *length_result, offset));
      offset += *length_result;
    }
  } else if (info.contains("length")) {
    auto length_result = parse_int_field(info, "length");
    if (!length_result) {
      return std::unexpected {TorrentFileParseError::MissingField};
    }
    torrent.file_length = *length_result;
    torrent.files.push_back(
        FileItem(torrent.metadata.directory_name, *length_result, 0));
  } else {
    return std::unexpected {TorrentFileParseError::MissingField};
  }

  // Parse optional metadata fields
  if (toplevel.contains("comment")) {
    auto comment_result = parse_string_field(toplevel, "comment");
    if (comment_result) {
      torrent.metadata.comment = std::move(*comment_result);
    }
  }
  if (toplevel.contains("created by")) {
    auto created_by_result = parse_string_field(toplevel, "created by");
    if (created_by_result) {
      torrent.metadata.created_by = std::move(*created_by_result);
    }
  }
  if (toplevel.contains("creation date")) {
    auto creation_date_result = parse_int_field(toplevel, "creation date");
    if (creation_date_result) {
      torrent.metadata.creation_date =
          std::chrono::seconds(*creation_date_result);
    }
  }
  if (toplevel.contains("encoding")) {
    auto encoding_result = parse_string_field(toplevel, "encoding");
    if (encoding_result) {
      torrent.metadata.encoding_type = std::move(*encoding_result);
    }
  }

  // Return the fully populated TorrentFile
  return torrent;
}

}  // namespace btr