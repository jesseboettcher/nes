#pragma once

#include "io/files.hpp"
#include "processor/processor_6502.hpp"
#include "processor/nes_ppu.hpp"

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

    Cartridge(std::filesystem::path path);
    Cartridge(std::span<uint8_t> buffer);

    // Returns true if successful
    bool parse();

    bool valid() const;

    Format format() const { return format_; }

    uint8_t mapper() const;

    uint32_t sizeof_prg_rom() const;
    uint32_t sizeof_chr_rom() const;

    std::span<uint8_t> prg_rom() const;
    std::optional<std::span<uint8_t>> chr_rom() const;

    friend std::ostream& operator << (std::ostream& os, const Cartridge &f);

private:
    bool has_trainer() const;
    bool has_battery() const { return buffer_[6] & 0x02; }
    bool horizontal_nametable_mirroring() const { return buffer_[6] & 0x01; }

    std::span<uint8_t> buffer_;
    std::shared_ptr<MappedFile> file_;

    Format format_{Format::Unknown};

    std::string name_;
};


class CartridgeInterface
{
public:
    // Copies the PRG ROM to the last 16kb of the processor's memory
    static bool load(Processor6502& processor, NesPPU& ppu, const Cartridge& parsed_file);

    // Copies the PRG ROM to the last 16kb of the processor's memory
    static void load(Processor6502& processor, NesPPU& ppu, const MappedFile& generic_rom_file);
};
