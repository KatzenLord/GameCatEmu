//
// Created by katzenlord on 20.05.26.
//

#pragma once

#ifndef GAMECATEMU_CPU_H
#define GAMECATEMU_CPU_H
#include "../bus/Bus.h"

class CPU {
public:
    explicit CPU(Bus& bus);

    void reset();
    int step();
private:
    Bus& bus;

    uint8_t A = 0;
    uint8_t F = 0;
    uint8_t B = 0;
    uint8_t C = 0;
    uint8_t D = 0;
    uint8_t E = 0;
    uint8_t H = 0;
    uint8_t L = 0;

    uint16_t PC = 0;
    uint16_t SP = 0;

    bool halted = false;
    bool stopped = false;


    uint8_t fetch8();
    uint16_t fetch16();

    uint8_t read8(uint16_t address) const;
    void write8(uint16_t address, uint8_t value);

    uint16_t getAF() const;
    uint16_t getBC() const;
    uint16_t getDE() const;
    uint16_t getHL() const;

    void setAF(uint16_t value);
    void setBC(uint16_t value);
    void setDE(uint16_t value);
    void setHL(uint16_t value);

    bool getZ() const;
    bool getN() const;
    bool getH() const;
    bool getC() const;

    void setZ(bool value);
    void setN(bool value);
    void setH(bool value);
    void setC(bool value);

    int stepCB();

    int nop();
    int ld_r_u8(uint8_t& reg);
    int jp_u16();
    int xor_a();
};

#endif //GAMECATEMU_CPU_H
