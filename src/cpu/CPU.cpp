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
    if (halted) {
        return 4;
    }
    const uint16_t oldPC = PC;
    const uint8_t opcode = fetch8();

    if (PC < 0x0214 || PC > 0x0219) {
        printTrace(oldPC, opcode);
    }

    if ((opcode & 0xC7) == 0xC7) {
        uint16_t addr = opcode & 0x38;
        return rst(addr);
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
        case 0x18: return jr_i8();
        case 0x2A: return ld_a_hli();
        case 0x32: return ld_hld_a();
        case 0xC3: return jp_u16();
        case 0xC9: return ret();
        case 0xCB: return stepCB();
        case 0xCD: return call_u16();
        case 0xE0: return ldh_a8_a();
        case 0xEA: return ld_a16_a();
        case 0xF0: return ldh_a_a8();
        case 0xF3: return di();
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

    PC = static_cast<uint8_t>(PC + offset);
    return 12;
}

int CPU::jp_u16() {
    const uint16_t addr = fetch16();
    PC = addr;
    return 16;
}

int CPU::ld_a_hli() {
    const uint16_t addr = getHL();
    A = read8(addr);
    setHL(addr+1);
    return 8;
}

int CPU::ld_hld_a() {
    const uint16_t addr = getHL();
    write8(addr, A);
    setHL(addr - 1);
    return 8;
}

int CPU::ldh_a8_a() {
    const uint8_t offset = fetch8();
    const uint16_t addr = static_cast<uint16_t>(0xFF00u + offset);

    write8(addr, A);

    std::cout << std::hex << std::uppercase << std::setfill('0')
              << "LDH ($" << std::setw(2) << static_cast<int>(offset)
              << "),A -> [0x" << std::setw(4) << addr
              << "] = 0x" << std::setw(2) << static_cast<int>(A)
              << std::dec << "\n";

    return 12;
}

int CPU::ldh_a_a8() {
    const uint8_t offset = fetch8();
    const uint16_t addr = static_cast<uint16_t>(0xFF00u + offset);
    A = read8(addr);

    std::cout << std::hex << std::uppercase << std::setfill('0')
              << "LDH A,($" << std::setw(2) << static_cast<int>(offset)
              << ") -> [0x" << std::setw(4) << addr
              << "] = 0x" << std::setw(2) << static_cast<int>(A)
              << std::dec << "\n";

    return 12;
}

int CPU::ld_a16_a() {
    const uint16_t addr = fetch16();
    write8(addr, A);
    return 16;
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

int CPU::di() {
    interruptMasterEnable=false;
    enableInterruptsNextInstruction=false;
    return 4;
}

int CPU::rst(uint16_t address) {
    push16(address);
    PC = address;
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

    std::cerr << "Unimplemented CB Opcode 0x"
        << std::hex << std::uppercase
        << static_cast<int>(opcode)
        << " \nTODO" << std::endl;

    return 8;
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

    const uint8_t value = readReg8(dest);
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
    setFlag(Flag::N, true);
    setFlag(Flag::H, (oldVal & 0x0F) == 0x00);

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

int CPU::halt() {
    halted = true;
    return 4;
}
