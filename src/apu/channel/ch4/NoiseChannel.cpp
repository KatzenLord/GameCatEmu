//
// Created by katzenlord on 17.10.26.
//

#include "NoiseChannel.h"

void NoiseChannel::reset() {
    writeRegister(1, 0xFF);
    writeRegister(2, 0x00);
    writeRegister(3, 0x00);
    writeRegister(4, 0xBF);
}

void NoiseChannel::writeRegister(int reg, uint8_t value) {
    regs[reg] = value;
    switch (reg) {
        case 1:
            lengthCounter = 64 - (regs[reg] & 0x3F);
            return;
        case 2:
            initialVolume = (regs[reg] >> 4) & 0x0F;
            envelopeIncrease = (regs[reg] & 0x08) != 0;
            envelopePace = regs[reg] & 0x07;

            dacEnabled = (regs[reg] & 0xF8) != 0;
            if (!dacEnabled)
                enabled = false;
            return;
        case 3:
            clockShift = (regs[reg] >> 4) & 0x0F;
            LSFRWidth = (regs[reg] & 0x08) != 0;
            divider = regs[reg] & 0x07;
            return;
        case 4:
            lengthEnabled = (regs[reg] & 0x40) != 0;
            if (regs[reg] & 0x80) {
                if (lengthCounter == 0)
                    lengthCounter = 64;
                enabled = dacEnabled;
                volume = initialVolume;
                envelopeTimer = (envelopePace == 0) ? 8 : envelopePace;
                LSFR = 0x7FFF;
                timer = NOISE_DIVISORS[divider] << clockShift;
            }
        default: return;
    }
}

uint8_t NoiseChannel::readRegister(int reg) {
    return regs[reg];
}

void NoiseChannel::tick(int cycles) {
    if (!enabled || !dacEnabled)
        return;

    timer -= cycles;

    while (timer <= 0) {
        timer += NOISE_DIVISORS[divider] << clockShift;
        const uint8_t xorBit = (LSFR & 0x01) ^ ((LSFR >> 1) & 0x01);
        LSFR >>= 1;
        LSFR |= static_cast<uint16_t>(xorBit) << 14;

        if (LSFRWidth) {
            LSFR &= ~(1 << 6);
            LSFR |= xorBit << 6;
        }
    }
}

float NoiseChannel::getSample() const {
    if (!enabled || !dacEnabled)
        return 0.0f;

    bool high = !(LSFR & 0x01);

    return high
        ? static_cast<float>(volume) / 15.0f * OUTPUT_VOLUME
        : -static_cast<float>(volume) / 15.0f * OUTPUT_VOLUME;
}

void NoiseChannel::clockLength() {
    if (!lengthEnabled)
        return;

    if (lengthCounter > 0) {
        lengthCounter--;
        if (lengthCounter == 0)
            enabled = false;
    }
}

void NoiseChannel::clockEnvelope() {
    if (envelopePace == 0)
        return;

    if (envelopeTimer > 0) {
        envelopeTimer--;
    }

    if (envelopeTimer == 0) {
        envelopeTimer = envelopePace;

        if (envelopeIncrease) {
            if (volume < 15) {
                volume++;
            }
        } else {
            if (volume > 0)
                volume--;
        }
    }
}
