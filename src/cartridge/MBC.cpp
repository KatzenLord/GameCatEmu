//
// Created by katzenlord on 20.11.26.
//

#include "MBC.h"

#include <iostream>

#include "Cartridge.h"
#include "MapperType.h"

void MBC::configure(MapperType mapperType, size_t romSizeBytes, size_t ramSizeBytes, bool multicart) {
    type = mapperType;
    isMulticart = multicart;

    romBankCount = romSizeBytes / 0x4000;
    ramBankCount = ramSizeBytes / 0x2000;

    ramEnabled = false;

    romBankLow = 1;
    bankHigh = 0;
    bankingMode = 0;
}

void MBC::write(uint16_t addr, uint8_t value) {
    if (type == MapperType::MBC0) {
        return;
    }

    if (type == MapperType::MBC1) {
        if (addr <= 0x1FFF) {
            ramEnabled = (value & 0x0F) == 0x0A;
        }
        else if (addr <= 0x3FFF) {
            romBankLow = value & 0x1F;

            if (romBankLow == 0) {
                romBankLow = 1;
            }
        }
        else if (addr <= 0x5FFF) {
            bankHigh = value & 0x03;
        }
        else if (addr <= 0x7FFF) {
            bankingMode = value & 0x01;
        }
    }
}

size_t MBC::mapRomAddress(uint16_t addr) const {
    if (type == MapperType::MBC0) {
        return addr;
    }
    size_t bank;

    if (isMulticart) {
        if (addr < 0x4000) {
            bank = bankingMode == 1
                    ? static_cast<size_t>(bankHigh) << 4
                    : 0;
        }
        else {
            bank = (static_cast<size_t>(bankHigh) << 4) | (romBankLow & 0x0F);
        }
    } else {
        if (addr < 0x4000) {
            bank = bankingMode == 1
                    ? static_cast<size_t>(bankHigh) << 5
                    : 0;
        }
        else {
            bank = (static_cast<size_t>(bankHigh) << 5) | romBankLow;
        }
    }
    bank &= romBankCount - 1;

    return bank * 0x4000 + (addr & 0x3FFF);
}

size_t MBC::mapRamAddress(uint16_t addr) const {
    const size_t offset = addr - 0xA000;

    if (type == MapperType::MBC0) {
        return offset;
    }

    size_t bank = bankingMode == 1 ? bankHigh : 0;

    if (ramBankCount > 0) {
        bank %= ramBankCount;
    }

    return bank * 0x2000 + offset;
}

bool MBC::isRamEnabled() const {
    if (type == MapperType::MBC0) {
        return true;
    }
    return ramEnabled;
}