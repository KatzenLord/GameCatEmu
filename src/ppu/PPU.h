//
// Created by katzenlord on 26.09.26.
//

#ifndef GAMECATEMU_PPU_H
#define GAMECATEMU_PPU_H
#pragma once
#include <array>
#include <cstdint>

static constexpr int ScreenWidth = 160;
static constexpr int ScreenHeight = 144;

class Bus;

class PPU {
public:
    explicit PPU(Bus& bus);
    void step(int cycles);

    bool frameReady() const;
    void clearFrameReady();

    std::array<uint32_t, ScreenWidth * ScreenHeight> getFramebuffer() const;

    void debugTileMap();
    void debugTile(uint8_t tileNumber);
    void debugUsedTiles();
private:
    Bus& bus;

    int scanlineCycles = 0;
    int windowLineCounter = 0;
    std::array<uint32_t, ScreenWidth * ScreenHeight> framebuffer{};
    std::array<uint8_t, ScreenWidth * ScreenHeight> bgColorIds{};

    bool frameReadyFlag = false;

    void updateLYC();
    void requestSTATInterrupt() const;
    void requestVBlankInterrupt() const;

    void renderScanline(int y);
    void renderSpritesForScanline(int y);
    uint8_t applyOBJPalette(uint8_t colorId, bool useOBP1) const;
    uint8_t getBackgroundPixel(int x, int y);
    uint8_t getWindowPixel(int x, int windowY);
    uint8_t applyBGPalette(uint8_t colorId) const;
    uint32_t dmgShadeToARGB(uint8_t shade) const;
};


#endif //GAMECATEMU_PPU_H
