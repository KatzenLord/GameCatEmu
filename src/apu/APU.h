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

    void tickChannel1(int cycles);
    void tickChannel2(int cycles);
    void tickChannel3(int cycles);
    void tickChannel4(int cycles);

    float getChannel1Sample() const;
    float getChannel2Sample() const;
    float getChannel3Sample() const;
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

    // Channel 1
    uint8_t nr10 = 0;
    uint8_t nr11 = 0;
    uint8_t nr12 = 0;
    uint8_t nr13 = 0;
    uint8_t nr14 = 0;

    // Channel 1 states
    bool ch1Enabled = false;
    bool ch1DacEnabled = false;

    uint8_t ch1SweepPace = 0;
    bool ch1SweepDecrease = false;
    uint8_t ch1SweepShift = 0;

    uint8_t ch1SweepTimer = 0;
    uint16_t ch1SweepShadowPeriod = 0;
    bool ch1SweepEnabled = false;

    uint16_t ch1Period = 0;
    int ch1Timer = 0;

    uint8_t ch1Duty = 0;
    uint8_t ch1DutyPosition = 0;
    uint8_t ch1Volume = 0;
    uint8_t ch1InitialVolume = 0;

    uint8_t ch1LengthCounter = 0;
    bool ch1LengthEnabled = false;

    bool ch1EnvelopeIncrease = false;
    uint8_t ch1EnvelopePace = 0;
    uint8_t ch1EnvelopeTimer = 0;

    bool ch1Left = false;
    bool ch1Right = false;

    void clockChannel1Length();
    void clockChannel1Envelope();
    void clockChannel1Sweep();

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

    uint8_t ch2LengthCounter = 0;
    bool ch2LengthEnabled = false;

    bool ch2EnvelopeIncrease = false;
    uint8_t ch2EnvelopePace = 0;
    uint8_t ch2EnvelopeTimer = 0;

    bool ch2Left = false;
    bool ch2Right = false;

    void clockChannel2Length();
    void clockChannel2Envelope();

    // Channel 3
    uint8_t nr30 = 0;
    uint8_t nr31 = 0;
    uint8_t nr32 = 0;
    uint8_t nr33 = 0;
    uint8_t nr34 = 0;
    std::array<uint8_t, 16> waveRam{};

    bool ch3Enabled = false;
    bool ch3DacEnabled = false;

    uint16_t ch3LengthCounter = 0;
    bool ch3LengthEnabled = false;

    uint8_t ch3Volume = 0;
    int ch3Timer;

    uint8_t ch3OutputLevel = 0;
    uint16_t ch3Period = 0;

    int ch3WaveIndex = 0;

    bool ch3Left = false;
    bool ch3Right = false;

    uint8_t getChannel3WaveSample() const;
    void clockChannel3Length();

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