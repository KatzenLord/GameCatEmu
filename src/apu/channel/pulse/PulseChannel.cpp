//
// Created by katzenlord on 16.10.26.
//

#include "PulseChannel.h"

#include <algorithm>
#include <iostream>

void PulseChannel::tick(int cycles) {
    if (!enabled || !dacEnabled)
        return;

    timer -= cycles;
    while (timer <= 0) {
        timer += (2048 - period) * 4;
        dutyPosition = (dutyPosition + 1) & 0x07;
    }
}

float PulseChannel::getSample() const {
    if (!enabled || !dacEnabled)
        return 0.0;

    const bool high =
        DUTY_TABLE[duty][dutyPosition];

    return high
        ? static_cast<float>(volume) / 15.0f * OUTPUT_VOLUME
        : -static_cast<float>(volume) / 15.0f * OUTPUT_VOLUME;
}

void PulseChannel::clockLength() {
    if (!lengthEnabled)
        return;

    if (lengthCounter > 0) {
        lengthCounter--;

        if (lengthCounter == 0) {
            enabled = false;
        }
    }
}

void PulseChannel::clockEnvelope() {
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

void PulseChannel::powerOff() {
    regs.fill(0);

    enabled = false;
    dacEnabled = false;

    timer = 0;
    lengthCounter = 0;
    lengthEnabled = false;

    duty = 0;
    dutyPosition = 0;

    period = 0;

    initialVolume = 0;
    volume = 0;

    envelopeTimer = 0;
    envelopePace = 0;
    envelopeIncrease = false;

    leftEnabled = false;
    rightEnabled = false;
}
