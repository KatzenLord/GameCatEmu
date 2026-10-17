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

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CPU_CLOCK = 4194304;

static constexpr int NOISE_DIVISORS[8] = {
    8, 16, 32, 48, 64, 80, 96, 112
};

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

    std::array<AudioChannel*, 3> audioChannels {
        &ch1,
        &ch2,
        &ch3
    };

    void tickChannel4(int cycles);

    float getChannel4Sample() const;

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

    // Channel 4
    uint8_t nr41 = 0;
    uint8_t nr42 = 0;
    uint8_t nr43 = 0;
    uint8_t nr44 = 0;

    bool ch4Enabled = false;
    bool ch4DacEnabled = false;

    uint16_t ch4LengthCounter = 0;
    bool ch4LengthEnabled = false;

    uint8_t ch4Volume = 0;
    uint8_t ch4InitialVolume = 0;
    int ch4Timer = 0;

    bool ch4EnvelopeIncrease = false;
    uint8_t ch4EnvelopePace = 0;
    uint8_t ch4EnvelopeTimer = 0;

    uint8_t ch4ClockShift = 0;
    bool ch4LSFRWidth = 0;
    uint8_t ch4DividerCode = 0.0;

    uint16_t ch4LSFR = 0;

    bool ch4Left = false;
    bool ch4Right = false;

    void clockChannel4Length();
    void clockChannel4Envelope();
};

#endif //GAMECATEMU_APU_H