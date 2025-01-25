cmake_minimum_required(VERSION 3.10)
project(simulator VERSION 1.0.0)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOS true)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

if(${MACOS})
  set(CMAKE_CXX_COMPILER /usr/bin/clang++)
endif()


add_executable(simulator main.cpp)

add_subdirectory(engine)
add_subdirectory(math)

target_link_libraries(simulator PRIVATE engine)
target_precompile_headers(simulator REUSE_FROM engine)
