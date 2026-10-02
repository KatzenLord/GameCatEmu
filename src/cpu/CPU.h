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

    void wakeUpJoyPad() {
        joypadWakeUp = true;
    }

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

    bool joypadWakeUp = false;

    bool interruptMasterEnable;
    ushort imeEnableDelay;

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
    int decodeJpCondition(uint8_t opcode);
    int decodeRetCondition(uint8_t opcode);
    int decodeAddHLReg16(uint8_t opcode);
    int decodeCallCondition(uint8_t opcode);

    // normal instructions
    int nop();
    int halt();
    int stop();
    int jr_i8();
    int jp_u16();
    int jp_hl();
    int ld_bc_a();
    int ld_de_a();
    int ld_a_bc();
    int ld_a_de();
    int ld_a_hli();
    int ld_a_hld();
    int ld_hli_a();
    int ld_hld_a();
    int ldh_c_a();
    int ldh_a_c();
    int ldh_a8_a();
    int ldh_a_a8();
    int ld_a16_a();
    int ld_a_a16();
    int ld_a16_sp();
    int cp_u8();
    int call_u16();
    int ret();
    int reti();
    int di();
    int ei();
    int cpl();
    int ccf();
    int scf();
    int and_a_n8();
    int add_a_n8();
    int adc_a_n8();
    int sub_a_n8();
    int sbc_a_n8();
    int or_a_n8();
    int xor_a_n8();
    int rlca();
    int rla();
    int rrca();
    int rra();
    int daa();
    int add_sp_e8();
    int ld_hl_spe8();
    int ld_sp_hl();

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

    // CB prefixed instructions

    int swap_reg(uint8_t reg);
    int rlc_reg(uint8_t reg);
    int rrc_reg(uint8_t reg);
    int rl_reg(uint8_t reg);
    int rr_reg(uint8_t reg);
    int sla_reg(uint8_t reg);
    int sra_reg(uint8_t reg);
    int srl_reg(uint8_t reg);
    int bit_reg(uint8_t bit, uint8_t reg);
    int res_reg(uint8_t bit, uint8_t reg);
    int set_reg(uint8_t bit, uint8_t reg);

    // utils
    int handleInterrupts();
    int unimplemented(uint8_t opcode, uint16_t oldPC);
    void printTrace(uint16_t oldPC, uint8_t opcode) const;
    int executeOpcodes(uint8_t opcode, uint16_t oldPC);
    void updateImeDelay();
};

#endif //GAMECATEMU_CPU_H
