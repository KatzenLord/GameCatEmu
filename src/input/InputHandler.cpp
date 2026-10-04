//
// Created by katzenlord on 04.10.26.
//

#include "InputHandler.h"

#include <iostream>

void InputHandler::handleEvent(const SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            pressedKeys.insert(event.key.key);
            std::cout << std::to_string(event.key.key) << " was pressed" << std::endl;
            break;
        case SDL_EVENT_KEY_UP:
            pressedKeys.erase(event.key.key);
            std::cout << std::to_string(event.key.key) << " was let go" << std::endl;
            break;
        default:
            break;
    }
}

bool InputHandler::keyDown(SDL_Keycode key) const {
    return pressedKeys.contains(key);
}
