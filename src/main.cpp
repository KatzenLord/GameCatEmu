#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iomanip>

#include "bus/Bus.h"
#include "cartridge/Cartridge.h"
#include "cpu/CPU.h"
constexpr int MAX_INSTRUCTIONS = 10'000'000;
int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: GameCatEmu <rom.gb>" << std::endl;
        return 1;
    }

    Cartridge cartridge;

    if (!cartridge.loadFromFile(argv[1])) {
        return 1;
    }

    const CartridgeHeader& header = cartridge.getHeader();

    std::cout << "ROM Loaded" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Title: " << header.title << std::endl;
    std::cout << "Cartridge Type: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(header.cartridgeType) << std::dec << " -> " << header.cartridgeTypeString << std::endl;
    std::cout << "ROM Size: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(header.romSize) << std::dec << " -> " << header.romSizeString << std::endl;
    std::cout << "RAM Size: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(header.ramSize) << std::dec << " -> " << header.ramSizeString << std::endl;

    Bus bus(cartridge);
    CPU cpu(bus);

    for (int i = 0; i < MAX_INSTRUCTIONS && !cpu.isHalted(); ++i) {
        int cycles = cpu.step();

        //std::cout << "Step " << i
        //    << " cycles=" << cycles << std::endl;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL could not be Initialized: " << SDL_GetError() << std::endl;
        return 1;
    }

    constexpr int GB_WIDTH = 160;
    constexpr int GB_HEIGHT = 144;
    constexpr int SCALE = 4;
    constexpr float GAMEBOY_ASPECT = 160.0f / 144.0f;


    SDL_Window* window{SDL_CreateWindow(
        ("GameCat Emu - "+header.title).c_str(),
        GB_WIDTH*SCALE,
        GB_HEIGHT*SCALE,
        SDL_WINDOW_RESIZABLE
    )};

    SDL_SetWindowAspectRatio(window, GAMEBOY_ASPECT, GAMEBOY_ASPECT);

    if (!window) {
        std::cout << "Window could not be Created: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cout << "Renderer could not be Created: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderLogicalPresentation(
        renderer,
        GB_WIDTH,
        GB_HEIGHT,
        SDL_LOGICAL_PRESENTATION_LETTERBOX
    );

    bool isRunning = true;

    SDL_Event e;
    while (isRunning) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }

            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                isRunning = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
