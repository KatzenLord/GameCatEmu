//
// Created by katzenlord on 05.10.26.
//

#ifndef GAMECATEMU_APU_H
#define GAMECATEMU_APU_H

#pragma once
#include <array>
#include <cstdint>
#include <vector>

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CPU_CLOCK = 4194304;

static constexpr uint8_t DUTY_TABLE[4][8] = {
    {0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {0,1,1,1,1,1,1,0}
};

class APU {
public:
    void tick(int cycles);

    void clearAudioBuffer();
    const std::vector<float>& getAudioBuffer() const;

    uint8_t readReg(uint16_t address) const;
    void writeReg(uint16_t address, uint8_t value);
private:
    double sampleCounter = 0.0;

    std::vector<float> audioBuffer;

    void tickChannel1(int cycles);
    void tickChannel2(int cycles);
    void tickChannel3(int cycles);
    void tickChannel4(int cycles);

    uint8_t getChannel2Output() const;
    float getChannel2Sample() const;

    void generateSample();


    // Global Registers
    uint8_t nr50 = 0;
    uint8_t nr51 = 0;
    uint8_t nr52 = 0;

    // Channel 1
    uint8_t nr10 = 0;
    uint8_t nr11 = 0;
    uint8_t nr12 = 0;
    uint8_t nr13 = 0;
    uint8_t nr14 = 0;

    // Channel 2
    uint8_t nr21 = 0;
    uint8_t nr22 = 0;
    uint8_t nr23 = 0;
    uint8_t nr24 = 0;

    // Channel 2 states
    bool ch2Enabled = false;
    bool ch2DacEnabled = false;

    uint16_t ch2Period = 0;
    int ch2Timer = 0;

    uint8_t ch2Duty = 0;
    uint8_t ch2DutyPosition = 0;
    uint8_t ch2Volume = 0;
    uint8_t ch2InitialVolume = 0;

    // Channel 3
    uint8_t nr30 = 0;
    uint8_t nr31 = 0;
    uint8_t nr32 = 0;
    uint8_t nr33 = 0;
    uint8_t nr34 = 0;
    std::array<uint8_t, 16> waveRam{};

    // Channel 4
    uint8_t nr41 = 0;
    uint8_t nr42 = 0;
    uint8_t nr43 = 0;
    uint8_t nr44 = 0;
};

#endif //GAMECATEMU_APU_H