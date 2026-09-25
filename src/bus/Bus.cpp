//
// Created by katzenlord on 20.05.26.
//

#include "Bus.h"

Bus::Bus(Cartridge &cartridge)
    : cartridge(cartridge){
}

uint8_t Bus::read8(const uint16_t address) const {
    // Debug, delete me later
    if (address == 0xFF44) {
        return 0x94;
    }

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

void Bus::write8(uint16_t address, uint8_t data) {
    if (address <= 0x7FFF) {
        cartridge.write(address, data);
        return;
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
        vram[address - 0x8000] = data;
        return;
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
        // TODO CARTRIDGE RAM
        return;
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
        wram[address - 0xC000] = data;
        return;
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
        // Echo RAM
        wram[address - 0xE000] = data;
        return;
    }
    if (address >= 0xFE00 && address <= 0xFE9F) {
        oam[address - 0xFE00] = data;
        return;
    }
    if (address >= 0xFEA0 && address <= 0xFEFF) {
        // Unusable
        return;
    }
    if (address >= 0xFF00 && address <= 0xFF7F) {
        io[address - 0xFF00] = data;
        return;
    }
    if (address >= 0xFF80 && address <= 0xFFFE) {
        hram[address - 0xFF80] = data;
        return;
    }
    if (address == 0xFFFF) {
        interuptEnable = data;
        return;
    }
}

uint16_t Bus::read16(const uint16_t address) const {
    const uint8_t low = read8(address);
    const uint8_t high = read8(address + 1);

    return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

void Bus::write16(const uint16_t address, uint16_t data) {
    write8(address, static_cast<uint8_t>(data & 0x00FF));
    write8(address + 1, static_cast<uint8_t>(data >> 8) & 0x00FF);
}