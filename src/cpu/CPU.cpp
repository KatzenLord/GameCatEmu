//
// Created by katzenlord on 20.05.26.
//

#include "CPU.h"

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
    uint16_t oldPC = PC;
    uint8_t opcode = fetch8();

    if ((opcode & 0xCF) == 0x01) {
        return decodeLdReg16(opcode);
    }

    if ((opcode & 0xC0) == 0x40 && opcode != 0x76)
        return decodeLdRegReg(opcode);

    if ((opcode & 0xC7) == 0x06)
        return decodeRegImmediate(opcode);

    if ((opcode & 0xF8) == 0xA8)
        return xor_a_reg(opcode);


    switch (opcode) {
        case 0x00: return nop();
        case 0x20: return jp_nz_i8();
        case 0x32: return ld_hld_a();
        case 0xC3: return jp_u16();
        case 0xCB: return stepCB();
        case 0xE0: return ldh_a8_a();
        case 0xEA: return ld_a16_a();
        case 0xF0: return ldh_a_a8();
        case 0xFE: return cp_u8();
        default:
            return unimplemented(opcode, oldPC);
    }
}

uint8_t CPU::fetch8() {
    return bus.read8(PC++);
}

uint16_t CPU::fetch16() {
    uint8_t low = fetch8();
    uint8_t high = fetch8();

    return static_cast<int>(low) | (static_cast<int>(high) << 8);
}

uint8_t CPU::read8(uint16_t address) const {
    return bus.read8(address);
}

void CPU::write8(uint16_t address, uint8_t data) const {
    bus.write8(address, data);
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

int CPU::jp_u16() {
    uint16_t addr = fetch16();
    PC = addr;
    return 16;
}

int CPU::ld_hld_a() {
    uint16_t addr = getHL();
    write8(addr, A);
    setHL(addr - 1);
    return 8;
}

int CPU::ldh_a8_a() {
    uint8_t offset = fetch8();
    write8(0xFF00 - offset, A);
    return 12;
}

int CPU::ldh_a_a8() {
    uint8_t offset = fetch8();
    A = read8(0xFF00 - offset);
    return 12;
}

int CPU::ld_a16_a() {
    uint16_t addr = fetch16();
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

int CPU::jp_nz_i8() {
    int8_t offset = static_cast<int8_t>(fetch8());

    if (!getFlag(Flag::Z)) {
        PC = static_cast<uint8_t>(PC + offset);
        return 12;
    }
    return 8;
}

int CPU::xor_a_reg(uint8_t opcode) {
    uint8_t reg = opcode & 0x07;
    A ^= readReg8(reg);
    setFlag(Flag::Z, A == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H,false);
    setFlag(Flag::C, false);

    return reg == 6 ? 8 : 4;
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
    const uint8_t mask = static_cast<uint8_t>(flag);

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
    uint8_t dest = (opcode >> 3) & 0x07;
    uint8_t src = opcode & 0x07;

    uint8_t value = readReg8(dest);
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
        default: return 0xFF;
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
    std::cout << "alu not implemented" << std::endl;
    return 4;
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

int CPU::halt() {
    halted = true;
    return 4;
}
