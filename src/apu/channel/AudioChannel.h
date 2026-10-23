//
// Created by katzenlord on 16.10.26.
//

#ifndef GAMECATEMU_AUDIOCHANNEL_H
#define GAMECATEMU_AUDIOCHANNEL_H

#pragma once
#include <array>
#include <cstdint>

constexpr float OUTPUT_VOLUME = 0.05f;

class AudioChannel {
public:
    virtual ~AudioChannel() = default;

    virtual void tick(int cycles) = 0;
    virtual float getSample() const = 0;

    virtual void writeRegister(int reg, uint8_t value) = 0;
    virtual uint8_t readRegister(int reg) const = 0;

    virtual void reset() = 0;

    bool getLeft() const {return leftEnabled;}
    void setLeft(bool val){leftEnabled = val;}

    bool getRight() const {return rightEnabled;}
    void setRight(bool val){rightEnabled = val;}

    bool getEnabled() const {return enabled;}
    void setEnabled(bool val){enabled = val;}

    bool getDac() const {return dacEnabled;}
    void setDac(bool val){dacEnabled = val;}

    virtual void powerOff() = 0;
protected:
    std::array<uint8_t, 5> regs{};

    bool enabled = false;
    bool dacEnabled = false;

    int timer = 0;

    bool lengthEnabled = false;
    uint16_t lengthCounter = 0;

    bool leftEnabled = false;
    bool rightEnabled = false;
};

#endif //GAMECATEMU_AUDIOCHANNEL_H
