//
// Created by katzenlord on 19.05.26.
//

#include "Cartridge.h"
#include <fstream>
#include <iostream>

bool Cartridge::loadFromFile(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);

    if (!file) {
        std::cerr << "Could not open ROM: " << filePath << std::endl;
        return false;
    }

    rom = std::vector<uint8_t>(
        std::istreambuf_iterator(file),
        std::istreambuf_iterator<char>()
    );

    if (rom.size() < 0x150) {
        std::cerr << "ROM too small: " << rom.size() << std::endl;
        rom.clear();
        return false;
    }

    parseHeader();

    return true;
}

void Cartridge::parseHeader() {
    header.title.clear();

    for (uint16_t addr = 0x0134; addr <= 0x0143; ++addr) {
        const uint8_t c = rom[addr];
        if ( c == 0x00) {
            break;
        }
        header.title += static_cast<char>(c);
    }

    header.cartridgeType = rom[0x0147];
    header.cartridgeTypeString = getCartridgeType(header.cartridgeType);
    header.romSize = rom[0x0148];
    header.romSizeString = getRomSizeString(header.romSize);
    header.ramSize = rom[0x0149];
    header.ramSizeString = getRamSizeString(header.ramSize);
    header.destinationCode = rom[0x014A];
    header.headerChecksum = rom[0x014D];

}

uint8_t Cartridge::read(uint8_t addr) const {
    if (addr < rom.size()) {
        return rom[addr];
    }

    std::cerr << "Cartridge read out of range: 0x" << std::hex << addr << std::dec << std::endl;

    return 0xFF;
}

void Cartridge::write(uint8_t addr, uint8_t data) {
    //TODO
    (void)addr;
    (void)data;
}

std::string Cartridge::getCartridgeType(const uint8_t type) {
    switch (type) {
        case 0x00:
            return "ROM ONLY";
        case 0x01:
            return "MBC1";
        case 0x02:
            return "MBC1+RAM";
        case 0x03:
            return "MBC1+RAM+BATTERY";
        case 0x05:
            return "MBC2";
        case 0x06:
            return "MBC2+BATTERY";
        case 0x08:
            return "ROM+RAM";
        case 0x09:
            return "ROM+RAM+BATTERY";
        case 0x0B:
            return "MMM01";
        case 0x0C:
            return "MMM01+RAM";
        case 0x0D:
            return "MMM01+RAM+BATTERY";
        case 0x0F:
            return "MBC3+TIMER+BATTERY";
        case 0x10:
            return "MBC3+TIMER+RAM+BATTERY";
        case 0x11:
            return "MBC3";
        case 0x12:
            return "MBC3+RAM";
        case 0x13:
            return "MBC3+RAM+BATTERY";
        case 0x19:
            return "MBC5";
        case 0x1A:
            return "MBC5+RAM";
        case 0x1B:
            return "MBC5+RAM+BATTERY";
        case 0x1C:
            return "MBC5+RUMBLE";
        case 0x1D:
            return "MBC5+RUMBLE+RAM";
        case 0x1E:
            return "MBC5+RUMBLE+RAM+BATTERY";
        case 0x20:
            return "MBC6";
        case 0x22:
            return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
        case 0xFC:
            return "POCKET CAMERA";
        case 0xFD:
            return "BANDAI TAMA5";
        case 0xFE:
            return "HuC3";
        case 0xFF:
            return "HuC1+RAM+BATTERY";
        default:
            return "INVALID TYPE";
    }
}

std::string Cartridge::getRomSizeString(const uint8_t type) {
    switch (type) {
        case 0x00:
            return "32 KiB";
        case 0x01:
            return "64 KiB";
        case 0x02:
            return "128 KiB";
        case 0x03:
            return "256 KiB";
        case 0x04:
            return "512 KiB";
        case 0x05:
            return "1 MiB";
        case 0x06:
            return "2 MiB";
        case 0x07:
            return "4 MiB";
        case 0x08:
            return "8 MiB";
        case 0x52:
            return "1.1 MiB";
        case 0x53:
            return "1.2 MiB";
        case 0x59:
            return "1.5 MiB";
        default:
            return "INVALID SIZE";
    }
}

std::string Cartridge::getRamSizeString(const uint8_t type) {
    switch (type) {
        case 0x00:
            return "NO RAM";
        case 0x01:
            return "UNUSED";
        case 0x02:
            return "8 KiB";
        case 0x03:
            return "32 KiB";
        case 0x04:
            return "128 KiB";
        case 0x05:
            return "64 KiB";
        default:
            return "INVALID SIZE";
    }
}
