cmake_minimum_required(VERSION 3.20)
project(GameCatEmu)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL3 REQUIRED CONFIG)

add_executable(GameCatEmu src/main.cpp)

target_link_libraries(GameCatEmu PRIVATE SDL3::SDL3)