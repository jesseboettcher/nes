#pragma once

#include "io/files.hpp"

class Cartridge
{
    // Parser for the .nes file format type: https://www.nesdev.org/wiki/INES
public:
    Cartridge(std::filesystem::path path);
    Cartridge(std::span<uint8_t> buffer);

    bool valid() const;

    uint8_t read(uint16_t a) const;
    uint8_t ppu_read(uint16_t a) const;

    void reset();

    friend std::ostream& operator << (std::ostream& os, const Cartridge &f);

private:
    enum class Format
    {
        Unknown,
        iNES,
        iNES2
    };

    // Returns true if successful
    bool parse();

    Format format() const { return format_; }

    uint8_t mapper() const;

    uint32_t sizeof_prg_rom() const;
    uint32_t sizeof_chr_rom() const;

    std::span<uint8_t> prg_rom() const;
    std::optional<std::span<uint8_t>> chr_rom() const;

    bool has_trainer() const;
    bool has_battery() const { return buffer_[6] & 0x02; }
    bool horizontal_nametable_mirroring() const { return buffer_[6] & 0x01; }

    std::span<uint8_t> cpu_mapping_; // data mapped to 0x8000 - 0xBFFF
    std::span<uint8_t> ppu_mapping_; // data mapped to 0x0000 - 0x1FFF

    std::span<uint8_t> buffer_;
    std::shared_ptr<MappedFile> file_;

    Format format_{Format::Unknown};

    std::string name_;
};
