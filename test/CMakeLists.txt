# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(torrenterTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

find_package(Boost REQUIRED COMPONENTS headers)

# ---- Tests ----

add_executable(torrenter_test "source/bitTorrent/bencode_test.cpp" "source/bitTorrent/protocol_test.cpp")

# Important to have that before any link to boost or a program that uses boost:
target_link_libraries(torrenter_test PRIVATE Catch2::Catch2WithMain) 
target_link_libraries(torrenter_test PRIVATE Boost::headers)

target_link_libraries(torrenter_test PRIVATE torrenter_lib)

target_compile_features(torrenter_test PRIVATE cxx_std_23)

catch_discover_tests(torrenter_test)

# ---- End-of-file commands ----

add_folders(Test)
