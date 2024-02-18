#pragma once

#include "io/files.hpp"

class Cartridge
{
    // Parser for the .nes file format type: https://www.nesdev.org/wiki/INES
public:
    enum class Format
    {
        Unknown,
        iNES,
        iNES2
    };

    // Instantiates a cartridge from the file. Mapper 0 is handeled directly by this class,
    // other mappers will be derived classes.
    static std::shared_ptr<Cartridge> create(std::filesystem::path path);

    bool valid() const;

    // Mapper subclasses implement these
    virtual uint8_t read(uint16_t a) const = 0;
    virtual uint8_t& write(uint16_t a) = 0;
    virtual uint8_t ppu_read(uint16_t a) const = 0;

    virtual void reset() = 0;

    bool horizontal_nametable_mirroring() const { return buffer_[6] & 0x01; }
    bool vertical_nametable_mirroring() const { return !horizontal_nametable_mirroring(); }

    friend std::ostream& operator << (std::ostream& os, const Cartridge &f);

protected:
    Cartridge(std::shared_ptr<MappedFile> file, Format format);

    Format format() const { return format_; }

    uint8_t mapper() const;

    uint32_t sizeof_prg_rom() const;
    uint32_t sizeof_chr_rom() const;

    std::span<uint8_t> prg_rom() const;
    std::optional<std::span<uint8_t>> chr_rom() const;

    bool has_trainer() const;
    bool has_battery() const { return buffer_[6] & 0x02; }

    std::span<uint8_t> buffer_;
    std::shared_ptr<MappedFile> file_;

    Format format_{Format::Unknown};

    std::string name_;
};
