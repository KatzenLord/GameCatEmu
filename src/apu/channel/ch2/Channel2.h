//
// Created by katzenlord on 16.10.26.
//

#ifndef GAMECATEMU_CHANNEL2_H
#define GAMECATEMU_CHANNEL2_H
#include "../pulse/PulseChannel.h"


class Channel2 : public PulseChannel {
public:
    void reset() override;
    void writeRegister(int reg, uint8_t value, bool shouldExtraClock) override;
    uint8_t readRegister(int reg) const override;
};


#endif //GAMECATEMU_CHANNEL2_H
