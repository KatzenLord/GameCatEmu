//
// Created by katzenlord on 04.10.26.
//

#include "InputMapper.h"

InputMapper::InputMapper(const InputHandler &inputHandler) : inputHandler(inputHandler) {
}

InputState InputMapper::getState() const {
    InputState state;

    state.right = inputHandler.keyDown(SDLK_RIGHT);
    state.left = inputHandler.keyDown(SDLK_LEFT);
    state.up = inputHandler.keyDown(SDLK_UP);
    state.down = inputHandler.keyDown(SDLK_DOWN);

    state.a = inputHandler.keyDown(SDLK_Y);
    state.b = inputHandler.keyDown(SDLK_X);
    state.start = inputHandler.keyDown(SDLK_RETURN);
    state.select = inputHandler.keyDown(SDLK_BACKSPACE);

    return state;
}
