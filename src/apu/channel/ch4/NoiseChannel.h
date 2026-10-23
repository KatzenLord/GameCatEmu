//
// Created by katzenlord on 17.10.26.
//

#ifndef GAMECATEMU_NOISECHANNEL_H
#define GAMECATEMU_NOISECHANNEL_H
#include "../AudioChannel.h"

static constexpr int NOISE_DIVISORS[8] = {
    8, 16, 32, 48, 64, 80, 96, 112
};

class NoiseChannel : public AudioChannel {
public:
    void tick(int cycles) override;
    void reset() override;

    float getSample() const override;

    void writeRegister(int reg, uint8_t value, bool shouldExtraClock) override;
    uint8_t readRegister(int reg) const override;

    void clockLength();
    void clockEnvelope();

    void powerOff() override;
protected:
    uint8_t initialVolume = 0;
    uint8_t volume = 0;

    uint8_t envelopeTimer = 0;
    uint8_t envelopePace = 0;
    bool envelopeIncrease = false;

    uint8_t clockShift = 0;
    bool LSFRWidth = false;
    uint8_t divider = 0.0;

    uint16_t LSFR = 0;
};


#endif //GAMECATEMU_NOISECHANNEL_H
