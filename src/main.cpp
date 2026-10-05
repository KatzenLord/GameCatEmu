#include <chrono>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iomanip>
#include <bits/this_thread_sleep.h>

#include "bus/Bus.h"
#include "cartridge/Cartridge.h"
#include "cpu/CPU.h"
#include "input/InputHandler.h"
#include "input/InputMapper.h"
#include "ppu/PPU.h"
#include "timer/Timer.h"
//constexpr int MAX_INSTRUCTIONS = 2'000'000;
//static int frameCycles = 0;
//static int scanlineCycles = 0;
//static uint8_t ly = 0;

using Clock = std::chrono::steady_clock;

constexpr double GB_FPS = 59.7275;
const auto FRAME_TIME =
    std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / GB_FPS)
    );

auto nextFrame = Clock::now();

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

    InputHandler inputHandler{};
    Joypad joypad;
    APU apu;
    Bus bus(cartridge, joypad, apu);
    CPU cpu(bus);
    Timer timer(bus);
    PPU ppu(bus);
    InputMapper inputMapper(inputHandler);

    apu.writeReg(0xFF17, 0xF0);
    apu.writeReg(0xFF16, 0x80);
    apu.writeReg(0xFF18, 0xD6);
    apu.writeReg(0xFF19, 0x86);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
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

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = 48000;

    SDL_AudioStream* audioStream =
        SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &spec,
            nullptr,
            nullptr
        );

    if (!audioStream) {
        std::cerr << "Audio Stream could not be created: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_ResumeAudioStreamDevice(audioStream);

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
            inputHandler.handleEvent(e);

            if (e.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }

            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                isRunning = false;
            }
        }

        const InputState state = inputMapper.getState();

        if (joypad.setInputState(state)) {
            bus.requestInterrupt(Interrupt::Joypad);
        }

        while (!ppu.frameReady() && isRunning) {
            const int cycles = cpu.step();

            timer.tick(cycles);
            ppu.step(cycles);
            apu.tick(cycles);

            const auto& buffer = apu.getAudioBuffer();

            if (buffer.size() >= 512) {
                const auto [minIt, maxIt] =
                    std::minmax_element(buffer.begin(), buffer.end());

                std::cout
                    << "samples: " << buffer.size()
                    << " min: " << *minIt
                    << " max: " << *maxIt
                    << '\n';

                SDL_PutAudioStreamData(
                    audioStream,
                    buffer.data(),
                    buffer.size() * sizeof(float)
                );

                apu.clearAudioBuffer();
            }
        }

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

            nextFrame += FRAME_TIME;

            const auto now = Clock::now();

            if (now < nextFrame) {
                std::this_thread::sleep_until(nextFrame);
            } else {
                nextFrame = now;
            }
        }
    }

    SDL_DestroyAudioStream(audioStream);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
