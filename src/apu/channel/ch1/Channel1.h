//
// Created by katzenlord on 16.10.26.
//

#ifndef GAMECATEMU_CHANNEL1_H
#define GAMECATEMU_CHANNEL1_H
#include "../pulse/PulseChannel.h"


class Channel1 : public PulseChannel{
public:
    void clockSweep();
    void reset() override;
    void writeRegister(int reg, uint8_t value) override;
    uint8_t readRegister(int reg) override;
protected:
    uint8_t sweepPace = 0;
    bool sweepDecrease = false;
    uint8_t sweepShift = 0;

    uint8_t sweepTimer = 0;
    uint16_t sweepShadowPeriod = 0;
    bool sweepEnabled = false;
};


#endif //GAMECATEMU_CHANNEL1_H
