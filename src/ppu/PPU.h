//
// Created by katzenlord on 26.09.26.
//

#ifndef GAMECATEMU_PPU_H
#define GAMECATEMU_PPU_H
#pragma once
#include <cstdint>

class Bus;

class PPU {
public:
    explicit PPU(Bus& bus);

    void step(int cycles);

private:
    Bus& bus;

    int scanlineCycles = 0;
    void requestVBlankInterrupt();

};


#endif //GAMECATEMU_PPU_H
