//
// Created by katzenlord on 19.05.26.
//

#pragma once

#ifndef GAMECATEMU_CARTRIDGE_H
#define GAMECATEMU_CARTRIDGE_H
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "MapperType.h"
#include "MBC.h"

class MBC;
enum class MapperType : uint8_t;

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
struct CartridgeFeatures {
    MapperType mapper = MapperType::MBC0;

    bool hasRam = false;
    bool hasBattery = false;

    size_t romSizeBytes = 0;
    size_t ramSizeBytes = 0;
};

class ExternalRAM {
public:
    explicit ExternalRAM(size_t size);

    uint8_t read(size_t offset) const;
    void write(size_t offset, uint8_t value);
private:
    std::vector<uint8_t> data;
};

class Cartridge {
public:
    bool loadFromFile(const std::string& filePath);
    void parseHeader();
    bool parseFeatures();

    static std::string getCartridgeType(uint8_t type);
    static std::string getRomSizeString(uint8_t type);
    static std::string getRamSizeString(uint8_t type);

    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t data);

    const CartridgeHeader& getHeader() const {
        return header;
    }

    const CartridgeFeatures& getFeatures() const {
        return features;
    }

    bool isLoaded() const {
        return !rom.empty();
    }
private:
    MBC mbc;
    std::vector<uint8_t> rom;
    std::optional<ExternalRAM> externalRam;
    CartridgeHeader header;
    CartridgeFeatures features;

    bool detectMulticart() const;
};
#endif //GAMECATEMU_CARTRIDGE_H
