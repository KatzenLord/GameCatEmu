//
// Created by katzenlord on 05.10.26.
//

#include "APU.h"

#include <iostream>

APU::APU() {
    reset();
}

void APU::reset() {
    for (AudioChannel* ch: audioChannels) {
        ch->reset();
    }

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

        ch1.setLeft((nr51 & 0x10) != 0);
        ch1.setRight((nr51 & 0x01) != 0);

        ch2.setLeft((nr51 & 0x20) != 0);
        ch2.setRight((nr51 & 0x02) != 0);

        ch3.setLeft((nr51 & 0x40) != 0);
        ch3.setRight((nr51 & 0x04) != 0);

        ch4Left = (nr51 & 0x80) != 0;
        ch4Right = (nr51 & 0x08) != 0;

        return;
    }
    if (address == 0xFF26) {

        const bool enable = (value & 0x80) != 0;

        if (!enable) {
            nr52 = 0;

            for (AudioChannel* ch: audioChannels) {
                ch->setEnabled(false);
                ch->setDac(false);
            }

            ch4Enabled = false;
            ch4DacEnabled = false;
        }
        else {
            nr52 = 0x80;
        }

        return;
    }
    // Channel 1
    if (address == 0xFF10) {
        ch1.writeRegister(0, value);
        return;
    }
    if (address == 0xFF11) {
        ch1.writeRegister(1, value);
        return;
    }
    if (address == 0xFF12) {
        ch1.writeRegister(2, value);
        return;
    }
    if (address == 0xFF13) {
        ch1.writeRegister(3, value);
        return;
    }
    if (address == 0xFF14) {
        ch1.writeRegister(4, value);
        return;
    }

    // Channel 2
    if (address == 0xFF16) {
        ch2.writeRegister(1, value);
        return;
    }
    if (address == 0xFF17) {
        ch2.writeRegister(2, value);
        return;
    }
    if (address == 0xFF18) {
        ch2.writeRegister(3, value);
        return;
    }
    if (address == 0xFF19) {
        ch2.writeRegister(4, value);
        return;
    }

    // Channel 3
    if (address == 0xFF1A) {
        ch3.writeRegister(0, value);
        return;
    }
    if (address == 0xFF1B) {
        ch3.writeRegister(1, value);
        return;
    }
    if (address == 0xFF1C) {
        writeReg(2, value);
        return;
    }
    if (address == 0xFF1D) {
        ch3.writeRegister(3, value);
        return;
    }
    if (address == 0xFF1E) {
        ch3.writeRegister(4, value);
        return;
    }
    if (address >= 0xFF30 && address <= 0xFF3F) {
        ch3.writeWaveRam(address, value);
        return;
    }

    // Channel 4
    if (address == 0xFF20) {
        nr41 = value;
        ch4LengthCounter = 64 - (nr41 & 0x3F);
        return;
    }
    if (address == 0xFF21) {
        nr42 = value;
        ch4InitialVolume = (nr42 >> 4) & 0x0F;

        ch4EnvelopeIncrease = (nr42 & 0x08) != 0;
        ch4EnvelopePace = nr42 & 0x07;

        ch4DacEnabled = (nr42 & 0xF8) != 0;
        if (!ch4DacEnabled) {
            ch4Enabled = false;
        }
        return;
    }
    if (address == 0xFF22) {
        nr43 = value;
        ch4ClockShift = (nr43 >> 4) & 0x0F;
        ch4LSFRWidth = (nr43 & 0x08) != 0;
        ch4DividerCode = nr43 & 0x07;

        return;
    }
    if (address == 0xFF23) {
        nr44 = value;
        ch4LengthEnabled = (nr44 & 0x40) != 0;
        if ((nr44 & 0x80) != 0) {
            ch4Enabled = ch4DacEnabled;
            if (ch4LengthCounter == 0) {
                ch4LengthCounter = 64;
            }
            ch4Volume = ch4InitialVolume;
            ch4EnvelopeTimer = (ch4EnvelopePace == 0) ? 8 : ch4EnvelopePace;
            ch4LSFR = 0x7FFF;
            ch4Timer = NOISE_DIVISORS[ch4DividerCode] << ch4ClockShift;
        }
    }
    return;
}

uint8_t APU::readReg(uint16_t address) const {
    if (address >= 0xFF30 && address <= 0xFF3F) {
        return ch3.readWaveRam(address);
    }
    switch (address) {
        case 0xFF24:
            return nr50;

        case 0xFF25:
            return nr51;

        case 0xFF26: {
            uint8_t result = 0x70;

            if (nr52 & 0x80) result |= 0x80;
            if (ch1.getEnabled()) result |= 0x01;
            if (ch2.getEnabled()) result |= 0x02;
            if (ch3.getEnabled()) result |= 0x04;
            if (ch4Enabled) result |= 0x08;

            return result;
        }
        default:
            return 0xFF;
    }
}

void APU::tick(int cycles) {
    for (AudioChannel* ch: audioChannels) {
        ch->tick(cycles);
    }
    tickChannel4(cycles);

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

void APU::tickChannel4(int cycles) {
    if (!ch4Enabled || !ch4DacEnabled)
        return;

    ch4Timer -= cycles;

    while (ch4Timer <= 0) {
        ch4Timer += NOISE_DIVISORS[ch4DividerCode] << ch4ClockShift;
        const uint8_t xorBit = (ch4LSFR & 0x01) ^ ((ch4LSFR >> 1) & 0x01);
        ch4LSFR >>= 1;
        ch4LSFR |= static_cast<uint16_t>(xorBit) << 14;

        if (ch4LSFRWidth) {
            ch4LSFR &= ~(1 << 6);
            ch4LSFR |= xorBit << 6;
        }
    }
}

float APU::getChannel4Sample() const {
    if (!ch4Enabled || !ch4DacEnabled)
        return 0.0f;

    bool high = !(ch4LSFR & 0x01);

    return high
        ? static_cast<float>(ch4Volume) / 15.0f * 0.05f
        : -static_cast<float>(ch4Volume) / 15.0f * 0.05f;
}

void APU::generateSample() {
    float ch4 = getChannel4Sample();

    float left = 0.0f;
    float right = 0.0f;

    for (const AudioChannel* ch: audioChannels) {
        const float sample = ch->getSample();
        if (ch->getLeft()) left += sample;
        if (ch->getRight()) right += sample;
    }

    if (ch4Left) left += ch4;
    if (ch4Right) right += ch4;

    const float leftVolume = static_cast<float>(((nr50 >> 4) & 0x07) + 1) / 8.0f;
    const float rightVolume = static_cast<float>((nr50 & 0x07) + 1) / 8.0f;

    left *= leftVolume;
    right *= rightVolume;

    audioBuffer.push_back(left);
    audioBuffer.push_back(right);
}

void APU::clockFrameSequencer() {

    if (frameSequencerStep == 0 ||
        frameSequencerStep == 2 ||
        frameSequencerStep == 4 ||
        frameSequencerStep == 6) {

        ch1.clockLength();
        ch2.clockLength();
        ch3.clockLength();
        clockChannel4Length();

        if (frameSequencerStep == 2 || frameSequencerStep == 6) {
            ch1.clockSweep();
        }
    } else if (frameSequencerStep == 7) {
        ch1.clockEnvelope();
        ch2.clockEnvelope();
        clockChannel4Envelope();
    }
    frameSequencerStep = (frameSequencerStep + 1) & 0x07;
}

void APU::clockChannel4Length() {
    if (!ch4LengthEnabled)
        return;

    if (ch4LengthCounter > 0) {
        ch4LengthCounter--;
        if (ch4LengthCounter == 0) {
            ch4Enabled = false;
        }
    }
}

void APU::clockChannel4Envelope() {
    if (ch4EnvelopePace == 0)
        return;
    if (ch4EnvelopeTimer > 0) {
        ch4EnvelopeTimer--;
    }
    if (ch4EnvelopeTimer == 0) {
        ch4EnvelopeTimer = ch4EnvelopePace;
        if (ch4EnvelopeIncrease) {
            if (ch4Volume < 15) {
                ch4Volume++;
            } else {
                if (ch4Volume > 0) {
                    ch4Volume--;
                }
            }
        }
    }
}

const std::vector<float> &APU::getAudioBuffer() const {
    return audioBuffer;
}

void APU::clearAudioBuffer() {
    audioBuffer.clear();
}