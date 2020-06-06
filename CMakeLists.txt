cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(sed++)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add executable
add_executable(sed src/main.cpp)

# add dependencies
include(cmake/CPM.cmake)

CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  VERSION 6.1.2
  GIT_TAG 6.1.2
)
target_link_libraries(sed fmt)

CPMAddPackage(
  NAME scn
  GITHUB_REPOSITORY eliaskosunen/scnlib
  VERSION 0.3
)
target_link_libraries(sed scn::scn)

CPMAddPackage(
  NAME doctest
  GITHUB_REPOSITORY onqtam/doctest
  VERSION 2.3.8
  GIT_TAG 2.3.8
)
target_link_libraries(sed doctest)

CPMAddPackage(
  NAME magic_enum
  GITHUB_REPOSITORY Neargye/magic_enum
  VERSION 0.6.6
)
target_link_libraries(sed magic_enum)
