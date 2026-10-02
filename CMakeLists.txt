cmake_minimum_required(VERSION 3.20)
project(GameCatEmu)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL3 REQUIRED CONFIG)

add_executable(GameCatEmu
        src/main.cpp
        src/cartridge/Cartridge.cpp
        src/bus/Bus.cpp
        src/bus/Bus.h
        src/cpu/CPU.cpp
        src/cpu/CPU.h
        src/ppu/PPU.cpp
        src/ppu/PPU.h
        src/timer/Timer.cpp
        src/timer/Timer.h
)

target_link_libraries(GameCatEmu PRIVATE SDL3::SDL3)