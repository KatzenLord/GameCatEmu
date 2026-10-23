//
// Created by katzenlord on 17.10.26.
//

#ifndef GAMECATEMU_WAVECHANNEL_H
#define GAMECATEMU_WAVECHANNEL_H
#include "../AudioChannel.h"


class WaveChannel : public AudioChannel {
public:
    void tick(int cycles) override;
    void reset() override;

    float getSample() const override;

    void writeRegister(int reg, uint8_t value, bool shouldExtraClock) override;
    uint8_t readRegister(int reg) const override;

    void writeWaveRam(uint16_t address, uint8_t value);
    uint8_t readWaveRam(uint16_t address) const;

    void clockLength();

    void powerOff() override;
protected:
    std::array<uint8_t, 16> waveRam{};

    uint8_t outputLevel = 0;
    uint16_t period = 0;
    int waveIndex = 0;

    uint8_t getWaveSample() const;
};

#endif //GAMECATEMU_WAVECHANNEL_H
