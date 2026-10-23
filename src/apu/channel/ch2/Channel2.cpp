//
// Created by katzenlord on 16.10.26.
//

#include "Channel2.h"

void Channel2::reset() {
    writeRegister(1, 0x3F, false);
    writeRegister(2, 0x00, false);
    writeRegister(4, 0xBF, false);
}

void Channel2::writeRegister(int reg, uint8_t value, bool shouldExtraClock) {
    regs[reg] = value;
    switch (reg) {
        case 1: {
            duty = (regs[reg] >> 6) & 0x03;
            lengthCounter = 64 - (regs[reg] & 0x3F);
            return;
        }
        case 2: {
            initialVolume = (regs[reg] >> 4) & 0x0F;
            envelopeIncrease = (regs[reg] & 0x08) != 0;
            envelopePace = regs[reg] & 0x07;
            dacEnabled = (regs[reg] & 0xF8) != 0;
            if (!dacEnabled)
                enabled = false;
            return;
        }
        case 3: {
            period = (period & 0x0700) | static_cast<uint16_t>(regs[reg]);
            return;
        }
        case 4: {
            bool oldLengthEnabled = lengthEnabled;

            period = (period & 0x00FF) | (static_cast<uint16_t>(regs[reg] & 0x07) << 8);

            if (!oldLengthEnabled &&
                lengthCounter &&
                shouldExtraClock &&
                lengthCounter > 0) {
                lengthCounter--;

                if (lengthCounter == 0)
                    enabled = false;
                }

            lengthEnabled = (regs[reg] & 0x40) != 0;
            if ((regs[reg] & 0x80)) {
                if (lengthCounter == 0) {
                    lengthCounter = 64;

                    if (lengthEnabled && shouldExtraClock)
                        lengthCounter--;
                }
                enabled = dacEnabled;
                volume = initialVolume;
                envelopeTimer = (envelopePace == 0) ? 8 : envelopePace;
                dutyPosition = 0;
                timer = (2048 - period) * 4;
            }
            return;
        }
        default: return;
    }
}

uint8_t Channel2::readRegister(int reg) const {
    switch (reg) {
        case 1: return regs[reg] | 0x3F;
        case 2: return regs[reg];
        case 4: return regs[reg] | 0xBF;
        default: return 0xFF;
    }
}
