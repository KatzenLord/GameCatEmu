cmake_minimum_required(VERSION 3.20)
project(GameCatEmu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

include(FetchContent)

FetchContent_Declare(
	SDL3
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG release-3.2.18
	GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3)

# find_package(SDL3 REQUIRED CONFIG)

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
        src/bus/Interrupt.h
        src/apu/APU.cpp
        src/apu/APU.h
)

target_link_libraries(GameCatEmu PRIVATE SDL3::SDL3)
