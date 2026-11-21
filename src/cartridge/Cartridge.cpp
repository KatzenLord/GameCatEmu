//
// Created by katzenlord on 19.05.26.
//

#include "Cartridge.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "MapperType.h"

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

    std::cout << "ROM Loaded" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Title: " << header.title << std::endl;
    std::cout << "Cartridge Type: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(header.cartridgeType) << std::dec << " -> " << header.cartridgeTypeString << std::endl;
    std::cout << "ROM Size: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(header.romSize) << std::dec << " -> " << header.romSizeString << std::endl;
    std::cout << "RAM Size: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(header.ramSize) << std::dec << " -> " << header.ramSizeString << std::endl;

    if (!parseFeatures()) {
        rom.clear();
        return false;
    }

    if (features.hasRam) {
        externalRam.emplace(features.ramSizeBytes);
    }
    const bool multicart = features.mapper == MapperType::MBC1 && detectMulticart();
    mbc.configure(features.mapper, features.romSizeBytes, features.ramSizeBytes, multicart);

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

bool Cartridge::parseFeatures() {
    features = {};

    switch (header.cartridgeType) {
        case 0x00: features.mapper = MapperType::MBC0; break;
        case 0x08:
        case 0x09:
            features.mapper = MapperType::MBC0;
            features.hasRam = true;
            features.hasBattery = header.cartridgeType == 0x09;
            break;

        case 0x01:
        case 0x02:
        case 0x03:
            features.mapper = MapperType::MBC1;
            features.hasRam = header.cartridgeType != 0x01;
            features.hasBattery = header.cartridgeType == 0x03;
            break;

        default:
            std::cerr << "Unsupported cartridge Type: 0x"
                        << std::hex
                        << static_cast<int>(header.cartridgeType)
                        << std::dec << "\n";
            return false;
    }

    switch (header.romSize) {
        case 0x00: features.romSizeBytes = 32 * 1024; break;
        case 0x01: features.romSizeBytes = 64 * 1024; break;
        case 0x02: features.romSizeBytes = 128 * 1024; break;
        case 0x03: features.romSizeBytes = 256 * 1024; break;
        case 0x04: features.romSizeBytes = 512 * 1024; break;
        case 0x05: features.romSizeBytes = 1024 * 1024; break;
        case 0x06: features.romSizeBytes = 2 * 1024 * 1024; break;
        case 0x07: features.romSizeBytes = 4 * 1024 * 1024; break;
        case 0x08: features.romSizeBytes = 8 * 1024 * 1024; break;

        case 0x52: features.romSizeBytes = 72 * 16 * 1024; break;
        case 0x53: features.romSizeBytes = 80 * 16 * 1024; break;
        case 0x54: features.romSizeBytes = 96 * 16 * 1024; break;

        default:
            std::cerr << "Unsupported ROM size code\n";
            return false;
    }

    if (features.hasRam) {
        switch (header.ramSize) {
            case 0x00: features.ramSizeBytes = 0; break;
            case 0x02: features.ramSizeBytes = 8 * 1024; break;
            case 0x03: features.ramSizeBytes = 32 * 1024; break;
            case 0x04: features.ramSizeBytes = 128 * 1024; break;
            case 0x05: features.ramSizeBytes = 64 * 1024; break;

            default:
                std::cerr << "Unsupported RAM size code\n";
                return false;
        }

        if (features.ramSizeBytes == 0) {
            std::cerr << "Cartridge requires RAM but header specifies none\n";
            return false;
        }
    }

    if (rom.size() != features.romSizeBytes) {
        std::cerr << "ROM size does not match cartridge header\n";
        return false;
    }

    return true;
}

uint8_t Cartridge::read(uint16_t addr) const {
    if (addr <= 0x7FFF) {
        const size_t offset = mbc.mapRomAddress(addr);
        if (offset < rom.size()) {
            return rom[offset];
        }
    }

    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!externalRam || !mbc.isRamEnabled()) {
            return 0xFF;
        }
        return externalRam->read(mbc.mapRamAddress(addr));
    }

    std::cerr << "Cartridge read out of range: 0x" << std::hex << addr << std::dec << std::endl;

    return 0xFF;
}

void Cartridge::write(uint16_t addr, uint8_t data) {
    if (addr <= 0x7FFF) {
        mbc.write(addr, data);
        return;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!externalRam || !mbc.isRamEnabled()) {
            return;
        }
        externalRam->write(mbc.mapRamAddress(addr), data);
    }
}

ExternalRAM::ExternalRAM(size_t size) : data(size, 0xFF) {
}

uint8_t ExternalRAM::read(size_t offset) const {
    if (offset >= data.size()) {
        return 0xFF;
    }
    return data[offset];
}

void ExternalRAM::write(size_t offset, uint8_t value) {
    if (offset >= data.size()) {
        return;
    }
    data[offset] = value;
}

bool Cartridge::detectMulticart() const {
    if (rom.size() != 0x100000) {
        return false;
    }

    constexpr size_t LOGO_OFFSET = 0x0104;
    constexpr size_t LOGO_SIZE = 48;
    constexpr size_t GAME_SIZE = 0x40000;

    int validLogos = 0;

    for (size_t i = 0; i < 4; i++) {
        const size_t offset = i * GAME_SIZE + LOGO_OFFSET;

        const bool matches = std::equal(
            rom.begin() + LOGO_OFFSET,
            rom.begin() + LOGO_OFFSET + LOGO_SIZE,
            rom.begin() + offset
        );

        if (matches) {
            ++validLogos;
        }
    }
    return validLogos >= 2;
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
