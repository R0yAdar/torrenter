cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    torrenter
    VERSION 1.0.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    torrenter_lib OBJECT
    source/lib.cpp
 "source/bitTorrent/protocol.cpp" "source/bitTorrent/protocol.h")

target_include_directories(
    torrenter_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(torrenter_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(torrenter_lib PRIVATE fmt::fmt)

find_package(OpenSSL REQUIRED)
add_definitions(-DBOOST_ALL_NO_LIB) 
target_link_libraries(torrenter_lib PRIVATE openssl::openssl)

find_package(Boost REQUIRED COMPONENTS asio)
target_link_libraries(torrenter_lib PRIVATE boost::boost)

# ---- Declare executable ----

add_executable(torrenter_exe source/main.cpp "source/bitTorrent/protocol.cpp" "source/bitTorrent/protocol.h")
add_executable(torrenter::exe ALIAS torrenter_exe)

set_property(TARGET torrenter_exe PROPERTY OUTPUT_NAME torrenter)

target_compile_features(torrenter_exe PRIVATE cxx_std_20)

target_link_libraries(torrenter_exe PRIVATE torrenter_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----
return() # Causes boost problem
if(NOT torrenter_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of torrenter"
  )
endif()

include(cmake/dev-mode.cmake)
