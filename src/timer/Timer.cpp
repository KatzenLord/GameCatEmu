//
// Created by katzenlord on 02.10.26.
//

#include "Timer.h"

#include "../bus/Bus.h"

Timer::Timer(Bus &bus): bus(bus) {}

uint8_t Timer::readDIV() const {
    return static_cast<uint8_t>(dividerCounter >> 8);
}

void Timer::tick(int cycles) {
    if (bus.consumeDivResetRequest()) {
        dividerCounter = 0;
    }
    for (int i = 0; i < cycles; i++) {
        const bool oldSignal = timerSignal();

        dividerCounter++;
        bus.setDIV(static_cast<uint8_t>(dividerCounter >> 8));

        const bool newSignal = timerSignal();

        if (oldSignal && !newSignal) {
            incrementTIMA();
        }
    }
}

bool Timer::timerSignal() const {
    const uint8_t tac = bus.read8(0xFF07);

    if ((tac & 0x04) == 0) {
        return false;
    }

    const int bit = [&]() {
        switch (tac & 0x03) {
            case 0: return 9;
            case 1: return 3;
            case 2: return 5;
            case 3: return 7;
        }
        return 9;
    }();
    return (dividerCounter & (1u << bit)) != 0;
}

void Timer::incrementTIMA() {
    uint8_t tima = bus.read8(0xFF05);

    if (tima == 0xFF) {
        const uint8_t tma = bus.read8(0xFF06);

        bus.write8(0xFF05, tma);

        const uint8_t interruptFlags = bus.read8(0xFF0F);
        bus.write8(0xFF0F, interruptFlags | 0x04);
    } else {
        bus.write8(0xFF05, static_cast<uint8_t>(tima + 1));
    }
}

void Timer::writeDIV() {
    dividerCounter = 0;
}
