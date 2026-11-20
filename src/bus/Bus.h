//
// Created by katzenlord on 20.05.26.
//

#pragma once

#ifndef GAMECATEMU_BUS_H
#define GAMECATEMU_BUS_H

#include <array>
#include <cstdint>

#include "Interrupt.h"
#include "../cartridge/Cartridge.h"
#include "../joypad/Joypad.h"
#include "../apu/APU.h"

class Bus {
public:
    explicit Bus(Cartridge& cartridge, Joypad& joypad, APU& apu);

    uint8_t read8(uint16_t address) const;
    void write8(uint16_t address, uint8_t value);

    uint16_t read16(uint16_t address) const;
    void write16(uint16_t address, uint16_t value);

    uint8_t getLY() const;
    void setLY(uint8_t value);

    uint8_t getPPUMode() const;
    void setPPUMode(uint8_t value);

    void setDIV(uint8_t value);
    bool consumeDivResetRequest();

    void setTAC(uint8_t value);
    bool consumeTacWriteRequest(uint8_t& value);

    void setTIMA(uint8_t value);
    bool consumeTimaWriteRequest(uint8_t& value);

    void setTMA(uint8_t value);
    bool consumeTmaWriteRequest(uint8_t& value);

    void requestInterrupt(Interrupt interrupt);
private:
    Cartridge& cartridge;
    Joypad& joypad;
    APU& apu;

    std::array<uint8_t, 0x2000> vram{}; // 0x0000 - 0x9FFF
    std::array<uint8_t, 0x2000> wram{}; // 0xC000 - 0xDFFF
    std::array<uint8_t, 0x00A0> oam{};  // 0xFE00 - 0xFE9F
    std::array<uint8_t, 0x0080> io{};   // 0xFF00 - 0xFF7F
    std::array<uint8_t, 0x007F> hram{}; // 0xFF80 - 0xFFFE

    uint8_t interruptEnable = 0;         // 0xFFFF

    uint8_t ly = 0;
    uint8_t ppuMode = 2;

    uint8_t div = 0;
    bool divResetRequested = false;

    bool tacWritePending = false;
    uint8_t pendingTacValue = 0;
    uint8_t tac = 0;

    bool timaWritePending = false;
    uint8_t pendingTimaValue = 0;
    uint8_t tima = 0;

    bool tmaWritePending = false;
    uint8_t pendingTmaValue = 0;
    uint8_t tma = 0;
};

#endif //GAMECATEMU_BUS_H
