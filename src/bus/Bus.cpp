//
// Created by katzenlord on 20.05.26.
//

#include "Bus.h"

#include <iostream>


Bus::Bus(Cartridge &cartridge, Joypad &joypad)
    : cartridge(cartridge), joypad(joypad){
}

uint8_t Bus::read8(const uint16_t address) const {
    if (address == 0xFF00) {
        return joypad.read();
    }

    if (address == 0xFF04) {
        return div;
    }

    if (address == 0xFF44) {
        return ly;
    }
    if (address == 0xFF41) {
        // STAT
        return static_cast<uint8_t>((io[0x41] & 0xFC) | (ppuMode & 0x03));
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
        return interruptEnable;
    }
    return 0xFF;
}

void Bus::write8(uint16_t address, uint8_t data) {
    if (address == 0xFF00) {
        joypad.write(data);
        return;
    }

    if (address == 0xFF46) {
        const uint16_t source =
            static_cast<uint16_t>(data) << 8;

        for (uint16_t i = 0; i < 0xA0; i++) {
            oam[i] = read8(source + i);
        }

        io[0x46] = data;
        return;
    }

    if (address == 0xFF04) {
        divResetRequested = true;
        return;
    }

    if (address == 0xFF44) {
        ly = 0;
        return;
    }
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
        interruptEnable = data;
        return;
    }
}

void Bus::setLY(uint8_t value) {
    ly = value;
}

uint8_t Bus::getLY() const {
    return ly;
}

void Bus::setPPUMode(uint8_t value) {
    ppuMode = value & 0x03;
}

uint8_t Bus::getPPUMode() const {
    return ppuMode;
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

void Bus::setDIV(uint8_t value) {
    div = value;
}

bool Bus::consumeDivResetRequest() {
    if (!divResetRequested) {
        return false;
    }
    divResetRequested = false;
    return true;
}

void Bus::requestInterrupt(Interrupt interrupt) {
    uint8_t interruptFlags = read8(0xFF0F);

    interruptFlags |= (1 << static_cast<uint8_t>(interrupt));

    write8(0xFF0F, interruptFlags);
}
