# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(torrenterTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(torrenter_test source/torrenter_test.cpp)
target_link_libraries(
    torrenter_test PRIVATE
    torrenter_lib
    Catch2::Catch2WithMain
)
target_compile_features(torrenter_test PRIVATE cxx_std_20)

catch_discover_tests(torrenter_test)

# ---- End-of-file commands ----

add_folders(Test)
