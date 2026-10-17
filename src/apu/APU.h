//
// Created by katzenlord on 05.10.26.
//

#ifndef GAMECATEMU_APU_H
#define GAMECATEMU_APU_H

#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "channel/AudioChannel.h"
#include "channel/ch1/Channel1.h"
#include "channel/ch2/Channel2.h"
#include "channel/ch3/WaveChannel.h"
#include "channel/ch4/NoiseChannel.h"

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CPU_CLOCK = 4194304;

class APU {
public:
    APU();

    void tick(int cycles);

    void clearAudioBuffer();
    const std::vector<float>& getAudioBuffer() const;

    uint8_t readReg(uint16_t address) const;
    void writeReg(uint16_t address, uint8_t value);
private:
    void reset();

    double sampleCounter = 0.0;

    std::vector<float> audioBuffer;

    Channel1 ch1;
    Channel2 ch2;
    WaveChannel ch3;
    NoiseChannel ch4;

    std::array<AudioChannel*, 4> audioChannels {
        &ch1,
        &ch2,
        &ch3,
        &ch4
    };

    void generateSample();

    int frameSequencerTimer = 8192;
    uint8_t frameSequencerStep = 0;

    void clockFrameSequencer();

    // Global Registers
    uint8_t nr50 = 0;
    uint8_t nr51 = 0;
    uint8_t nr52 = 0;

    uint8_t leftVolume = 0;
    uint8_t rightVolume = 0;
};

#endif //GAMECATEMU_APU_H