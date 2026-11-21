#include <chrono>
#include <filesystem>
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

    const std::filesystem::path romPath = argv[1];
    bool testMode = false;

    for (int i = 2; i < argc; i++) {
        if (std::string_view(argv[i]) == "--test") {
            testMode = true;
        }
    }
    Cartridge cartridge;

    if (!cartridge.loadFromFile(romPath)) {
        return 1;
    }

    const CartridgeHeader& header = cartridge.getHeader();

    const auto& features = cartridge.getFeatures();

    std::cout << "==================\n";

    std::cout << "Mapper: "
                << (features.mapper == MapperType::MBC0 ? "MBC0" : "MBC1")
                << std::endl;

    std::cout << "RAM: " << features.hasRam << '\n';
    std::cout << "Battery: " << features.hasBattery << '\n';
    std::cout << "ROM bytes: " << features.romSizeBytes << '\n';
    std::cout << "RAM bytes: " << features.ramSizeBytes << '\n';

    InputHandler inputHandler{};
    Joypad joypad;
    APU apu{};
    Bus bus(cartridge, joypad, apu);
    CPU cpu(bus);
    Timer timer(bus);
    PPU ppu(bus);
    InputMapper inputMapper(inputHandler);

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
    spec.channels = 2;
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

            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {

                if (testMode) {
                    namespace fs = std::filesystem;

                    fs::create_directory("logs");

                    const fs::path outputPath = fs::path("logs") / (romPath.stem().string() + ".bmp");

                    SDL_Surface* surface = SDL_CreateSurfaceFrom(
                        160,
                        144,
                        SDL_PIXELFORMAT_RGBA8888,
                        const_cast<uint32_t*>(ppu.getFramebuffer().data()),
                        160 * sizeof(uint32_t)
                        );

                    if (!surface) {
                        std::cerr << "Screenshot surface failed: "
                                  << SDL_GetError() << '\n';
                        break;
                    }

                    if (!SDL_SaveBMP(surface, outputPath.string().c_str())) {
                        std::cerr << "Screenshot failed: "
                                  << SDL_GetError() << '\n';
                    } else {
                        std::cout << "Screenshot saved: "
                                  << outputPath << '\n';
                    }

                    SDL_DestroySurface(surface);
                }

                isRunning = false;
            }
        }

        const InputState state = inputMapper.getState();

        if (joypad.setInputState(state)) {
            bus.requestInterrupt(Interrupt::Joypad);
        }

        while (!ppu.frameReady() && isRunning) {
            const int cycles = cpu.step();

            if (timer.tick(cycles)) {
                apu.clockFramesequencer();
            }
            ppu.step(cycles);
            apu.tick(cycles);

            const auto& buffer = apu.getAudioBuffer();

            if (buffer.size() >= 512) {

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
