//
// Created by katzenlord on 04.10.26.
//

#ifndef GAMECATEMU_INPUTHANDLER_H
#define GAMECATEMU_INPUTHANDLER_H

#pragma once

#include "SDL3/SDL.h"
#include "unordered_set"

class InputHandler {
public:
    void handleEvent(const SDL_Event& event);
    bool keyDown(SDL_Keycode key) const;
private:
    std::unordered_set<SDL_Keycode> pressedKeys;
};


#endif //GAMECATEMU_INPUTHANDLER_H
