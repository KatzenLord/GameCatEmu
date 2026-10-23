//
// Created by katzenlord on 16.10.26.
//

#ifndef GAMECATEMU_PULSECHANNEL_H
#define GAMECATEMU_PULSECHANNEL_H

#include "../AudioChannel.h"

static constexpr uint8_t DUTY_TABLE[4][8] = {
    {0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {0,1,1,1,1,1,1,0}
};

class PulseChannel : public AudioChannel {
protected:
    uint8_t duty = 0;
    uint8_t dutyPosition = 0;

    uint16_t period = 0;

    uint8_t initialVolume = 0;
    uint8_t volume = 0;

    uint8_t envelopeTimer = 0;
    uint8_t envelopePace = 0;
    bool envelopeIncrease = false;
public:
    void tick(int cycles) override;
    float getSample() const override;

    void clockLength();
    void clockEnvelope();
    void powerOff() override;
};


#endif //GAMECATEMU_PULSECHANNEL_H
