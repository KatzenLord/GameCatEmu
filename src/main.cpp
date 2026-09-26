#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iomanip>

#include "bus/Bus.h"
#include "cartridge/Cartridge.h"
#include "cpu/CPU.h"
#include "ppu/PPU.h"
//constexpr int MAX_INSTRUCTIONS = 2'000'000;
//static int frameCycles = 0;
//static int scanlineCycles = 0;
//static uint8_t ly = 0;
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
    PPU ppu(bus);

    //for (int i = 0; i < MAX_INSTRUCTIONS && !cpu.isHalted(); ++i) {
    //    int cycles = cpu.step();
//
    //    scanlineCycles += cycles;
    //    frameCycles += cycles;
//
    //    if (scanlineCycles >= 456) {
    //        scanlineCycles -= 456;
//
    //        ly++;
//
    //        if (ly > 153) {
    //            ly = 0;
    //        }
//
    //        bus.setLY(ly);
//
    //        if (ly == 144) {
    //            bus.write8(0xFF0F, bus.read8(0xFF0F) | 0x01);
    //        }
    //    }
//
    //    //std::cout << "Step " << i
    //    //    << " cycles=" << cycles << std::endl;
    //}

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

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        160,
        144
    );
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

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
        int cycles = cpu.step();
        ppu.step(cycles);
        if (ppu.frameReady()) {
            SDL_UpdateTexture(
                texture,
                nullptr,
                ppu.getFramebuffer().data(),
                160 * sizeof(uint32_t)
            );
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            ppu.clearFrameReady();
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
