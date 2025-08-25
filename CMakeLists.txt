cmake_minimum_required(VERSION 3.10)
project(perfect_hash_finder)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_BUILD_TYPE Debug)

add_executable(main perfect_hash_finder.c)

set_target_properties(main PROPERTIES OUTPUT_NAME perfect_hash_finder)
