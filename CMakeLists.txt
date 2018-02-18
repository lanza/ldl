cmake_minimum_required(VERSION 3.10)

project(linker CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CTEST_OUTPUT_ON_FAILURE ON)

include_directories(
  ${CMAKE_SOURCE_DIR}/cget/include
  ${CMAKE_SOURCE_DIR}/include
  )
link_directories(${CMAKE_SOURCE_DIR}/cget/lib)

add_executable(linker linker.cpp)

add_subdirectory(lib)
enable_testing()
add_subdirectory(tests)
