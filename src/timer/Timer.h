//
// Created by katzenlord on 02.10.26.
//

#ifndef GAMECATEMU_TIMER_H
#define GAMECATEMU_TIMER_H

#pragma once
#include <cstdint>

class Bus;

class Timer {
public:
    explicit Timer(Bus& bus);

    bool tick(int cycles);
    uint8_t readDIV() const;
private:
    Bus& bus;

    uint16_t dividerCounter = 0;

    void incrementTIMA();
    void writeTAC(uint8_t value);
    bool timerSignal() const;

    bool timaReloadPending = false;
    uint8_t timaReloadDelay = 0;

    bool timaReloading = false;
    void writeTIMA(uint8_t value);
    void writeTMA(uint8_t value) const;
};


#endif //GAMECATEMU_TIMER_H
