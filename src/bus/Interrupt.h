//
// Created by katzenlord on 04.10.26.
//

#ifndef GAMECATEMU_INTERRUPT_H
#define GAMECATEMU_INTERRUPT_H
#include <cstdint>

enum class Interrupt : uint8_t {
    VBlank  = 0,
    LCDStat = 1,
    Timer   = 2,
    Serial  = 3,
    Joypad  = 4
};

#endif //GAMECATEMU_INTERRUPT_H

