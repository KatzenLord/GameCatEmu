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
        src/input/InputHandler.cpp
        src/input/InputHandler.h
        src/input/InputMapper.cpp
        src/input/InputMapper.h
        src/input/InputState.h
        src/joypad/Joypad.cpp
        src/joypad/Joypad.h
)

target_link_libraries(GameCatEmu PRIVATE SDL3::SDL3)