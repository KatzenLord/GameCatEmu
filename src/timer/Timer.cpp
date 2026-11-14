//
// Created by katzenlord on 02.10.26.
//

#include "Timer.h"

#include "../bus/Bus.h"

Timer::Timer(Bus &bus): bus(bus) {}

uint8_t Timer::readDIV() const {
    return static_cast<uint8_t>(dividerCounter >> 8);
}

bool Timer::tick(int cycles) {
    bool apuClock = false;

    if (bus.consumeDivResetRequest()) {
        const bool oldTimerSignal = timerSignal();
        const bool oldApuSignal = (dividerCounter & (1u << 12)) != 0;

        dividerCounter = 0;
        bus.setDIV(0);

        const bool newTimerSignal = timerSignal();

        if (oldTimerSignal && !newTimerSignal) {
            incrementTIMA();
        }
        if (oldApuSignal) {
            apuClock = true;
        }
    }
    for (int i = 0; i < cycles; i++) {
        const bool oldTimerSignal = timerSignal();
        const bool oldApuSignal = (dividerCounter & (1u << 12)) != 0;

        dividerCounter++;
        bus.setDIV(static_cast<uint8_t>(dividerCounter >> 8));

        const bool newTimerSignal = timerSignal();
        const bool newApuSignal = (dividerCounter & (1u << 12)) != 0;

        if (oldTimerSignal && !newTimerSignal) {
            incrementTIMA();
        }
        if (oldApuSignal && !newApuSignal) {
            apuClock = true;
        }
    }

    return apuClock;
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

        bus.requestInterrupt(Interrupt::Timer);
    } else {
        bus.write8(0xFF05, static_cast<uint8_t>(tima + 1));
    }
}