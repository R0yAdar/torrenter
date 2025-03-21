cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    torrenter
    VERSION 1.0.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL ""
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    torrenter_lib OBJECT
    "source/app.cpp" 
    "source/torrent/metadata/bencode.hpp" 
    "source/torrent/metadata/bencode.cpp"
    "source/torrent/metadata/torrentfile.hpp"
    "source/torrent/metadata/torrentfile.cpp"
    "source/torrent/bitfield/bitfield.hpp"
    "source/torrent/bitfield/bitfield.cpp"
    "source/torrent/messages.hpp" 
    "source/client/reactor/reactor.hpp"
    "source/client/reactor/reactor.cpp"
    
    "source/client/storage/storage.cpp"
    "source/client/storage/storage.hpp"

    "source/client/downloader/downloader.hpp"
    "source/client/downloader/downloader.cpp"
    
    "source/client/transmit/transmit.hpp"
    "source/client/transmit/transmit.cpp"
    "source/client/tracker/tracker.hpp"
    "source/client/tracker/tracker.cpp"
    "source/client/peer.hpp"
    "source/client/peer.cpp"
    "source/client/reactor/strategy/strategy.hpp"
    "source/client/reactor/strategy/strategy.cpp"
    "source/client/context.hpp"
    "source/auxiliary/peer_id.hpp"
     
    "source/client/torrenter.cpp")

target_include_directories(
    torrenter_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(torrenter_lib PUBLIC cxx_std_23)

find_package(fmt REQUIRED)
target_link_libraries(torrenter_lib PRIVATE fmt::fmt)

find_package(OpenSSL REQUIRED)
target_link_libraries(torrenter_lib PRIVATE openssl::openssl)

find_package(Boost REQUIRED)
target_link_libraries(torrenter_lib PRIVATE Boost::headers)

# ---- Declare executable ----

add_executable(torrenter_exe source/main.cpp)
add_executable(torrenter::exe ALIAS torrenter_exe)

set_property(TARGET torrenter_exe PROPERTY OUTPUT_NAME torrenter)

target_compile_features(torrenter_exe PRIVATE cxx_std_23)

target_link_libraries(torrenter_exe PRIVATE Boost::headers)
target_link_libraries(torrenter_exe PRIVATE torrenter_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT torrenter_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of torrenter"
  )
endif()

include(cmake/dev-mode.cmake)
