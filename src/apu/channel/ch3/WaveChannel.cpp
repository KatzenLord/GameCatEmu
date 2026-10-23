//
// Created by katzenlord on 17.10.26.
//

#include "WaveChannel.h"

void WaveChannel::reset() {
    writeRegister(0, 0x7F);
    writeRegister(1, 0xFF);
    writeRegister(2, 0x9F);
    writeRegister(4, 0xBF);
}

void WaveChannel::writeRegister(int reg, uint8_t value) {
    regs[reg] = value;
    switch (reg) {
        case 0:
            dacEnabled = (regs[reg] & 0x80) != 0;
            if (!dacEnabled)
                enabled = false;
            return;
        case 1:
            lengthCounter = 256 - regs[reg];
            return;
        case 2:
            outputLevel = (regs[reg] >> 5) & 0x03;
            return;
        case 3:
            period = (period & 0x0700) | static_cast<uint16_t>(regs[reg]);
            return;
        case 4:
            period = (period & 0x00FF) | (static_cast<uint16_t>(regs[reg] & 0x07) << 8);
            lengthEnabled = (regs[reg] & 0x40) != 0;
            if (regs[reg] & 0x80) {
                if (lengthCounter == 0)
                    lengthCounter = 256;
                enabled = dacEnabled;
                waveIndex = 0;
                timer = (2048 - period) * 2;
            }
            return;
        default: return;
    }
}

uint8_t WaveChannel::readRegister(int reg) const {
    switch (reg) {
        case 0: return regs[reg] | 0x7F;
        case 2: return regs[reg] | 0x9F;
        case 4: return regs[reg] | 0xBF;
        default: return 0xFF;
    }
}

void WaveChannel::writeWaveRam(uint16_t address, uint8_t value) {
    waveRam[address - 0xFF30] = value;
}

uint8_t WaveChannel::readWaveRam(uint16_t address) const {
    return waveRam[address - 0xFF30];
}

void WaveChannel::tick(int cycles) {
    if (!enabled || !dacEnabled)
        return;

    timer -= cycles;

    while (timer <= 0) {
        timer += (2048 - period) * 2;
        waveIndex = (waveIndex + 1) & 0x1F;
    }
}

uint8_t WaveChannel::getWaveSample() const {
    const uint8_t byte = waveRam[waveIndex / 2];

    if ((waveIndex & 1) == 0)
        return byte >> 4;
    return byte & 0x0F;
}

float WaveChannel::getSample() const {
    if (!enabled || !dacEnabled)
        return 0.0f;

    uint8_t sample = getWaveSample();

    switch (outputLevel) {
        case 0:
            sample = 0; break;
        case 1:
            break;
        case 2:
            sample >>= 1; break;
        case 3:
            sample >>=2; break;
    }
    float normalized = static_cast<float>(sample) / 15.0f;
    normalized = normalized * 2.0f - 1.0f;

    return normalized * OUTPUT_VOLUME;
}

void WaveChannel::clockLength() {
    if (!lengthEnabled)
        return;

    if (lengthCounter > 0) {
        lengthCounter--;

        if (lengthCounter == 0)
            enabled= false;
    }
}

void WaveChannel::powerOff() {
    regs.fill(0);

    enabled = false;
    dacEnabled = false;

    timer = 0;
    lengthCounter = 0;
    lengthEnabled = false;

    outputLevel = 0;
    period = 0;
    waveIndex = 0;

    leftEnabled = false;
    rightEnabled = false;
}
