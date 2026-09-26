//
// Created by katzenlord on 20.05.26.
//

#pragma once

#ifndef GAMECATEMU_BUS_H
#define GAMECATEMU_BUS_H

#include <array>
#include <cstdint>

#include "../cartridge/Cartridge.h"

class Bus {
public:
    explicit Bus(Cartridge& cartridge);

    uint8_t read8(uint16_t address) const;
    void write8(uint16_t address, uint8_t value);

    uint16_t read16(uint16_t address) const;
    void write16(uint16_t address, uint16_t value);
    void setLY(uint8_t value);

private:
    Cartridge& cartridge;

    std::array<uint8_t, 0x2000> vram{}; // 0x0000 - 0x9FFF
    std::array<uint8_t, 0x2000> wram{}; // 0xC000 - 0xDFFF
    std::array<uint8_t, 0x00A0> oam{};  // 0xFE00 - 0xFE9F
    std::array<uint8_t, 0x0080> io{};   // 0xFF00 - 0xFF7F
    std::array<uint8_t, 0x007F> hram{}; // 0xFF80 - 0xFFFE

    uint8_t interuptEnable = 0;         // 0xFFFF

    mutable uint8_t fakeLY = 0;


};

#endif //GAMECATEMU_BUS_H
