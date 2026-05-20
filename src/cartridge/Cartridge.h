//
// Created by katzenlord on 19.05.26.
//

#pragma once

#ifndef GAMECATEMU_CARTRIDGE_H
#define GAMECATEMU_CARTRIDGE_H
#include <cstdint>
#include <string>
#include <vector>

struct CartridgeHeader {
    std::string title;

    uint8_t cartridgeType = 0;
    std::string cartridgeTypeString;

    uint8_t romSize = 0;
    std::string romSizeString;

    uint8_t ramSize = 0;
    std::string ramSizeString;

    uint8_t destinationCode = 0;
    uint8_t headerChecksum = 0;
};

class Cartridge {
public:
    bool loadFromFile(const std::string& filePath);
    void parseHeader();

    static std::string getCartridgeType(uint8_t type);
    static std::string getRomSizeString(uint8_t type);
    static std::string getRamSizeString(uint8_t type);

    uint8_t read(uint16_t addr) const;
    void write(uint8_t addr, uint8_t data);

    const CartridgeHeader& getHeader() const {
        return header;
    }

    bool isLoaded() const {
        return !rom.empty();
    }
private:
    std::vector<uint8_t> rom;
    CartridgeHeader header;
};
#endif //GAMECATEMU_CARTRIDGE_H
