//
// Created by katzenlord on 20.05.26.
//

#pragma once

#ifndef GAMECATEMU_CPU_H
#define GAMECATEMU_CPU_H
#include "../bus/Bus.h"

class CPU {
public:
    explicit CPU(Bus& bus);

    void reset();
    int step();

    bool isHalted() const {
        return halted;
    }
    bool isStopped() const {
        return stopped;
    }
private:
    enum class Flag : uint8_t {
        Z = 0x80,
        N = 0x40,
        H = 0x20,
        C = 0x10,
    };

    Bus& bus;

    uint8_t A = 0;
    uint8_t F = 0;
    uint8_t B = 0;
    uint8_t C = 0;
    uint8_t D = 0;
    uint8_t E = 0;
    uint8_t H = 0;
    uint8_t L = 0;

    uint16_t PC = 0;
    uint16_t SP = 0;

    bool halted = false;
    bool stopped = false;

    bool interruptMasterEnable;
    bool enableInterruptsNextInstruction;

    uint8_t fetch8();
    uint16_t fetch16();

    uint8_t read8(uint16_t address) const;
    void write8(uint16_t address, uint8_t value) const;

    uint16_t getAF() const;
    uint16_t getBC() const;
    uint16_t getDE() const;
    uint16_t getHL() const;

    void setAF(uint16_t value);
    void setBC(uint16_t value);
    void setDE(uint16_t value);
    void setHL(uint16_t value);

    bool getFlag(Flag flag) const;
    void setFlag(Flag flag, bool value);

    int stepCB();

    uint8_t readReg8(uint8_t code) const;
    void writeReg8(uint8_t code, uint8_t value);

    void push16(uint16_t value);
    uint16_t pop16();

    int decodeLdRegReg(uint8_t opcode);
    int decodeLdReg16(uint8_t opcode);
    int decodeDecReg8(uint8_t opcode);
    int decodeIncReg8(uint8_t opcode);
    int decodeDecReg16(uint8_t opcode);
    int decodeIncReg16(uint8_t opcode);
    int decodeRegImmediate(uint8_t opcode);
    int decodeAluRegister(uint8_t opcode);
    int decodeJrCondition(uint8_t opcode);
    int decodePushReg16(uint8_t opcode);
    int decodePopReg16(uint8_t opcode);
    

    int nop();
    int halt();
    int jr_i8();
    int jp_u16();
    int ld_hld_a();
    int ldh_a8_a();
    int ldh_a_a8();
    int ld_a16_a();
    int cp_u8();
    int call_u16();
    int ret();
    int di();

    int rst(uint16_t address);

    // ALU operations
    void add_a(uint8_t value);
    void adc_a(uint8_t value);
    void sub_a(uint8_t value);
    void sbc_a(uint8_t value);
    void and_a(uint8_t value);
    void xor_a(uint8_t value);
    void or_a(uint8_t value);
    void cp_a(uint8_t value);


    int unimplemented(uint8_t opcode, uint16_t oldPC);

    void printTrace(uint16_t oldPC, uint8_t opcode) const;
};

#endif //GAMECATEMU_CPU_H
