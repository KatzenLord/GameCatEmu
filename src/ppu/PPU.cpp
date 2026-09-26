//
// Created by katzenlord on 26.09.26.
//

#include "PPU.h"

#include "../bus/Bus.h"

PPU::PPU(Bus& bus) : bus(bus) {}

void PPU::step(int cycles) {
    scanlineCycles += cycles;

    while (scanlineCycles >= 456) {
        scanlineCycles -= 456;

        uint8_t ly = bus.getLY();
        ly++;

        if (ly == 144) {
            requestVBlankInterrupt();
        }

        if (ly > 153) {
            ly = 0;
        }

        bus.setLY(ly);
    }

    const uint8_t ly = bus.getLY();

    if (ly >= 144) {
        bus.setPPUMode(1);
    } else if (scanlineCycles < 80) {
        bus.setPPUMode(2);
    } else if (scanlineCycles < 252) {
        bus.setPPUMode(3);
    } else {
        bus.setPPUMode(0);
    }
}

void PPU::requestVBlankInterrupt() {
    const uint8_t currentIF = bus.read8(0xFF0F);
    bus.write8(0xFF0F, currentIF | 0x01);
}
