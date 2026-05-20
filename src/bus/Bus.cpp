//
// Created by katzenlord on 20.05.26.
//

#include "Bus.h"

Bus::Bus(Cartridge &cartridge)
    : cartridge(cartridge){
}

uint8_t Bus::read8(const uint16_t address) const {
    if (address <= 0x7FFF) {
        return cartridge.read(address);
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
        return vram[address - 0x8000];
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
        // TODO CARTRIDGE RAM
        return 0xFF;
    }

    if (address >= 0xC000 && address <= 0xDFFF) {
        return wram[address - 0xC000];
    }

    if (address >= 0xE000 && address <= 0xFDFF) {
        // Echo RAM -> mirrors WRAM 0xC000 - 0xDDFF
        return wram[address - 0xE000];
    }

    if (address >= 0xFE00 && address <= 0xFE9F) {
        return oam[address - 0xFE00];
    }

    if (address >= 0xFEA0 && address <= 0xFEFF) {
        // Unusable
        return 0xFF;
    }

    if (address >= 0xFF00 && address <= 0xFF7F) {
        return io[address - 0xFF00];
    }

    if (address >= 0xFF80 && address <= 0xFFFE) {
        return hram[address - 0xFF80];
    }

    if (address == 0xFFFF) {
        return interuptEnable;
    }

    return 0xFF;
}

