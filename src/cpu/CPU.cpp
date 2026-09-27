//
// Created by katzenlord on 20.05.26.
//

#include "CPU.h"

#include <charconv>
#include <iomanip>
#include <iostream>

CPU::CPU(Bus &bus)
    : bus(bus){
    reset();
}

struct TraceEntry {
    uint16_t pc;
    uint8_t opcode;
    uint16_t af;
    uint16_t bc;
    uint16_t de;
    uint16_t hl;
    uint16_t sp;
};

static std::array<TraceEntry, 32> traceHistory{};
static size_t traceIndex = 0;

void CPU::reset() {
    A = 0x01;
    F = 0xB0;
    B = 0x00;
    C = 0x13;
    D = 0x00;
    E = 0xD8;
    H = 0x01;
    L = 0x4D;

    SP = 0xFFFE;
    PC = 0x0100;

    halted = false;
    stopped = false;
}
int CPU::step() {
    if (stopped) {
        if (joypadWakeUp) {
            stopped = false;
            joypadWakeUp = false;
        } else {
            return 4;
        }
    }

    const uint8_t ie = bus.read8(0xFFFF);
    const uint8_t interruptFlags = bus.read8(0xFF0F);
    const uint8_t pending = ie & interruptFlags & 0x1F;

    if (halted) {
        if (pending != 0) {
            halted = false;
        } else {
            return 4;
        }
    }

    const int interruptCycles = handleInterrupts();
    if (interruptCycles > 0) {
        return interruptCycles;
    }

    const uint16_t oldPC = PC;
    const uint8_t opcode = fetch8();

    const int cycles = executeOpcodes(opcode, oldPC);

    updateImeDelay();

    return cycles;
}

int CPU::executeOpcodes(uint8_t opcode, uint16_t oldPC) {
    if ((opcode & 0xC7) == 0xC7) {
        return rst(opcode & 0x38, oldPC);
    }
    if ((opcode & 0xCF) == 0x09) {
        return decodeAddHLReg16(opcode);
    }
    if ((opcode & 0xC0) == 0x40 && opcode != 0x76)
        return decodeLdRegReg(opcode);

    if ((opcode & 0xCF) == 0x01)
        return decodeLdReg16(opcode);

    if ((opcode & 0xCF) == 0x03)
        return decodeIncReg16(opcode);

    if ((opcode & 0xC7) == 0x04)
        return decodeIncReg8(opcode);

    if ((opcode & 0xC7) == 0x05)
        return decodeDecReg8(opcode);

    if ((opcode & 0xC7) == 0x06)
        return decodeRegImmediate(opcode);

    if ((opcode & 0xE7) == 0x20)
        return decodeJrCondition(opcode);

    if ((opcode & 0xE7) == 0xC0)
        return decodeRetCondition(opcode);

    if ((opcode & 0xE7) == 0xC2)
        return decodeJpCondition(opcode);

    if ((opcode & 0xCF) == 0x0B)
        return decodeDecReg16(opcode);

    if ((opcode & 0xC0) == 0x80)
        return decodeAluRegister(opcode);

    if ((opcode & 0xCF) == 0xC5)
        return decodePushReg16(opcode);

    if ((opcode & 0xCF) == 0xC1)
        return decodePopReg16(opcode);

    switch (opcode) {
        case 0x00: return nop();
        case 0x02: return ld_bc_a();
        case 0x07: return rlca();
        case 0x08: return ld_a16_sp();
        case 0x0A: return ld_a_bc();
        case 0x10: return stop();
        case 0x12: return ld_de_a();
        case 0x17: return rla();
        case 0x18: return jr_i8();
        case 0x1A: return ld_a_de();
        case 0x22: return ld_hli_a();
        case 0x2A: return ld_a_hli();
        case 0x3A: return ld_a_hld();
        case 0x2F: return cpl();
        case 0x32: return ld_hld_a();
        case 0x76: return halt();
        case 0xC3: return jp_u16();
        case 0xC6: return add_a_n8();
        case 0xC9: return ret();
        case 0xCB: return stepCB();
        case 0xCD: return call_u16();
        case 0xD6: return sub_a_n8();
        case 0xD9: return reti();
        case 0xE0: return ldh_a8_a();
        case 0xE2: return ldh_c_a();
        case 0xE6: return and_a_n8();
        case 0xE9: return jp_hl();
        case 0xEA: return ld_a16_a();
        case 0xF0: return ldh_a_a8();
        case 0xF3: return di();
        case 0xF6: return or_a_n8();
        case 0xFA: return ld_a_a16();
        case 0xFB: return ei();
        case 0xFE: return cp_u8();
        default:
            return unimplemented(opcode, oldPC);
    }
}

uint8_t CPU::fetch8() {
    return bus.read8(PC++);
}

uint16_t CPU::fetch16() {
    const uint8_t low = fetch8();
    const uint8_t high = fetch8();

    return static_cast<int>(low) | (static_cast<int>(high) << 8);
}

uint8_t CPU::read8(uint16_t address) const {
    return bus.read8(address);
}

void CPU::write8(uint16_t address, uint8_t data) const {
    bus.write8(address, data);
}

void CPU::push16(uint16_t value) {
    SP--;
    write8(SP, static_cast<uint8_t>((value >> 8) & 0xFF));

    SP--;
    write8(SP, static_cast<uint8_t>(value & 0xFF));
}

uint16_t CPU::pop16() {
    const uint8_t low = read8(SP);
    SP++;

    const uint8_t high = read8(SP);
    SP++;

    return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

uint16_t CPU::getAF() const {
    return (static_cast<uint16_t>(A) << 8) | F;
}

uint16_t CPU::getBC() const {
    return (static_cast<uint16_t>(B) << 8) | C;
}

uint16_t CPU::getDE() const {
    return (static_cast<uint16_t>(D) << 8) | E;
}

uint16_t CPU::getHL() const {
    return (static_cast<uint16_t>(H) << 8) | L;
}

void CPU::setAF(uint16_t value) {
    A = static_cast<uint8_t>(value >> 8);

    F = static_cast<uint8_t>(value & 0xF0);
}

void CPU::setBC(uint16_t value) {
    B = static_cast<uint8_t>(value >> 8);
    C = static_cast<uint8_t>(value & 0x00FF);
}

void CPU::setDE(uint16_t value) {
    D = static_cast<uint8_t>(value >> 8);
    E = static_cast<uint8_t>(value & 0x00FF);
}

void CPU::setHL(uint16_t value) {
    H = static_cast<uint8_t>(value >> 8);
    L = static_cast<uint8_t>(value & 0x00FF);
}

int CPU::nop() {
    return 4;
}

int CPU::jr_i8() {
    int8_t offset = static_cast<int8_t>(fetch8());

    PC = static_cast<uint16_t>(PC + offset);
    return 12;
}

int CPU::jp_u16() {
    const uint16_t addr = fetch16();
    PC = addr;
    return 16;
}

int CPU::jp_hl() {
    PC = getHL();
    return 4;
}

int CPU::ld_bc_a() {
    const uint16_t addr = getBC();
    write8(addr, A);
    return 8;
}

int CPU::ld_de_a() {
    const uint16_t addr = getDE();
    write8(addr, A);
    return 8;
}

int CPU::ld_a_bc() {
    const uint16_t addr = getBC();
    A = read8(addr);
    return 8;
}

int CPU::ld_a_de() {
    const uint16_t addr = getDE();
    A = read8(addr);
    return 8;
}

int CPU::ld_a_hli() {
    const uint16_t addr = getHL();
    A = read8(addr);
    setHL(static_cast<uint16_t>(addr + 1));
    return 8;
}

int CPU::ld_a_hld() {
    const uint16_t addr = getHL();
    A = read8(addr);
    setHL(static_cast<uint16_t>(addr - 1));
    return 8;
}

int CPU::ld_hli_a() {
    const uint16_t addr = getHL();
    write8(addr, A);
    setHL(static_cast<uint16_t>(addr + 1));
    return 8;
}

int CPU::ld_hld_a() {
    const uint16_t addr = getHL();
    write8(addr, A);
    setHL(addr - 1);
    return 8;
}

int CPU::ldh_c_a() {
    const uint16_t addr = static_cast<uint16_t>(0xFF00u + C);
    write8(addr, A);

    return 8;
}

int CPU::ldh_a8_a() {
    const uint8_t offset = fetch8();
    const uint16_t addr = static_cast<uint16_t>(0xFF00u + offset);
    write8(addr, A);

    return 12;
}

int CPU::ldh_a_a8() {
    const uint8_t offset = fetch8();
    const uint16_t addr = static_cast<uint16_t>(0xFF00u + offset);
    A = read8(addr);

    return 12;
}

int CPU::ld_a16_a() {
    const uint16_t addr = fetch16();
    write8(addr, A);
    return 16;
}

int CPU::ld_a_a16() {
    const uint16_t addr = fetch16();
    A = read8(addr);

    return 16;
}

int CPU::ld_a16_sp() {
    const uint16_t addr = fetch16();
    const uint8_t valueLow  = static_cast<uint8_t>(SP & 0xFF);
    const uint8_t valueHigh = static_cast<uint8_t>((SP >> 8) & 0xFF);

    write8(addr, valueLow);
    write8(addr + 1, valueHigh);

    return 20;
}

int CPU::cp_u8() {
    uint8_t val = fetch8();
    setFlag(Flag::Z, A == val);
    setFlag(Flag::N, true);
    setFlag(Flag::H, (A & 0x0F) < (val & 0x0F));
    setFlag(Flag::C, A < val);
    return 8;
}

int CPU::call_u16() {
    uint16_t addr = fetch16();

    push16(PC);
    PC = addr;

    return 24;
}

int CPU::ret() {
    PC = pop16();
    return 16;
}

int CPU::reti() {
    const uint16_t returnPC = pop16();
    PC = returnPC;

    interruptMasterEnable = true;
    imeEnableDelay=0;

    return 16;
}

int CPU::di() {
    interruptMasterEnable=false;
    imeEnableDelay=0;
    return 4;
}

int CPU::ei() {
    imeEnableDelay = 2;
    return 4;
}

int CPU::cpl() {
    A = ~A;
    setFlag(Flag::N, true);
    setFlag(Flag::H, true);

    return 4;
}

int CPU::and_a_n8() {
    const uint8_t value = fetch8();
    and_a(value);
    return 8;
}

int CPU::add_a_n8() {
    const uint8_t value = fetch8();
    add_a(value);
    return 8;
}

int CPU::sub_a_n8() {
    const uint8_t value = fetch8();
    sub_a(value);
    return 8;
}

int CPU::or_a_n8() {
    const uint8_t value = fetch8();
    or_a(value);
    return 8;
}

int CPU::rlca() {
    const uint8_t value = A;
    const bool carry = value & 0x80;
    const uint8_t result = (value << 1) | (carry ? 1: 0);

    setFlag(Flag::Z, false);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);
    A = result;

    return 4;
}

int CPU::rla() {
    uint8_t value = A;

    const bool oldCarry = getFlag(Flag::C);
    const bool newCarry = value & 0x80;

    const uint8_t result = (value << 1) | (oldCarry ? 1: 0);

    setFlag(Flag::Z, false);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, newCarry);
    A = result;

    return 4;
}

int CPU::rst(uint16_t address, uint16_t oldPC) {
    std::cout << "RST from PC=0x"
          << std::hex << std::uppercase << oldPC
          << " to=0x" << address
          << " return=0x" << PC
          << " SP before=0x" << SP
          << std::dec << "\n";
    push16(PC);
    PC = address;
    std::cout << "RST SP after=0x"
          << std::hex << std::uppercase << SP
          << std::dec << "\n";
    return 16;
}

void CPU::add_a(uint8_t value) {
    uint16_t result = static_cast<uint16_t>(A) + value;

    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, ((A & 0x0F) + (value & 0x0F)) > 0x0F);
    setFlag(Flag::C, result > 0xFF);
    A = result;
}

void CPU::adc_a(uint8_t value) {
    uint8_t carry = getFlag(Flag::C) ? 1 : 0;
    uint16_t result = static_cast<uint16_t>(A) + value + carry;

    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, ((A & 0x0F) + (value & 0x0F)+carry) > 0x0F);
    setFlag(Flag::C, result > 0xFF);
    A = static_cast<uint8_t>(result);
}

void CPU::sub_a(uint8_t value) {
    uint8_t result = static_cast<uint8_t>(A - value);

    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, true);
    setFlag(Flag::H, ((A & 0x0F)) < (value & 0x0F));
    setFlag(Flag::C, result < value);
    A = result;
}

void CPU::sbc_a(uint8_t value) {
    uint8_t carry = getFlag(Flag::C) ? 1 : 0;

    uint16_t subtrahend = static_cast<uint16_t>(value) + carry;
    uint8_t result = static_cast<uint8_t>(A - subtrahend);

    setFlag(Flag::Z, result == 0);
    setFlag(Flag::N, true);
    setFlag(Flag::H, ((A & 0x0F) < ((value & 0x0F) + carry)));
    setFlag(Flag::C, static_cast<uint16_t>(A) < subtrahend);
    A = result;
}

void CPU::and_a(uint8_t value) {
    A &= value;

    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, true);
    setFlag(Flag::C, false);
}

void CPU::xor_a(uint8_t value) {
    A ^= value;
    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H,false);
    setFlag(Flag::C, false);
}

void CPU::or_a(uint8_t value) {
    A |= value;

    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, false);
}

void CPU::cp_a(uint8_t value) {
    uint8_t result = static_cast<uint8_t>(A - value);

    setFlag(Flag::Z, result == 0);
    setFlag(Flag::N, true);
    setFlag(Flag::H, (A & 0x0F) < (value & 0x0F));
    setFlag(Flag::C, A < value);
}

int CPU::stepCB() {
    uint8_t opcode = fetch8();

    if ((opcode & 0xF8) == 0x00) {
        const uint8_t reg = opcode & 0x07;
        return rlc_reg(reg);
    }

    if ((opcode & 0xF8) == 0x08) {
        const uint8_t reg = opcode & 0x07;
        return rrc_reg(reg);
    }

    if ((opcode & 0xF8) == 0x10) {
        const uint8_t reg = opcode & 0x07;
        return rl_reg(reg);
    }

    if ((opcode & 0xF8) == 0x18) {
        const uint8_t reg = opcode & 0x07;
        return rr_reg(reg);
    }

    if ((opcode & 0xF8) == 0x20) {
        const uint8_t reg = opcode & 0x07;
        return sla_reg(reg);
    }

    if ((opcode & 0xF8) == 0x28) {
        const uint8_t reg = opcode & 0x07;
        return sra_reg(reg);
    }

    if ((opcode & 0xF8) == 0x30) {
        const uint8_t reg = opcode & 0x07;
        return swap_reg(reg);
    }

    if ((opcode & 0xF8) == 0x38) {
        const uint8_t reg = opcode & 0x07;
        return srl_reg(reg);
    }

    if ((opcode & 0xC0) == 0x40) {
        const uint8_t bit = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;
        return bit_reg(bit, reg);
    }

    if ((opcode & 0xC0) == 0x80) {
        const uint8_t bit = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;
        return res_reg(bit, reg);
    }

    if ((opcode & 0xC0) == 0xC0) {
        const uint8_t bit = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;
        return set_reg(bit, reg);
    }

    std::cerr << "Unimplemented CB Opcode 0x"
        << std::hex << std::uppercase
        << static_cast<int>(opcode)
        << " \nTODO" << std::endl;
    halted = true;
    return 4;
}

int CPU::swap_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);
    value = static_cast<uint8_t>((value >> 4) | (value << 4));

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N,false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, false);

    return reg == 6 ? 16 : 8;
}

int CPU::rlc_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);

    bool carry = value & 0x80;

    value = (value << 1) | (carry ? 1 : 0);

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);

    return reg == 6 ? 16 : 8;
}

int CPU::rrc_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);
    bool carry = value & 0x80;

    value = (value >> 1) | (carry ? 0x80 : 0);

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);

    return reg == 6 ? 16 : 8;
}

int CPU::rl_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);

    bool oldCarry = getFlag(Flag::C);
    bool newCarry = value & 0x80;

    value = (value << 1) | (oldCarry ? 1 : 0);

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, newCarry);

    return reg == 6 ? 16 : 8;
}

int CPU::rr_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);

    bool oldCarry = getFlag(Flag::C);
    bool newCarry = value & 0x80;

    value = (value >> 1) | (oldCarry ? 0x80 : 0);

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, newCarry);

    return reg == 6 ? 16 : 8;
}

int CPU::sla_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);
    bool carry = value & 0x80;

    value <<= 1;

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);

    return reg == 6 ? 16 : 8;
}

int CPU::sra_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);
    bool carry = value & 0x01;
    uint8_t msb = value & 0x80;

    value = (value >> 8) | msb;

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);

    return reg == 6 ? 16 : 8;
}

int CPU::srl_reg(uint8_t reg) {
    uint8_t value = readReg8(reg);
    bool carry = value & 0x01;

    value >>= 1;

    writeReg8(reg, value);

    setFlag(Flag::Z, value == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);

    return reg == 6 ? 16 : 8;
}

int CPU::bit_reg(uint8_t bit, uint8_t reg) {
    const uint8_t value = readReg8(reg);
    const bool isZero = (value & (1u << bit)) == 0;

    setFlag(Flag::Z, isZero);
    setFlag(Flag::N, false);
    setFlag(Flag::H, true);

    return reg == 6 ? 16 : 8;
}

int CPU::res_reg(uint8_t bit, uint8_t reg) {
    uint8_t value = readReg8(reg);
    value &= static_cast<uint8_t>(~(1u << bit));
    writeReg8(reg, value);

    return reg == 6 ? 16 : 8;
}

int CPU::set_reg(uint8_t bit, uint8_t reg) {
    uint8_t value = readReg8(reg);
    value |= static_cast<uint8_t>(1u << bit);
    writeReg8(reg, value);

    return reg == 6 ? 16 : 8;
}

bool CPU::getFlag(Flag flag) const {
    return (F & static_cast<uint8_t>(flag)) != 0;
}

void CPU::setFlag(Flag flag, bool value) {
    const auto mask = static_cast<uint8_t>(flag);

    if (value) {
        F |= mask;
    }
    else {
        F &= static_cast<uint8_t>(~mask);
    }

    F &= 0xF0;
}

uint8_t CPU::readReg8(uint8_t code) const {
    switch (code) {
        case 0: return B;
        case 1: return C;
        case 2: return D;
        case 3: return E;
        case 4: return H;
        case 5: return L;
        case 6: return read8(getHL());
        case 7: return A;
        default:
            return 0xFF;
    }
}

void CPU::writeReg8(uint8_t code, uint8_t value) {
    switch (code) {
        case 0: B = value; break;
        case 1: C = value; break;
        case 2: D = value; break;
        case 3: E = value; break;
        case 4: H = value; break;
        case 5: L = value; break;
        case 6: write8(getHL(), value); break;
        case 7: A = value; break;
        default: break;
    }
}

int CPU::decodeLdRegReg(uint8_t opcode) {
    const uint8_t dest = (opcode >> 3) & 0x07;
    const uint8_t src = opcode & 0x07;

    const uint8_t value = readReg8(src);
    writeReg8(dest, value);

    if (dest == 6 || src == 6) {
        return 8;
    }
    return 4;
}

int CPU::decodeLdReg16(uint8_t opcode) {
    uint8_t regPair = (opcode >> 4) & 0x03;
    uint16_t value = fetch16();

    switch (regPair) {
        case 0: setBC(value); break;
        case 1: setDE(value); break;
        case 2: setHL(value); break;
        case 3: SP = value; break;
        default: break;
    }
    return 12;
}

int CPU::decodeDecReg8(uint8_t opcode) {
    const uint8_t reg = (opcode >> 3) & 0x07;

    const uint8_t oldVal = readReg8(reg);
    const auto newVal = static_cast<uint8_t>(oldVal - 1);

    writeReg8(reg, newVal);

    setFlag(Flag::Z, newVal == 0);
    setFlag(Flag::N, true);
    setFlag(Flag::H, (oldVal & 0x0F) == 0x00);

    return reg == 6 ? 8 : 4;
}

int CPU::decodeIncReg8(uint8_t opcode) {
    const uint8_t reg = (opcode >> 3) & 0x07;

    const uint8_t oldVal = readReg8(reg);
    const auto newVal = static_cast<uint8_t>(oldVal + 1);

    writeReg8(reg, newVal);

    setFlag(Flag::Z, newVal == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, (oldVal & 0x0F) == 0x0F);

    return reg == 6 ? 8 : 4;
}

int CPU::decodeIncReg16(uint8_t opcode) {
    switch ((opcode >> 4) & 0x03) {
        case 0: setBC(getBC() + 1); break;
        case 1: setDE(getDE() + 1); break;
        case 2: setHL(getHL() + 1); break;
        case 3: SP++; break;
        default: break;
    }
    return 8;
}

int CPU::decodeDecReg16(uint8_t opcode) {
    switch ((opcode >> 4) & 0x03) {
        case 0: setBC(getBC() - 1); break;
        case 1: setDE(getDE() - 1); break;
        case 2: setHL(getHL() - 1); break;
        case 3: SP--; break;
        default: break;
    }
    return 8;
}

int CPU::decodeJrCondition(uint8_t opcode) {
    int8_t offset = static_cast<int8_t>(fetch8());
    uint8_t condition = (opcode >> 3) & 0x03;
    bool shouldJump = false;

    switch (condition) {
        case 0: shouldJump = !getFlag(Flag::Z); break;
        case 1: shouldJump = getFlag(Flag::Z); break;
        case 2: shouldJump = !getFlag(Flag::C); break;
        case 3: shouldJump = getFlag(Flag::C); break;
    }
    if (shouldJump) {
        PC = static_cast<uint16_t>(PC + offset);
        return 12;
    }
    return 8;
}

int CPU::decodePushReg16(uint8_t opcode) {
    uint8_t regPair = (opcode >> 4) & 0x03;

    switch (regPair) {
        case 0: push16(getBC()); break;
        case 1: push16(getDE()); break;
        case 2: push16(getHL()); break;
        case 3: push16(getAF()); break;
    }

    return 16;
}

int CPU::decodePopReg16(uint8_t opcode) {
    uint8_t regPair = (opcode >> 4) & 0x03;
    uint16_t value = pop16();

    switch (regPair) {
        case 0: setBC(value); break;
        case 1: setDE(value); break;
        case 2: setHL(value); break;
        case 3: setAF(value); break;
    }

    return 12;
}

int CPU::decodeJpCondition(uint8_t opcode) {
    const uint16_t addr = fetch16();
    const uint8_t condition = (opcode >> 3) & 0x03;

    bool shouldJump = false;
    switch (condition) {
        case 0: shouldJump = !getFlag(Flag::Z); break;
        case 1: shouldJump = getFlag(Flag::Z); break;
        case 2: shouldJump = !getFlag(Flag::C); break;
        case 3: shouldJump = getFlag(Flag::C); break;
    }

    if (shouldJump) {
        PC = addr;
        return 16;
    }

    return 12;
}

int CPU::decodeRetCondition(uint8_t opcode) {
    const uint8_t condition = (opcode >> 3) & 0x03;

    bool shouldReturn = false;

    switch (condition) {
        case 0: shouldReturn = !getFlag(Flag::Z); break;
        case 1: shouldReturn = getFlag(Flag::Z); break;
        case 2: shouldReturn = !getFlag(Flag::C); break;
        case 3: shouldReturn = getFlag(Flag::C); break;
    }

    if (shouldReturn) {
        PC = pop16();
        return 20;
    }
    return 8;
}

int CPU::decodeAddHLReg16(uint8_t opcode) {
    const uint8_t reg = (opcode >> 4) & 0x03;

    uint16_t value;

    switch (reg) {
        case 0: value = getBC(); break;
        case 1: value = getDE(); break;
        case 2: value = getHL(); break;
        case 3: value = SP; break;
        default: return 0;
    }

    const uint16_t hl = getHL();

    const uint32_t res = static_cast<uint32_t>(hl) + value;

    setFlag(Flag::N, false);
    setFlag(Flag::H, ((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF);
    setFlag(Flag::C, res > 0xFFFF);

    setHL(static_cast<uint16_t>(res));

    return 8;
}

int CPU::decodeRegImmediate(uint8_t opcode) {
    uint8_t dest = (opcode >> 3) & 0x07;
    uint8_t value = fetch8();

    writeReg8(dest, value);

    if (dest == 6) {
        return 12; // LD (HL), d8
    }
    return 8;
}

int CPU::decodeAluRegister(uint8_t opcode) {
    uint8_t operation = (opcode >> 3) & 0x07;
    uint8_t reg = opcode & 0x07;

    uint8_t value = readReg8(reg);

    switch (operation) {
        case 0: add_a(value); break;
        case 1: adc_a(value); break;
        case 2: sub_a(value); break;
        case 3: sbc_a(value); break;
        case 4: and_a(value); break;
        case 5: xor_a(value); break;
        case 6: or_a(value); break;
        case 7: cp_a(value); break;
    }

    return reg == 6 ? 8 : 4;
}

int CPU::handleInterrupts() {
    const uint8_t ie = bus.read8(0xFFFF);
    const uint8_t interruptFlags = bus.read8(0xFF0F);

    const uint8_t pending = ie & interruptFlags & 0x1F;

    if (pending == 0) {
        return 0;
    }

    if (!interruptMasterEnable) {
        return 0;
    }

    interruptMasterEnable = false;

    uint16_t vector = 0;
    uint8_t mask = 0;

    if (pending & 0x01) {
        vector = 0x0040;
        mask = 0x01;
    } else if (pending & 0x02) {
        vector = 0x0048;
        mask = 0x02;
    } else if (pending & 0x04) {
        vector = 0x0050;
        mask = 0x04;
    } else if (pending & 0x08) {
        vector = 0x0058;
        mask = 0x08;
    } else if (pending & 0x10) {
        vector = 0x0060;
        mask = 0x10;
    }

    bus.write8(0xFF0F, interruptFlags & ~mask);

    push16(PC);
    PC = vector;

    return 20;
}

int CPU::unimplemented(uint8_t opcode, uint16_t oldPC) {
    std::cerr << "Unimplemented Opcode 0x"
        << std::uppercase << std::hex
        << std::setfill('0') << std::setw(2)
        << static_cast<int>(opcode)
        << " at PC=0x"
        << std::setw(4)
        << oldPC
        << std::endl;

    halted = true;
    return 4;
}

void CPU::printTrace(uint16_t oldPC, uint8_t opcode) const {
    std::cout << "PC=" << std::hex << std::uppercase
          << std::setw(4) << std::setfill('0') << oldPC
          << " OP=" << std::setw(2) << static_cast<int>(opcode)
          << " AF=" << std::setw(4) << getAF()
          << " BC=" << std::setw(4) << getBC()
          << " DE=" << std::setw(4) << getDE()
          << " HL=" << std::setw(4) << getHL()
          << " SP=" << std::setw(4) << SP
          << std::dec << "\n";
}

void CPU::updateImeDelay() {
    if (imeEnableDelay > 0) {
        imeEnableDelay--;

        if (imeEnableDelay == 0) {
            interruptMasterEnable = true;
        }
    }
}

int CPU::halt() {
    halted = true;
    return 4;
}

int CPU::stop() {
    fetch8();
    stopped = true;
    return 4;
}
