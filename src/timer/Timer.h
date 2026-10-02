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

    void tick(int cycles);
    uint8_t readDIV() const;
    void writeDIV();
private:
    Bus& bus;

    uint16_t dividerCounter = 0;

    void incrementTIMA();
    bool timerSignal() const;
};


#endif //GAMECATEMU_TIMER_H
