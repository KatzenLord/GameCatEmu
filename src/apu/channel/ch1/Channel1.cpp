//
// Created by katzenlord on 16.10.26.
//

#include "Channel1.h"

void Channel1::reset() {
    writeRegister(0, 0x80);
    writeRegister(1, 0xBF);
    writeRegister(2, 0xF3);
    writeRegister(4, 0xBF);
}

void Channel1::clockSweep() {
    if (!sweepEnabled)
        return;

    if (sweepTimer > 0)
        sweepTimer--;

    if (sweepTimer != 0)
        return;

    sweepTimer = (sweepPace == 0) ? 8 : sweepPace;

    if (sweepShift <= 0)
        return;

    const uint16_t delta = sweepShadowPeriod >> sweepShift;
    uint16_t newPeriod;

    if (sweepDecrease) {
        newPeriod = sweepShadowPeriod - delta;
    } else {
        newPeriod = sweepShadowPeriod + delta;
    }

    if (newPeriod > 2047) {
        enabled = false;
        return;
    }

    sweepShadowPeriod = newPeriod;
    period = newPeriod;
}

void Channel1::writeRegister(int reg, uint8_t value) {
    regs[reg] = value;
    switch (reg) {
        case 0:
            sweepPace = (regs[reg] >> 4) & 0x07;
            sweepDecrease = (regs[reg] & 0x08) != 0;
            sweepShift = regs[reg] & 0x07;
            return;
        case 1:
            duty = (regs[reg] >> 6) & 0x03;
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
            period = (period & 0x0700) | static_cast<uint16_t>(regs[reg]);
            return;
        case 4:
            period = (period & 0x00FF) | (static_cast<uint16_t>(regs[reg] & 0x07) << 8);
            lengthEnabled = (regs[reg] & 0x40) != 0;
            if ((regs[reg] & 0x80)) {
                if (lengthCounter == 0)
                    lengthCounter = 64;

                enabled = dacEnabled;
                volume = initialVolume;
                envelopeTimer = (envelopePace == 0) ? 8 : envelopePace;
                dutyPosition = 0;
                timer = (2048 - period) * 4;
                sweepTimer = (sweepPace == 0) ? 8 : sweepPace;
                sweepShadowPeriod = period;
                sweepEnabled = (sweepPace != 0 || sweepShift != 0);
            }
            return;
        default: return;
    }
}

uint8_t Channel1::readRegister(int reg) const {
    switch (reg) {
        case 0: return regs[reg];
        case 1: return regs[reg] | 0x3F;
        case 2: return regs[reg];
        case 4: return regs[reg] | 0xBF;
        default: return 0xFF;
    }
}

void Channel1::powerOff() {
    PulseChannel::powerOff();

    sweepPace = 0;
    sweepDecrease = false;
    sweepShift = 0;
    sweepTimer = 0;
    sweepShadowPeriod = 0;
    sweepEnabled = false;
}
