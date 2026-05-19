#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL could not be Initialized: " << SDL_GetError() << std::endl;
        return 1;
    }

    constexpr int GB_WIDTH = 160;
    constexpr int GB_HEIGHT = 144;
    constexpr int SCALE = 4;
    constexpr float GAMEBOY_ASPECT = 160.0f / 144.0f;


    SDL_Window* window{SDL_CreateWindow(
        "GameCat Emu",
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
