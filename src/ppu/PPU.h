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
    uint8_t getBackgroundPixel(int x, int y);
    uint8_t applyBGPalette(uint8_t colorId) const;
    uint32_t dmgShadeToARGB(uint8_t shade) const;
    void renderFrame();
    bool frameReady() const;
    void clearFrameReady();

    std::array<uint32_t, ScreenWidth * ScreenHeight> getFramebuffer() const;

    void debugTileMap();
    void debugTile(uint8_t tileNumber);
    void debugUsedTiles();
private:
    Bus& bus;

    int scanlineCycles = 0;
    void requestVBlankInterrupt();
    std::array<uint32_t, ScreenWidth * ScreenHeight> framebuffer{};

    bool frameReadyFlag = false;
};


#endif //GAMECATEMU_PPU_H
