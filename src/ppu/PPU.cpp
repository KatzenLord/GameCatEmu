//
// Created by katzenlord on 26.09.26.
//

#include "PPU.h"

#include <iostream>

#include "../bus/Bus.h"

PPU::PPU(Bus& bus) : bus(bus) {}

void PPU::step(int cycles) {
    scanlineCycles += cycles;

    while (scanlineCycles >= 456) {
        scanlineCycles -= 456;

        uint8_t ly = bus.getLY();
        ly++;

        if (ly == 144) {
            renderFrame();
            frameReadyFlag = true;
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

uint8_t PPU::getBackgroundPixel(int x, int y) {
    const uint8_t lcdc = bus.read8(0xFF40);
    const uint8_t scy = bus.read8(0xFF42);
    const uint8_t scx = bus.read8(0xFF43);

    const uint8_t bgX = static_cast<uint8_t>(x + scx);
    const uint8_t bgY = static_cast<uint8_t>(y + scy);

    const uint16_t tileMapBase =
        (lcdc & 0x08) ? 0x9C00 : 0x9800;

    const uint8_t tileX = bgX / 8;
    const uint8_t tileY = bgY / 8;

    const uint16_t tileMapAddress =
        tileMapBase + tileY * 32 + tileX;

    const uint8_t tileNumber =
        bus.read8(tileMapAddress);

    uint16_t tileAddress;

    if (lcdc & 0x10) {
        tileAddress =
            0x8000 + static_cast<uint16_t> (tileNumber) * 16;
    } else {
        const int8_t signedTile =
            static_cast<int8_t>(tileNumber);
        tileAddress =
            static_cast<uint16_t> (0x9000 + static_cast<int16_t>(tileNumber) * 16);
    }

    const uint8_t pixelX = bgX % 8;
    const uint8_t pixelY = bgY % 8;

    const uint16_t rowAddress =
        tileAddress + pixelY * 2;

    const uint8_t low = bus.read8(rowAddress);
    const uint8_t high = bus.read8(rowAddress + 1);

    const uint8_t bit = 7 - pixelX;

    const uint8_t lowBit = (low >> bit) & 0x01;
    const uint8_t highBit = (high >> bit) & 0x01;

    return static_cast<uint8_t>(lowBit | (highBit << 1));
}

void PPU::debugTileMap() {
    const uint8_t lcdc = bus.read8(0xFF40);

    const uint16_t tileMapBase =
        (lcdc & 0x08) ? 0x9C00 : 0x9800;

    std::cout << "LCDC = 0x"
              << std::hex << static_cast<int>(lcdc)
              << "\nTileMapBase = 0x"
              << tileMapBase
              << std::dec << '\n';

    int nonZeroCount = 0;

    for (int i = 0; i < 32 * 32; i++) {
        const uint8_t tile = bus.read8(tileMapBase + i);

        if (tile != 0) {
            std::cout
                << "Map[" << i << "] = "
                << static_cast<int>(tile)
                << '\n';

            nonZeroCount++;

            if (nonZeroCount >= 20)
                break;
        }
    }

    std::cout << "Non-zero entries found: "
              << nonZeroCount << '\n';
}

uint8_t PPU::applyBGPalette(uint8_t colorId) const {
    const uint8_t bgp = bus.read8(0xFF47);
    return static_cast<uint8_t>((bgp >> (colorId * 2)) & 0x03);
}

uint32_t PPU::dmgShadeToARGB(uint8_t shade) const {
    switch (shade) {
        case 0: return 0xFFFFFFFF;
        case 1: return 0xFFAAAAAA;
        case 2: return 0xFF555555;
        case 3: return 0xFF000000;
        default: return 0xFFFF00FF;
    }
}

void PPU::renderFrame() {
    for (int y = 0; y < ScreenHeight; y++) {
        for (int x = 0; x < ScreenWidth; x++) {
            const uint8_t colorId = getBackgroundPixel(x, y);
            const uint8_t shade = applyBGPalette(colorId);

            framebuffer[y * ScreenWidth + x] =
                dmgShadeToARGB(shade);
        }
    }
}

bool PPU::frameReady() const {
    return frameReadyFlag;
}

void PPU::clearFrameReady() {
    frameReadyFlag = false;
}

std::array<uint32_t, ScreenWidth * ScreenHeight> PPU::getFramebuffer() const {
    return framebuffer;
}

void PPU::debugTile(uint8_t tileNumber) {
    const uint16_t tileAddress =
        0x8000 + static_cast<uint16_t>(tileNumber) * 16;

    std::cout << "Tile " << static_cast<int>(tileNumber)
              << " at 0x" << std::hex << tileAddress << std::dec << '\n';

    for (int row = 0; row < 8; row++) {
        const uint8_t low  = bus.read8(tileAddress + row * 2);
        const uint8_t high = bus.read8(tileAddress + row * 2 + 1);

        std::cout << "Row " << row
                  << ": low=0x" << std::hex << static_cast<int>(low)
                  << " high=0x" << static_cast<int>(high)
                  << std::dec << "  ";

        for (int x = 0; x < 8; x++) {
            const int bit = 7 - x;

            const uint8_t lowBit  = (low >> bit) & 1;
            const uint8_t highBit = (high >> bit) & 1;

            const uint8_t color =
                lowBit | (highBit << 1);

            std::cout << static_cast<int>(color);
        }

        std::cout << '\n';
    }
}

void PPU::debugUsedTiles() {
    const uint8_t lcdc = bus.read8(0xFF40);

    const uint16_t tileMapBase =
        (lcdc & 0x08) ? 0x9C00 : 0x9800;

    bool seen[256] = {};

    for (int i = 0; i < 32 * 32; i++) {
        const uint8_t tile = bus.read8(tileMapBase + i);

        if (!seen[tile]) {
            seen[tile] = true;

            const int x = i % 32;
            const int y = i / 32;

            std::cout
                << "Tile ID "
                << static_cast<int>(tile)
                << " at map ("
                << x << ", "
                << y << ")\n";
        }
    }
}

void PPU::requestVBlankInterrupt() {
    const uint8_t currentIF = bus.read8(0xFF0F);
    bus.write8(0xFF0F, currentIF | 0x01);
}

