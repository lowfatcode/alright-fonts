cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set(CMAKE_C_STANDARD 11)

project(examples C)

include_directories(
  .
)

add_subdirectory(examples)
