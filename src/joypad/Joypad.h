//
// Created by katzenlord on 04.10.26.
//

#ifndef GAMECATEMU_JOYPAD_H
#define GAMECATEMU_JOYPAD_H

#pragma once
#include <cstdint>
#include "../input/InputState.h"

class Joypad {
public:
    bool setInputState(const InputState& state);

    uint8_t read() const;
    void write(uint8_t value);
private:
    InputState inputState;

    uint8_t select = 0x30;
};


#endif //GAMECATEMU_JOYPAD_H
