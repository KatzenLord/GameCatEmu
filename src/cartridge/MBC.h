//
// Created by katzenlord on 20.11.26.
//

#ifndef GAMECATEMU_MBC_H
#define GAMECATEMU_MBC_H
#pragma once

#include <cstddef>
#include <cstdint>

enum class MapperType : uint8_t;

class MBC {
public:
    void configure(MapperType mapperType, size_t romSizeBytes, size_t ramSizeBytes, bool multicart);
    void write(uint16_t addr, uint8_t value);

    size_t mapRomAddress(uint16_t addr) const;
    size_t mapRamAddress(uint16_t addr) const;

    bool isRamEnabled() const;
private:
    MapperType type;

    bool ramEnabled = false;;

    uint8_t romBankLow = 1;
    uint8_t bankHigh = 0;
    uint8_t bankingMode = 0;

    size_t romBankCount = 2;
    size_t ramBankCount = 0;

    bool isMulticart = false;
};


#endif //GAMECATEMU_MBC_H
