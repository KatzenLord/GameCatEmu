//
// Created by katzenlord on 20.05.26.
//

#include "CPU.h"

#include <charconv>
#include <iomanip>
#include <iostream>

CPU::CPU(Bus &bus)
    : bus(bus) {
    initOpcodeTables();
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

    haltBug = false;

    interruptMasterEnable = false;
    imeEnableDelay = 0;

    joypadWakeUp = false;
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

    const uint8_t ie = read8(0xFFFF);
    const uint8_t interruptFlags = read8(0xFF0F);
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
    const OpcodeHandler handler = opcodeTable[opcode];
    if (handler == nullptr) {
        return unimplemented(opcode, oldPC);
    }

    return (this->*handler)(opcode);
}

void CPU::initOpcodeTables() {
    opcodeTable.fill(nullptr);
    cbOpcodeTable.fill(nullptr);

    for (uint16_t value = 0; value < 0x100; ++value) {
        const auto opcode = static_cast<uint8_t>(value);

        if ((opcode & 0xC7) == 0xC7) {
            opcodeTable[opcode] = &CPU::decodeRst;
        } else if ((opcode & 0xCF) == 0x09) {
            opcodeTable[opcode] = &CPU::decodeAddHLReg16;
        } else if ((opcode & 0xC0) == 0x40 && opcode != 0x76) {
            opcodeTable[opcode] = &CPU::decodeLdRegReg;
        } else if ((opcode & 0xCF) == 0x01) {
            opcodeTable[opcode] = &CPU::decodeLdReg16;
        } else if ((opcode & 0xCF) == 0x03) {
            opcodeTable[opcode] = &CPU::decodeIncReg16;
        } else if ((opcode & 0xC7) == 0x04) {
            opcodeTable[opcode] = &CPU::decodeIncReg8;
        } else if ((opcode & 0xC7) == 0x05) {
            opcodeTable[opcode] = &CPU::decodeDecReg8;
        } else if ((opcode & 0xC7) == 0x06) {
            opcodeTable[opcode] = &CPU::decodeRegImmediate;
        } else if ((opcode & 0xE7) == 0x20) {
            opcodeTable[opcode] = &CPU::decodeJrCondition;
        } else if ((opcode & 0xE7) == 0xC0) {
            opcodeTable[opcode] = &CPU::decodeRetCondition;
        } else if ((opcode & 0xE7) == 0xC2) {
            opcodeTable[opcode] = &CPU::decodeJpCondition;
        } else if ((opcode & 0xE7) == 0xC4) {
            opcodeTable[opcode] = &CPU::decodeCallCondition;
        } else if ((opcode & 0xCF) == 0x0B) {
            opcodeTable[opcode] = &CPU::decodeDecReg16;
        } else if ((opcode & 0xC0) == 0x80) {
            opcodeTable[opcode] = &CPU::decodeAluRegister;
        } else if ((opcode & 0xCF) == 0xC5) {
            opcodeTable[opcode] = &CPU::decodePushReg16;
        } else if ((opcode & 0xCF) == 0xC1) {
            opcodeTable[opcode] = &CPU::decodePopReg16;
        }
    }

    opcodeTable[0x00] = &CPU::invokeNoArg<&CPU::nop>;
    opcodeTable[0x02] = &CPU::invokeNoArg<&CPU::ld_bc_a>;
    opcodeTable[0x07] = &CPU::invokeNoArg<&CPU::rlca>;
    opcodeTable[0x08] = &CPU::invokeNoArg<&CPU::ld_a16_sp>;
    opcodeTable[0x0A] = &CPU::invokeNoArg<&CPU::ld_a_bc>;
    opcodeTable[0x0F] = &CPU::invokeNoArg<&CPU::rrca>;
    opcodeTable[0x10] = &CPU::invokeNoArg<&CPU::stop>;
    opcodeTable[0x12] = &CPU::invokeNoArg<&CPU::ld_de_a>;
    opcodeTable[0x17] = &CPU::invokeNoArg<&CPU::rla>;
    opcodeTable[0x18] = &CPU::invokeNoArg<&CPU::jr_i8>;
    opcodeTable[0x1A] = &CPU::invokeNoArg<&CPU::ld_a_de>;
    opcodeTable[0x1F] = &CPU::invokeNoArg<&CPU::rra>;
    opcodeTable[0x22] = &CPU::invokeNoArg<&CPU::ld_hli_a>;
    opcodeTable[0x27] = &CPU::invokeNoArg<&CPU::daa>;
    opcodeTable[0x2A] = &CPU::invokeNoArg<&CPU::ld_a_hli>;
    opcodeTable[0x2F] = &CPU::invokeNoArg<&CPU::cpl>;
    opcodeTable[0x32] = &CPU::invokeNoArg<&CPU::ld_hld_a>;
    opcodeTable[0x37] = &CPU::invokeNoArg<&CPU::scf>;
    opcodeTable[0x3A] = &CPU::invokeNoArg<&CPU::ld_a_hld>;
    opcodeTable[0x3F] = &CPU::invokeNoArg<&CPU::ccf>;
    opcodeTable[0x76] = &CPU::invokeNoArg<&CPU::halt>;
    opcodeTable[0xC3] = &CPU::invokeNoArg<&CPU::jp_u16>;
    opcodeTable[0xC6] = &CPU::invokeNoArg<&CPU::add_a_n8>;
    opcodeTable[0xC9] = &CPU::invokeNoArg<&CPU::ret>;
    opcodeTable[0xCB] = &CPU::invokeNoArg<&CPU::stepCB>;
    opcodeTable[0xCD] = &CPU::invokeNoArg<&CPU::call_u16>;
    opcodeTable[0xCE] = &CPU::invokeNoArg<&CPU::adc_a_n8>;
    opcodeTable[0xD6] = &CPU::invokeNoArg<&CPU::sub_a_n8>;
    opcodeTable[0xD9] = &CPU::invokeNoArg<&CPU::reti>;
    opcodeTable[0xDE] = &CPU::invokeNoArg<&CPU::sbc_a_n8>;
    opcodeTable[0xE0] = &CPU::invokeNoArg<&CPU::ldh_a8_a>;
    opcodeTable[0xE2] = &CPU::invokeNoArg<&CPU::ldh_c_a>;
    opcodeTable[0xE6] = &CPU::invokeNoArg<&CPU::and_a_n8>;
    opcodeTable[0xE8] = &CPU::invokeNoArg<&CPU::add_sp_e8>;
    opcodeTable[0xE9] = &CPU::invokeNoArg<&CPU::jp_hl>;
    opcodeTable[0xEA] = &CPU::invokeNoArg<&CPU::ld_a16_a>;
    opcodeTable[0xEE] = &CPU::invokeNoArg<&CPU::xor_a_n8>;
    opcodeTable[0xF0] = &CPU::invokeNoArg<&CPU::ldh_a_a8>;
    opcodeTable[0xF2] = &CPU::invokeNoArg<&CPU::ldh_a_c>;
    opcodeTable[0xF3] = &CPU::invokeNoArg<&CPU::di>;
    opcodeTable[0xF6] = &CPU::invokeNoArg<&CPU::or_a_n8>;
    opcodeTable[0xF8] = &CPU::invokeNoArg<&CPU::ld_hl_spe8>;
    opcodeTable[0xF9] = &CPU::invokeNoArg<&CPU::ld_sp_hl>;
    opcodeTable[0xFA] = &CPU::invokeNoArg<&CPU::ld_a_a16>;
    opcodeTable[0xFB] = &CPU::invokeNoArg<&CPU::ei>;
    opcodeTable[0xFE] = &CPU::invokeNoArg<&CPU::cp_u8>;
    
    for (uint16_t value = 0; value < 0x100; ++value) {
        const auto opcode = static_cast<uint8_t>(value);

        if ((opcode & 0xF8) == 0x00) {
            cbOpcodeTable[opcode] = &CPU::decodeCbRlc;
        } else if ((opcode & 0xF8) == 0x08) {
            cbOpcodeTable[opcode] = &CPU::decodeCbRrc;
        } else if ((opcode & 0xF8) == 0x10) {
            cbOpcodeTable[opcode] = &CPU::decodeCbRl;
        } else if ((opcode & 0xF8) == 0x18) {
            cbOpcodeTable[opcode] = &CPU::decodeCbRr;
        } else if ((opcode & 0xF8) == 0x20) {
            cbOpcodeTable[opcode] = &CPU::decodeCbSla;
        } else if ((opcode & 0xF8) == 0x28) {
            cbOpcodeTable[opcode] = &CPU::decodeCbSra;
        } else if ((opcode & 0xF8) == 0x30) {
            cbOpcodeTable[opcode] = &CPU::decodeCbSwap;
        } else if ((opcode & 0xF8) == 0x38) {
            cbOpcodeTable[opcode] = &CPU::decodeCbSrl;
        } else if ((opcode & 0xC0) == 0x40) {
            cbOpcodeTable[opcode] = &CPU::decodeCbBit;
        } else if ((opcode & 0xC0) == 0x80) {
            cbOpcodeTable[opcode] = &CPU::decodeCbRes;
        } else {
            cbOpcodeTable[opcode] = &CPU::decodeCbSet;
        }
    }
}

int CPU::decodeRst(uint8_t opcode) {
    return rst(opcode & 0x38);
}

uint8_t CPU::fetch8() {
    const uint8_t value = read8(PC);

    if (haltBug) {
        haltBug = false;
    } else {
        PC++;
    }

    return value;
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

int CPU::ldh_a_c() {
    const uint16_t addr = static_cast<uint16_t>(0xFF00u + C);
    const uint8_t value = read8(addr);
    A = value;

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

int CPU::ccf() {
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, !getFlag(Flag::C));
    return 4;
}

int CPU::scf() {
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, true);
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

int CPU::adc_a_n8() {
    const uint8_t value = fetch8();
    adc_a(value);
    return 8;
}

int CPU::sub_a_n8() {
    const uint8_t value = fetch8();
    sub_a(value);
    return 8;
}

int CPU::sbc_a_n8() {
    const uint8_t value = fetch8();
    sbc_a(value);
    return 8;
}

int CPU::or_a_n8() {
    const uint8_t value = fetch8();
    or_a(value);
    return 8;
}

int CPU::xor_a_n8() {
    const uint8_t value = fetch8();
    xor_a(value);
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

int CPU::rrca() {
    uint8_t value = A;

    const bool carry = value & 0x01;
    const uint8_t result = (value >> 1) | (carry ? 0x80 : 0);

    setFlag(Flag::Z, false);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, carry);
    A = result;

    return 4;
}

int CPU::rra() {
    uint8_t value = A;

    const bool oldCarry = getFlag(Flag::C);
    const bool newCarry = value & 0x01;

    const uint8_t result = (value >> 1) | (oldCarry ? 0x80 : 0);

    setFlag(Flag::Z, false);
    setFlag(Flag::N, false);
    setFlag(Flag::H, false);
    setFlag(Flag::C, newCarry);
    A = result;

    return 4;
}

int CPU::daa() {
    const bool nFlag = getFlag(Flag::N);
    const bool hFlag = getFlag(Flag::H);
    const bool cFlag = getFlag(Flag::C);

    uint8_t result = A;

    uint8_t adjustment = 0x00;

    if (nFlag) {
        if (hFlag) {
            adjustment += 0x6;
        }
        if (cFlag) {
            adjustment += 0x60;
        }
        result -= adjustment;

    } else {
        if (hFlag || (result & 0xF) > 0x9 ) {
            adjustment += 0x6;
        }
        if (cFlag || result > 0x99) {
            adjustment += 0x60;
            setFlag(Flag::C, true);
        }
        result += adjustment;
    }

    setFlag(Flag::Z, result == 0);
    setFlag(Flag::H, false);

    A = result;

    return 4;
}

int CPU::add_sp_e8() {
    const int8_t value = static_cast<int8_t>(fetch8());
    const uint16_t oldSP = SP;
    const uint16_t result = static_cast<uint16_t>(oldSP + value);

    setFlag(Flag::Z, false);
    setFlag(Flag::N, false);
    setFlag(Flag::H, ((oldSP & 0x0F) + (static_cast<uint8_t>(value) & 0x0F)) > 0x0F);
    setFlag(Flag::C, ((oldSP & 0xFF) + static_cast<uint8_t>(value)) > 0xFF);
    SP = result;

    return 16;
}

int CPU::ld_hl_spe8() {
    const int8_t value = static_cast<int8_t>(fetch8());
    const uint16_t oldSP = SP;
    const uint16_t result = static_cast<uint16_t>(oldSP + value);

    setFlag(Flag::Z, false);
    setFlag(Flag::N, false);
    setFlag(Flag::H, ((oldSP & 0x0F) + (static_cast<uint8_t>(value) & 0x0F)) > 0x0F);
    setFlag(Flag::C, ((oldSP & 0xFF) + static_cast<uint8_t>(value)) > 0xFF);
    setHL(result);

    return 12;
}

int CPU::ld_sp_hl() {
    SP = getHL();
    return 8;
}

int CPU::rst(uint16_t address) {
    push16(PC);
    PC = address;
    return 16;
}

void CPU::add_a(uint8_t value) {
    uint16_t result = static_cast<uint16_t>(A) + value;

    setFlag(Flag::Z, static_cast<uint8_t>(result) == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, ((A & 0x0F) + (value & 0x0F)) > 0x0F);
    setFlag(Flag::C, result > 0xFF);
    A = result;
}

void CPU::adc_a(uint8_t value) {
    uint8_t carry = getFlag(Flag::C) ? 1 : 0;
    uint16_t result = static_cast<uint16_t>(A) + value + carry;

    setFlag(Flag::Z, static_cast<uint8_t>(result) == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, ((A & 0x0F) + (value & 0x0F)+carry) > 0x0F);
    setFlag(Flag::C, result > 0xFF);
    A = static_cast<uint8_t>(result);
}

void CPU::sub_a(uint8_t value) {
    uint8_t result = static_cast<uint8_t>(A - value);

    setFlag(Flag::Z, result == 0);
    setFlag(Flag::N, true);
    setFlag(Flag::H, ((A & 0x0F)) < (value & 0x0F));
    setFlag(Flag::C, A < value);
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
    const uint8_t opcode = fetch8();
    const OpcodeHandler handler = cbOpcodeTable[opcode];

    if (handler == nullptr) {
        std::cerr << "Unimplemented CB Opcode 0x"
                  << std::hex << std::uppercase
                  << static_cast<int>(opcode)
                  << " \nTODO" << std::endl;
        halted = true;
        return 4;
    }

    return (this->*handler)(opcode);
}

int CPU::decodeCbRlc(uint8_t opcode) {
    return rlc_reg(opcode & 0x07);
}

int CPU::decodeCbRrc(uint8_t opcode) {
    return rrc_reg(opcode & 0x07);
}

int CPU::decodeCbRl(uint8_t opcode) {
    return rl_reg(opcode & 0x07);
}

int CPU::decodeCbRr(uint8_t opcode) {
    return rr_reg(opcode & 0x07);
}

int CPU::decodeCbSla(uint8_t opcode) {
    return sla_reg(opcode & 0x07);
}

int CPU::decodeCbSra(uint8_t opcode) {
    return sra_reg(opcode & 0x07);
}

int CPU::decodeCbSwap(uint8_t opcode) {
    return swap_reg(opcode & 0x07);
}

int CPU::decodeCbSrl(uint8_t opcode) {
    return srl_reg(opcode & 0x07);
}

int CPU::decodeCbBit(uint8_t opcode) {
    const uint8_t bit = (opcode >> 3) & 0x07;
    const uint8_t reg = opcode & 0x07;
    return bit_reg(bit, reg);
}

int CPU::decodeCbRes(uint8_t opcode) {
    const uint8_t bit = (opcode >> 3) & 0x07;
    const uint8_t reg = opcode & 0x07;
    return res_reg(bit, reg);
}

int CPU::decodeCbSet(uint8_t opcode) {
    const uint8_t bit = (opcode >> 3) & 0x07;
    const uint8_t reg = opcode & 0x07;
    return set_reg(bit, reg);
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
    bool carry = value & 0x01;

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
    bool newCarry = value & 0x01;

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

    value = (value >> 1) | msb;

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

    return reg == 6 ? 12 : 4;
}

int CPU::decodeIncReg8(uint8_t opcode) {
    const uint8_t reg = (opcode >> 3) & 0x07;

    const uint8_t oldVal = readReg8(reg);
    const auto newVal = static_cast<uint8_t>(oldVal + 1);

    writeReg8(reg, newVal);

    setFlag(Flag::Z, newVal == 0);
    setFlag(Flag::N, false);
    setFlag(Flag::H, (oldVal & 0x0F) == 0x0F);

    return reg == 6 ? 12 : 4;
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

int CPU::decodeCallCondition(uint8_t opcode) {
    uint16_t addr = fetch16();
    const uint8_t condition = (opcode >> 3) & 0x03;

    bool shouldCall = false;
    switch (condition) {
        case 0: shouldCall = !getFlag(Flag::Z); break;
        case 1: shouldCall = getFlag(Flag::Z); break;
        case 2: shouldCall = !getFlag(Flag::C); break;
        case 3: shouldCall = getFlag(Flag::C); break;
    }
    if (shouldCall) {
        push16(PC);
        PC = addr;
        return 24;
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

int CPU::handleInterrupts() {
    const uint8_t ie = read8(0xFFFF);
    const uint8_t interruptFlags = read8(0xFF0F);

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
    std::cerr << "Unimplemented or Unused Opcode 0x"
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
    const uint8_t pending =
        read8(0xFFFF) &
        read8(0xFF0F) &
        0x1F;

    if (!interruptMasterEnable && pending != 0) {
        haltBug = true;
    } else {
        halted = true;
    }
    return 4;
}

int CPU::stop() {
    fetch8();
    stopped = true;
    return 4;
}
