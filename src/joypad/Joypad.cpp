//
// Created by katzenlord on 04.10.26.
//

#include "Joypad.h"

bool Joypad::setInputState(const InputState &state) {
    const uint8_t oldValue = read();

    inputState = state;

    const uint8_t newValue = read();

    const uint8_t fallingEdges =
        (oldValue & ~newValue) & 0x0F;

    return fallingEdges != 0;
}

void Joypad::write(uint8_t value) {
    select = value & 0x30;
}

uint8_t Joypad::read() const {
    uint8_t result = 0xC0 | select | 0x0F;

    if ((select & 0x10) == 0) {
        if (inputState.right) result &= ~0x01;
        if (inputState.left) result &= ~0x02;
        if (inputState.up) result &= ~0x04;
        if (inputState.down) result &= ~0x08;
    }

    if ((select & 0x20) == 0) {
        if (inputState.a) result &= ~0x01;
        if (inputState.b) result &= ~0x02;
        if (inputState.select) result &= ~0x04;
        if (inputState.start) result &= ~0x08;
    }

    return result;
}
