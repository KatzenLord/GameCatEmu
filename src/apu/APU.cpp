//
// Created by katzenlord on 05.10.26.
//

#include "APU.h"

void APU::writeReg(uint16_t address, uint8_t value) {
    if (address == 0xFF16) {
        nr21 = value;
        ch2Duty = (nr21 >> 6) & 0x0F;
        return;
    }
    if (address == 0xFF17) {
        nr22 = value;
        ch2InitialVolume = (nr22 >> 4) & 0x0F;
        ch2DacEnabled = (nr22 & 0xF8) != 0;
        if (!ch2DacEnabled) {
            ch2Enabled = false;
        }
        return;
    }
    if (address == 0xFF18) {
        nr23 = value;
        ch2Period = (ch2Period & 0x0700) |static_cast<uint16_t>(nr23);
        return;
    }
    if (address == 0xFF19) {
        nr24 = value;
        ch2Period = (ch2Period & 0x00FF) | (static_cast<uint16_t>(nr24) << 8);
        if ((nr24 & 0x80) != 0) {
            ch2Enabled = ch2DacEnabled;
            ch2Volume = ch2InitialVolume;
            ch2DutyPosition = 0;
            ch2Timer = (2048 - ch2Period) * 4;
        }
        return;
    }
    return;
}

void APU::tick(int cycles) {
    tickChannel2(cycles);

    sampleCounter += cycles;

    constexpr double cyclesPerSample = static_cast<double>(CPU_CLOCK) / SAMPLE_RATE;

    while (sampleCounter >= cyclesPerSample) {
        sampleCounter -= cyclesPerSample;

        generateSample();
    }
}

void APU::tickChannel2(int cycles) {
    if (!ch2Enabled || !ch2DacEnabled)
        return;

    ch2Timer -= cycles;

    while (ch2Timer <= 0) {
        ch2Timer += (2048 - ch2Period) * 4;
        ch2DutyPosition = (ch2DutyPosition + 1) & 0x07;
    }
}

uint8_t APU::getChannel2Output() const {
    if (!ch2Enabled || !ch2DacEnabled)
        return 0;

    return DUTY_TABLE[ch2Duty][ch2DutyPosition] ? ch2Volume : 0;
}

float APU::getChannel2Sample() const {
    if (!ch2Enabled || !ch2DacEnabled)
        return 0.0f;

    const bool high =
        DUTY_TABLE[ch2Duty][ch2DutyPosition];

    return high
        ? static_cast<float>(ch2Volume) / 15.0f
        : -static_cast<float>(ch2Volume) / 15.0f;
}

void APU::generateSample() {
    float sample = getChannel2Sample();
    audioBuffer.push_back(sample);
}

const std::vector<float> &APU::getAudioBuffer() const {
    return audioBuffer;
}

void APU::clearAudioBuffer() {
    audioBuffer.clear();
}
