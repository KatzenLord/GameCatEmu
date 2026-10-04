//
// Created by katzenlord on 26.09.26.
//

#include "PPU.h"

#include <iostream>
#include <algorithm>
#include <cassert>

#include "../bus/Bus.h"

PPU::PPU(Bus& bus) : bus(bus) {}

struct Sprite {
    int index;
    int x;
    int y;
    uint8_t tile;
    uint8_t attributes;
};

void PPU::step(int cycles) {
    scanlineCycles += cycles;

    while (scanlineCycles >= 456) {
        scanlineCycles -= 456;

        uint8_t ly = bus.getLY();

        if (ly < 144) {
            renderScanline(ly);
        }
        ly++;

        if (ly > 153) {
            ly = 0;
            windowLineCounter = 0;
        }

        bus.setLY(ly);

        updateLYC();

        const uint8_t stat = bus.read8(0xFF41);

        if ((stat & 0x40) && (stat & 0x04)) {
            requestSTATInterrupt();
        }

        if (ly == 144) {
            frameReadyFlag = true;
            requestVBlankInterrupt();
        }
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

        tileAddress = static_cast<uint16_t>(
            0x9000 + static_cast<int16_t>(signedTile) * 16
        );
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

void PPU::renderScanline(int y) {
    const uint8_t lcdc = bus.read8(0xFF40);
    const uint8_t wy = bus.read8(0xFF4A);
    const uint8_t wx = bus.read8(0xFF4B);

    const bool windowEnable =
        (lcdc & 0x20) != 0 &&
        (lcdc & 0x01) != 0;

    const bool windowVisible =
        windowEnable &&
        y >= wy &&
        wx <= 166;

    const int windowStartX =
        static_cast<int>(wx) - 7;

    bool windowDrawnThisLine = false;

    for (int x = 0; x < ScreenWidth; x++) {
        uint8_t colorId = 0;

        if (lcdc & 0x01) {
            colorId = getBackgroundPixel(x, y);
        }

        if (windowVisible && x >= windowStartX) {
            colorId = getWindowPixel(x, windowLineCounter);
            windowDrawnThisLine = true;
        }

        const int index = y * ScreenWidth + x;
        bgColorIds[index] = colorId;

        const uint8_t shade = applyBGPalette(colorId);

        framebuffer[index] = dmgShadeToARGB(shade);
    }

    renderSpritesForScanline(y);

    if (windowDrawnThisLine) {
        windowLineCounter++;
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

void PPU::requestVBlankInterrupt() const {
    bus.requestInterrupt(Interrupt::VBlank);
}

void PPU::requestSTATInterrupt() const {
    bus.requestInterrupt(Interrupt::LCDStat);
}

uint8_t PPU::applyOBJPalette(uint8_t colorId, bool useOBP1) const {
    const uint8_t palette = bus.read8(useOBP1 ? 0xFF49 : 0xFF48);
    return static_cast<uint8_t>((palette >> (colorId * 2)) & 0x03);
}

void PPU::renderSpritesForScanline(int screenY) {
    const uint8_t lcdc = bus.read8(0xFF40);

    if ((lcdc & 0x02) == 0)
        return;

    const int spriteHeight =
        (lcdc & 0x04) ? 16 : 8;

    std::array<Sprite, 10> sprites{};
    int spriteCount = 0;

    for (int i = 0; i < 40 && spriteCount < 10; i++) {
        const uint16_t addr =
            static_cast<uint16_t>(0xFE00 + i * 4);

        const int spriteY =
            static_cast<int>(bus.read8(addr)) - 16;

        const int spriteX =
            static_cast<int>(bus.read8(addr + 1)) - 8;

        if (screenY >= spriteY &&
            screenY < spriteY + spriteHeight) {

            sprites[spriteCount++] = {
                .index = i,
                .x = spriteX,
                .y = spriteY,
                .tile = bus.read8(addr + 2),
                .attributes = bus.read8(addr + 3)
            };
        }
    }

    std::sort(
        sprites.begin(),
        sprites.begin() + spriteCount,
        [](const Sprite& a, const Sprite& b) {
            if (a.x != b.x)
                return a.x < b.x;

            return a.index < b.index;
        }
    );

    for (int screenX = 0; screenX < ScreenWidth; screenX++) {
        for (int s = 0; s < spriteCount; s++) {
            const Sprite& sprite = sprites[s];

            if (screenX < sprite.x
                || screenX >= sprite.x + 8)
                continue;

            const bool priority = sprite.attributes & 0x80;
            const bool yFlip = sprite.attributes & 0x40;
            const bool xFlip = sprite.attributes & 0x20;
            const bool useOBP1 = sprite.attributes & 0x10;

            int tileY = screenY - sprite.y;

            if (yFlip) {
                tileY = spriteHeight - 1 - tileY;
            }

            uint8_t tileNumber = sprite.tile;

            if (spriteHeight == 16) {
                tileNumber &= 0xFE;

                if (tileY >= 8) {
                    tileNumber++;
                    tileY -= 8;
                }
            }

            const int localX = screenX - sprite.x;

            const int tileX = xFlip ? 7 - localX : localX;

            const uint16_t tileAddress =
                static_cast<uint16_t>(
                      0x8000 +
                      tileNumber * 16 +
                      tileY * 2
            );

            const uint8_t low = bus.read8(tileAddress);
            const uint8_t high = bus.read8(tileAddress + 1);

            const int bit = 7 - tileX;

            const uint8_t colorId =
                ((low >> bit) & 1) |
                ((high >> bit) & 1) << 1;

            if (colorId == 0)
                continue;

            const int framebufferIndex =
                screenY * ScreenWidth + screenX;

            if (!(priority && bgColorIds[framebufferIndex] != 0)) {
                const uint8_t shade =
                    applyOBJPalette(
                        colorId,
                        useOBP1
                    );
                framebuffer[framebufferIndex] = dmgShadeToARGB(shade);
            }
            break;
        }
    }
}

uint8_t PPU::getWindowPixel(int x, int windowY) {
    const uint8_t lcdc = bus.read8(0xFF40);
    const uint8_t wy = bus.read8(0xFF4A);
    const uint8_t wx = bus.read8(0xFF4B);

    const int windowX = x - (static_cast<int>(wx) - 7);

    const uint16_t tileMapBase = (lcdc & 0x40) ? 0x9C00 : 0x9800;

    const uint8_t tileX =
        static_cast<uint8_t>(windowX / 8);
    const uint8_t tileY = static_cast<uint8_t>(windowY / 8);

    const uint16_t tileMapAddress =
        static_cast<uint16_t>(
            tileMapBase + tileY * 32 + tileX
        );
    const uint8_t tileNumber =
        bus.read8(tileMapAddress);

    uint16_t tileAddress;

    if (lcdc & 0x10) {
        tileAddress = static_cast<uint16_t>(0x8000 + static_cast<uint16_t>(tileNumber) * 16);
    } else {
        const int8_t signedTile = static_cast<int8_t>(tileNumber);

        tileAddress = static_cast<uint16_t>(0x9000 + static_cast<uint16_t>(signedTile) * 16);
    }

    const uint8_t pixelX = static_cast<uint8_t>(windowX % 8);
    const uint8_t pixelY = static_cast<uint8_t>(windowY % 8);

    const uint16_t rowAddress = static_cast<uint16_t>(tileAddress + pixelY * 2);

    const uint8_t low = bus.read8(rowAddress);
    const uint8_t high = bus.read8(rowAddress + 1);

    const uint8_t bit = static_cast<uint8_t>(7 - pixelX);
    const uint8_t lowBit = static_cast<uint8_t>(low >> bit) & 0x01;
    const uint8_t highBit = static_cast<uint8_t>(high >> bit) & 0x01;

    return static_cast<uint8_t>(lowBit | (highBit << 1));
}

void PPU::updateLYC() {
    const uint8_t ly = bus.getLY();
    const uint8_t lyc = bus.read8(0xFF45);

    uint8_t stat = bus.read8(0xFF41);

    if (ly == lyc) {
        stat |= 0x04;
    } else {
        stat &= ~0x04;
    }

    bus.write8(0xFF41, stat);
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



