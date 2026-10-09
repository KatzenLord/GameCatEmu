//
// Created by katzenlord on 05.10.26.
//

#include "APU.h"

#include <iostream>

APU::APU() {
    reset();
}

void APU::reset() {
    nr10 = 0x80;
    nr11 = 0xBF;
    nr12 = 0xF3;
    nr14 = 0xBF;

    nr21 = 0x3F;
    nr22 = 0x00;
    nr24 = 0xBF;

    nr30 = 0x7F;
    nr31 = 0xFF;
    nr32 = 0x9F;
    nr34 = 0xBF;

    nr41 = 0xFF;
    nr42 = 0x00;
    nr43 = 0x00;
    nr44 = 0xBF;

    nr50 = 0x77;
    nr51 = 0xF3;
    nr52 = 0xF1;
}

void APU::writeReg(uint16_t address, uint8_t value) {
    // Global Registers
    if (address == 0xFF24) {
        nr50 = value;

        leftVolume = (nr50 >> 4) & 0x07;
        rightVolume = nr50 & 0x07;

        return;
    }
    if (address == 0xFF25) {
        nr51 = value;

        ch1Left = (nr51 & 0x10) != 0;
        ch1Right = (nr51 & 0x01) != 0;

        ch2Left = (nr51 & 0x20) != 0;
        ch2Right = (nr51 & 0x02) != 0;

        ch3Left = (nr51 & 0x40) != 0;
        ch3Right = (nr51 & 0x04) != 0;

        return;
    }
    if (address == 0xFF26) {

        const bool enable = (value & 0x80) != 0;

        if (!enable) {
            nr52 = 0;

            ch2Enabled = false;
            ch2DacEnabled = false;

            ch1Enabled = false;
            ch1DacEnabled = false;

            ch3Enabled = false;
            ch3DacEnabled = false;
        }
        else {
            nr52 = 0x80;
        }

        return;
    }
    // Channel 1
    if (address == 0xFF10) {
        nr10 = value;

        ch1SweepPace = (nr10 >> 4) & 0x07;
        ch1SweepDecrease = (nr10 & 0x08) != 0;
        ch1SweepShift = nr10 & 0x07;

        return;
    }
    if (address == 0xFF11) {
        nr11 = value;
        ch1Duty = (nr11 >> 6) & 0x03;
        ch1LengthCounter = 64 - (nr11 & 0x3F);
        return;
    }
    if (address == 0xFF12) {
        nr12 = value;
        ch1InitialVolume = (nr12 >> 4) & 0x0F;

        ch1EnvelopeIncrease = (nr12 & 0x08) != 0;
        ch1EnvelopePace = nr12 & 0x07;

        ch1DacEnabled = (nr12 & 0xF8) != 0;
        if (!ch1DacEnabled) {
            ch1Enabled = false;
        }
        return;
    }
    if (address == 0xFF13) {
        nr13 = value;
        ch1Period = (ch1Period & 0x0700) | static_cast<uint16_t>(nr13);
        return;
    }
    if (address == 0xFF14) {
        nr14 = value;
        ch1Period = (ch1Period & 0x00FF) | (static_cast<uint16_t>(nr14 & 0x07) << 8);
        ch1LengthEnabled = (nr14 & 0x40) != 0;
        if ((nr14 & 0x80) != 0) {
            if (ch1LengthCounter == 0) {
                ch1LengthCounter = 64;
            }
            ch1Enabled = ch1DacEnabled;
            ch1Volume = ch1InitialVolume;
            ch1EnvelopeTimer = (ch1EnvelopePace == 0) ? 8 : ch1EnvelopePace;
            ch1DutyPosition = 0;
            ch1Timer = (2048 - ch1Period) * 4;
            ch1SweepTimer = (ch1SweepPace == 0) ? 8 : ch1SweepPace;
            ch1SweepShadowPeriod = ch1Period;
            ch1SweepEnabled = (ch1SweepPace != 0 || ch1SweepShift != 0);
        }
        return;
    }

    // Channel 2
    if (address == 0xFF16) {
        nr21 = value;
        ch2Duty = (nr21 >> 6) & 0x03;
        ch2LengthCounter = 64 - (nr21 & 0x3F);
        return;
    }
    if (address == 0xFF17) {
        nr22 = value;
        ch2InitialVolume = (nr22 >> 4) & 0x0F;

        ch2EnvelopeIncrease = (nr22 & 0x08) != 0;
        ch2EnvelopePace = nr22 & 0x07;

        ch2DacEnabled = (nr22 & 0xF8) != 0;
        if (!ch2DacEnabled) {
            ch2Enabled = false;
        }
        return;
    }
    if (address == 0xFF18) {
        nr23 = value;
        ch2Period = (ch2Period & 0x0700) | static_cast<uint16_t>(nr23);
        return;
    }
    if (address == 0xFF19) {
        nr24 = value;
        ch2Period = (ch2Period & 0x00FF) | (static_cast<uint16_t>(nr24 & 0x07) << 8);
        ch2LengthEnabled = (nr24 & 0x40) != 0;
        if ((nr24 & 0x80) != 0) {
            if (ch2LengthCounter == 0) {
                ch2LengthCounter = 64;
            }
            ch2Enabled = ch2DacEnabled;
            ch2Volume = ch2InitialVolume;
            ch2EnvelopeTimer = (ch2EnvelopePace == 0) ? 8 : ch2EnvelopePace;
            ch2DutyPosition = 0;
            ch2Timer = (2048 - ch2Period) * 4;
        }
        return;
    }

    // Channel 3
    if (address == 0xFF1A) {
        nr30 = value;
        ch3DacEnabled = (nr30 & 0x80) != 0;
        if (!ch3DacEnabled) {
            ch3Enabled = false;
        }
        return;
    }
    if (address == 0xFF1B) {
        nr31 = value;
        ch3LengthCounter = 256 - nr31;
        return;
    }
    if (address == 0xFF1C) {
        nr32 = value;
        ch3OutputLevel = (nr32 >> 5) & 0x03;
        return;
    }
    if (address == 0xFF1D) {
        nr33 = value;
        ch3Period = (ch3Period & 0x0700) | static_cast<uint16_t>(nr33);
        return;
    }
    if (address == 0xFF1E) {
        nr34 = value;
        ch3Period = (ch3Period & 0x00FF) | (static_cast<uint16_t>(nr34 & 0x07) << 8);
        ch3LengthEnabled = (nr34 & 0x40) != 0;
        if ((nr34 & 0x80) != 0) {
            ch3Enabled = ch3DacEnabled;
            if (ch3LengthCounter == 0) {
                ch3LengthCounter = 256;
            }
            ch3Volume = ch3OutputLevel;
            ch3WaveIndex = 0;
            ch3Timer = (2048 - ch3Period) * 2;
        }
        return;
    }
    if (address >= 0xFF30 && address <= 0xFF3F) {
        waveRam[address - 0xFF30] = value;
        return;
    }
    return;
}

uint8_t APU::readReg(uint16_t address) const {
    if (address >= 0xFF30 && address <= 0xFF3F) {
        return waveRam[address - 0xFF30];
    }
    switch (address) {
        case 0xFF24:
            return nr50;

        case 0xFF25:
            return nr51;

        case 0xFF26: {
            uint8_t result = 0x70;

            if (nr52 & 0x80)
                result |= 0x80;

            if (ch1Enabled)
                result |= 0x01;

            if (ch2Enabled)
                result |= 0x02;

            if (ch3Enabled)
                result |= 0x04;
            return result;
        }
        default:
            return 0xFF;
    }
}

void APU::tick(int cycles) {
    tickChannel1(cycles);
    tickChannel2(cycles);
    tickChannel3(cycles);

    frameSequencerTimer -= cycles;

    while (frameSequencerTimer <= 0) {
        frameSequencerTimer += 8192;
        clockFrameSequencer();
    }

    sampleCounter += cycles;

    constexpr double cyclesPerSample = static_cast<double>(CPU_CLOCK) / SAMPLE_RATE;

    while (sampleCounter >= cyclesPerSample) {
        sampleCounter -= cyclesPerSample;

        generateSample();
    }
}

void APU::tickChannel1(int cycles) {
    if (!ch1Enabled || !ch1DacEnabled)
        return;

    ch1Timer -= cycles;
    while (ch1Timer <= 0) {
        ch1Timer += (2048 - ch1Period) * 4;
        ch1DutyPosition = (ch1DutyPosition + 1) & 0x07;
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

void APU::tickChannel3(int cycles) {
    if (!ch3Enabled || !ch3DacEnabled)
        return;

    ch3Timer -= cycles;

    while (ch3Timer <= 0) {
        ch3Timer += (2048 - ch3Period) * 2;
        ch3WaveIndex = (ch3WaveIndex + 1) & 0x1F;
    }
}

float APU::getChannel1Sample() const {
    if (!ch1Enabled || !ch1DacEnabled)
        return 0.0f;

    const bool high =
        DUTY_TABLE[ch1Duty][ch1DutyPosition];

    return high
        ? static_cast<float>(ch1Volume) / 15.0f * 0.05f
        : -static_cast<float>(ch1Volume) / 15.0f * 0.05f;
}

float APU::getChannel2Sample() const {
    if (!ch2Enabled || !ch2DacEnabled)
        return 0.0f;

    const bool high =
        DUTY_TABLE[ch2Duty][ch2DutyPosition];

    return high
        ? static_cast<float>(ch2Volume) / 15.0f * 0.05f
        : -static_cast<float>(ch2Volume) / 15.0f * 0.05f;
}

uint8_t APU::getChannel3WaveSample() const {
    const uint8_t byte = waveRam[ch3WaveIndex / 2];

    if ((ch3WaveIndex & 1) == 0) {
        return byte >> 4;
    }
    return byte & 0x0F;
}

float APU::getChannel3Sample() const {
    if (!ch3Enabled || !ch3DacEnabled)
        return 0.0f;

    uint8_t sample = getChannel3WaveSample();

    switch (ch3OutputLevel) {
        case 0:
            sample = 0;
            break;
        case 1:
            break;
        case 2:
            sample >>= 1;
            break;
        case 3:
            sample >>= 2;
            break;
    }

    float normalized = static_cast<float>(sample) / 15.0f;
    normalized = normalized * 2.0f - 1.0f;

    return normalized * 0.05f;
}

void APU::generateSample() {
    float ch1 = getChannel1Sample();
    float ch2 = getChannel2Sample();
    float ch3 = getChannel3Sample();

    float left = 0.0f;
    float right = 0.0f;

    if (ch1Left) {
        left += ch1;
    }
    if (ch1Right) {
        right += ch1;
    }
    if (ch2Left) {
        left += ch2;
    }
    if (ch2Right) {
        right += ch2;
    }
    if (ch3Left) {
        left += ch3;
    }
    if (ch3Right) {
        right += ch3;
    }

    const float leftVolume = static_cast<float>(((nr50 >> 4) & 0x07) + 1) / 8.0f;
    const float rightVolume = static_cast<float>((nr50 & 0x07) + 1) / 8.0f;

    left *= leftVolume;
    right *= rightVolume;

    audioBuffer.push_back(left);
    audioBuffer.push_back(right);
}

void APU::clockFrameSequencer() {
    switch (frameSequencerStep) {
        case 0:
            clockChannel1Length();
            clockChannel2Length();
            clockChannel3Length();
        case 2:
            clockChannel1Length();
            clockChannel2Length();
            clockChannel1Sweep();
            clockChannel3Length();
        case 4:
            clockChannel1Length();
            clockChannel2Length();
            clockChannel3Length();
            break;
        case 6:
            clockChannel1Length();
            clockChannel2Length();
            clockChannel1Sweep();
            clockChannel3Length();
            break;
        case 7:
            clockChannel1Envelope();
            clockChannel2Envelope();
            break;
    }
    frameSequencerStep = (frameSequencerStep + 1) & 0x07;
}

void APU::clockChannel1Sweep() {
    if (!ch1SweepEnabled)
        return;

    if (ch1SweepTimer > 0)
        ch1SweepTimer--;

    if (ch1SweepTimer != 0)
        return;

    ch1SweepTimer = (ch1SweepPace == 0) ? : ch1SweepPace;

    if (ch1SweepShift <= 0)
        return;

    uint16_t delta = ch1SweepShadowPeriod >> ch1SweepShift;

    uint16_t newPeriod;

    if (ch1SweepDecrease) {
        newPeriod = ch1SweepShadowPeriod - delta;
    } else {
        newPeriod = ch1SweepShadowPeriod + delta;
    }

    if (newPeriod > 2047) {
        ch1Enabled = false;
        return;
    }

    ch1SweepShadowPeriod = newPeriod;
    ch1Period = newPeriod;
}

void APU::clockChannel1Length() {
    if (!ch1LengthEnabled)
        return;

    if (ch1LengthCounter > 0) {
        ch1LengthCounter--;

        if (ch1LengthCounter == 0) {
            ch1Enabled = false;
        }
    }
}

void APU::clockChannel1Envelope() {
    if (ch1EnvelopePace == 0)
        return;

    if (ch1EnvelopeTimer > 0) {
        ch1EnvelopeTimer--;
    }

    if (ch1EnvelopeTimer == 0) {
        ch1EnvelopeTimer = ch1EnvelopePace;

        if (ch1EnvelopeIncrease) {
            if (ch1Volume < 15) {
                ch1Volume++;
            } else {
                if (ch1Volume > 0) {
                    ch1Volume--;
                }
            }
        }
    }
}

void APU::clockChannel2Length() {
    if (!ch2LengthEnabled)
        return;

    if (ch2LengthCounter > 0) {
        ch2LengthCounter--;

        if (ch2LengthCounter == 0) {
            ch2Enabled = false;
        }
    }
}

void APU::clockChannel2Envelope() {
    if (ch2EnvelopePace == 0)
        return;

    if (ch2EnvelopeTimer > 0) {
        ch2EnvelopeTimer--;
    }

    if (ch2EnvelopeTimer == 0) {
        ch2EnvelopeTimer = ch2EnvelopePace;

        if (ch2EnvelopeIncrease) {
            if (ch2Volume < 15) {
                ch2Volume++;
            } else {
                if (ch2Volume > 0) {
                    ch2Volume--;
                }
            }
        }
    }
}

void APU::clockChannel3Length() {
    if (!ch3LengthEnabled)
        return;

    if (ch3LengthCounter > 0) {
        ch3LengthCounter--;

        if (ch3LengthCounter == 0) {
            ch3Enabled = false;
        }
    }
}

const std::vector<float> &APU::getAudioBuffer() const {
    return audioBuffer;
}

void APU::clearAudioBuffer() {
    audioBuffer.clear();
}