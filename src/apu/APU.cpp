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

        uint8_t pos = 1;
        for (AudioChannel* ch: audioChannels) {
            ch->setLeft((nr51 & (pos << 4)) != 0);
            ch->setRight((nr51 & pos) != 0);
            pos <<= 1;
        }
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
        }
        else {
            nr52 = 0x80;
        }
        return;
    }
    // Channel 1
    if (address == 0xFF10) {
        ch1.writeRegister(0, value); return;
    }
    if (address == 0xFF11) {
        ch1.writeRegister(1, value); return;
    }
    if (address == 0xFF12) {
        ch1.writeRegister(2, value); return;
    }
    if (address == 0xFF13) {
        ch1.writeRegister(3, value); return;
    }
    if (address == 0xFF14) {
        ch1.writeRegister(4, value); return;
    }

    // Channel 2
    if (address == 0xFF16) {
        ch2.writeRegister(1, value); return;
    }
    if (address == 0xFF17) {
        ch2.writeRegister(2, value); return;
    }
    if (address == 0xFF18) {
        ch2.writeRegister(3, value); return;
    }
    if (address == 0xFF19) {
        ch2.writeRegister(4, value); return;
    }

    // Channel 3
    if (address == 0xFF1A) {
        ch3.writeRegister(0, value); return;
    }
    if (address == 0xFF1B) {
        ch3.writeRegister(1, value); return;
    }
    if (address == 0xFF1C) {
        ch3.writeRegister(2, value); return;
    }
    if (address == 0xFF1D) {
        ch3.writeRegister(3, value); return;
    }
    if (address == 0xFF1E) {
        ch3.writeRegister(4, value); return;
    }
    if (address >= 0xFF30 && address <= 0xFF3F) {
        ch3.writeWaveRam(address, value); return;
    }

    // Channel 4
    if (address == 0xFF20) {
        ch4.writeRegister(1, value); return;
    }
    if (address == 0xFF21) {
        ch4.writeRegister(2, value); return;
    }
    if (address == 0xFF22) {
        ch4.writeRegister(3, value); return;
    }
    if (address == 0xFF23) {
        ch4.writeRegister(4, value); return;
    }
    return;
}

uint8_t APU::readReg(uint16_t address) const {
    if (address >= 0xFF10 && address <= 0xFF14) {
        return ch1.readRegister(address - 0xFF10);
    }
    if (address >= 0xFF15 && address <= 0xFF19) {
        return ch2.readRegister(address - 0xFF15);
    }
    if (address >= 0xFF1A && address <= 0xFF1E) {
        return ch3.readRegister(address - 0xFF1A);
    }
    if (address >= 0xFF20 && address <= 0xFF23) {
        return ch4.readRegister(address - 0xFF20);
    }
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
            uint8_t pos = 1;
            for (AudioChannel* ch: audioChannels) {
                if (ch->getEnabled()) result |= pos;
                pos <<= 1;
            }
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

void APU::generateSample() {
    float left = 0.0f;
    float right = 0.0f;

    for (AudioChannel* ch: audioChannels) {
        const float sample = ch->getSample();
        if (ch->getLeft()) left += sample;
        if (ch->getRight()) right += sample;
    }

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
        ch4.clockLength();

        if (frameSequencerStep == 2 || frameSequencerStep == 6) {
            ch1.clockSweep();
        }
    } else if (frameSequencerStep == 7) {
        ch1.clockEnvelope();
        ch2.clockEnvelope();
        ch4.clockEnvelope();
    }
    frameSequencerStep = (frameSequencerStep + 1) & 0x07;
}

const std::vector<float> &APU::getAudioBuffer() const {
    return audioBuffer;
}

void APU::clearAudioBuffer() {
    audioBuffer.clear();
}