#include "io/cartridge.hpp"

#include "lib/magic_enum.hpp"

#include <glog/logging.h>

class Cartridge_NROM : public Cartridge
{
protected:
    friend class Cartridge;
    Cartridge_NROM(std::shared_ptr<MappedFile> file, Format format, std::string_view name)
     : Cartridge(file, format, name) {}

    uint8_t read(uint16_t a) const override;
    void write(uint16_t a, uint8_t v) override;
    uint8_t ppu_read(uint16_t a) const override;

    void reset() override;

private:
    std::span<uint8_t> cpu_mapping_; // data mapped to 0x8000 - 0xBFFF
    std::span<uint8_t> ppu_mapping_; // data mapped to 0x0000 - 0x1FFF

    std::array<uint8_t, 0x1000> prg_ram_; // 4kb, mapper 0 @ 0x6000, mirrored to 0x8000
};

uint8_t Cartridge_NROM::read(uint16_t a) const
{
    assert(a >= 0x4020);

    if (a < 0x6000)
    {
        return 0;
    }

    if (a >= 0x6000 && a < 0x8000)
    {
        return prg_ram_[a % 0x1000];
    }

    if (a >= 0xC000 && cpu_mapping_.size() == 0x4000) // 16kb roms (0x4000) are mirrored at 0xC000
    {
        return cpu_mapping_[a - 0xC000];
    }

    return cpu_mapping_[a - 0x8000];
}

void Cartridge_NROM::write(uint16_t a, uint8_t v)
{
    if (a >= 0x6000 && a < 0x8000)
    {
        prg_ram_[a % 0x1000] = v;
    }
    assert(false);
}

uint8_t Cartridge_NROM::ppu_read(uint16_t a) const
{
    assert(a <= 0x2000);

    return ppu_mapping_[a];
}

void Cartridge_NROM::reset()
{
    if (mapper() == 0)
    {
        cpu_mapping_ = prg_rom(0, sizeof_prg_rom());

        if (chr_rom(0, sizeof_chr_rom()))
        {
            ppu_mapping_ = chr_rom(0, sizeof_chr_rom()).value();
        }
        return;
    }
    else if( mapper() == 1)
    {

    }
    assert(false);
}

class Cartridge_MMC1 : public Cartridge
{
protected:
    friend class Cartridge;
    Cartridge_MMC1(std::shared_ptr<MappedFile> file, Format format, std::string_view name)
     : Cartridge(file, format, name) {}

    uint8_t read(uint16_t a) const override;
    void write(uint16_t a, uint8_t v) override;
    uint8_t ppu_read(uint16_t a) const override;

    void reset() override;

private:
    int32_t load_write_count_{0};
    uint8_t load_register_{0};
    uint8_t control_register_{0};
    uint8_t chr_bank0_register_{0};
    uint8_t chr_bank1_register_{0};
    uint8_t pgr_bank_register_{0};

    std::span<uint8_t> chr_bank0_;
    std::span<uint8_t> chr_bank1_;

    std::span<uint8_t> prg_bank0_;
    std::span<uint8_t> prg_bank1_;

    std::array<uint8_t, 0x2000> chr_ram_;
    std::array<uint8_t, 0x1000> prg_ram_; // 4kb, mapper 0 @ 0x6000, mirrored to 0x8000
};

uint8_t Cartridge_MMC1::read(uint16_t a) const
{
    if (a < 0x6000)
    {
        return 0;
    }

    if (a >= 0x6000 && a < 0x8000)
    {
        return prg_ram_[a % 0x1000];
    }

    if (a >= 0x8000 && a < 0x8000 + prg_bank0_.size())
    {
        return prg_bank0_[a - 0x8000];
    }
    else if (a >= 0xC000)
    {
        return prg_bank1_[a - 0xC000];
    }
    return 0;
}

void Cartridge_MMC1::write(uint16_t a, uint8_t v)
{
    if (a < 0x2000)
    {
        assert(has_chr_ram());

        chr_ram_[a] = v;
    }

    if (a >= 0x8000 && a <= 0xFFFF)
    {
        if (v & 0x80) // clear shift register
        {
            load_register_ = 0;
            load_write_count_ = 0;
            return;
        }
        load_register_ = load_register_ >> 1;
        load_register_ |= (v & 0x01) << 4;
        load_write_count_++;

        if (load_write_count_ == 5)
        {
            LOG(INFO) << "mmc1 load register " << std::hex << +load_register_ << " address " << a;
            if (a < 0xA000) // control register
            {
                control_register_ = load_register_;
            }
            else if (a < 0xC000) // chr bank 0 register
            {
                if (!has_chr_ram())
                {
                    int32_t bank_size = control_register_ & 0x10 ? 0x1000 : 0x2000;
                    chr_bank0_register_ = load_register_;

                    chr_bank0_ = chr_rom(chr_bank0_register_ * bank_size, bank_size).value();
                }
            }
            else if (a < 0xE000) // chr bank 1 register
            {
                assert(!has_chr_ram());
                chr_bank1_register_ = load_register_;

                if (control_register_ & 0x10)
                {
                    int32_t bank_size = 0x1000;

                    chr_bank1_ = chr_rom(chr_bank1_register_ * bank_size, bank_size).value();
                }
                else
                {
                    chr_bank1_ = std::span<uint8_t>();
                }
            }
            else // prg bank register
            {
                pgr_bank_register_ = load_register_;

                switch ((control_register_ >> 2) & 0x03)
                {
                    case 0:
                    case 1:
                    {
                        int32_t bank_size = 0x8000;
                        prg_bank0_ = prg_rom(pgr_bank_register_ * bank_size, bank_size);
                        break;
                    }

                    case 3:
                    {
                        int32_t bank_size = 0x4000;
                        prg_bank0_ = prg_rom(pgr_bank_register_ * bank_size, bank_size);
                        break;
                    }

                    case 4:
                    {
                        int32_t bank_size = 0x4000;
                        prg_bank0_ = prg_rom(pgr_bank_register_ * bank_size, bank_size);
                        break;
                    }
                }
            }
            load_register_ = 0;
            load_write_count_ = 0;
        }
    }
}

uint8_t Cartridge_MMC1::ppu_read(uint16_t a) const
{
    assert(a <= 0x2000);

    if (a < chr_bank0_.size())
    {
        return chr_bank0_[a];
    }

    return chr_bank1_[a - 0x1000];
}

void Cartridge_MMC1::reset()
{
    uint32_t header_size = 16;
    uint32_t trainer_size = has_trainer() ? 512 : 0;
    uint32_t prg_rom_size = sizeof_prg_rom();
    prg_bank0_ = std::span<uint8_t>(&buffer_[header_size + trainer_size], 0x4000); // first 16kb of prg
    prg_bank1_ = std::span<uint8_t>(&buffer_[header_size + trainer_size + prg_rom_size - 0x4000], 0x4000); // last 16kb of prg

    uint32_t chr_rom_size = sizeof_chr_rom();

    if (chr_rom_size == 0)
    {
        assert(has_chr_ram());

        chr_bank0_ = std::span<uint8_t>(chr_ram_.data(), chr_ram_.size()); // 8kb
    }
    else
    {
        chr_bank0_ = std::span<uint8_t>(&buffer_[header_size + trainer_size + prg_rom_size], 0x2000); // 8kb
    }
}

Cartridge::Cartridge(std::shared_ptr<MappedFile> file, Format format, std::string_view name)
{
    file_ = file;
    format_ = format;
    name_ = name;

    buffer_ = file_->buffer();
}

bool Cartridge::valid() const
{
    return format_ == Format::iNES || format_ == Format::iNES2;
}

uint8_t Cartridge::mapper() const
{
    uint8_t mapper_number = 0;
    mapper_number |= buffer_[6] >> 4;
    mapper_number |= buffer_[7] & 0xF0;

    return mapper_number;
}

uint32_t Cartridge::sizeof_prg_rom() const
{
    // https://www.nesdev.org/wiki/NES_2.0#PRG-ROM_Area
    uint16_t rom_area = ((buffer_[9] & 0x0F) << 8) | buffer_[4];
    uint16_t size = 0;

    if ((rom_area & 0x800) == 0)
    {
        size = rom_area;
    }
    else // exponent form
    {
        uint16_t exponent = (rom_area >> 2) & 0x003F;
        uint16_t multiplier = (rom_area & 0x0003);

        size = (2 ^ exponent) * multiplier * 2 + 1;
    }

    return size * 16 * 1024;
}

uint32_t Cartridge::sizeof_chr_rom() const
{
    // https://www.nesdev.org/wiki/NES_2.0#PRG-ROM_Area
    uint16_t rom_area = ((buffer_[9] & 0xF0) << 4) | buffer_[5];
    uint16_t size = 0;

    if ((rom_area & 0x800) == 0)
    {
        size = rom_area;
    }
    else // exponent form
    {
        uint16_t exponent = (rom_area >> 2) & 0x003F;
        uint16_t multiplier = (rom_area & 0x0003);

        size = (2 ^ exponent) * multiplier * 2 + 1;
    }

    return size * 8 * 1024;
}

std::span<uint8_t> Cartridge::prg_rom(int32_t bank_offset, int32_t size) const
{
    uint32_t header_size = 16;
    uint32_t trainer_size = has_trainer() ? 512 : 0;
    return std::span<uint8_t>(&buffer_[header_size + trainer_size + bank_offset], size);
}

std::optional<std::span<uint8_t>> Cartridge::chr_rom(int32_t bank_offset, int32_t size) const
{
    uint32_t header_size = 16;
    uint32_t chr_rom_size = sizeof_chr_rom();
    uint32_t prg_rom_size = sizeof_prg_rom();
    uint32_t trainer_size = has_trainer() ? 512 : 0;

    if (chr_rom_size == 0)
    {
        return std::nullopt;
    }
    return std::span<uint8_t>(&buffer_[header_size + trainer_size + prg_rom_size + bank_offset], size);
}

bool Cartridge::has_trainer() const
{
    return buffer_[6] & 0x04;
}

std::ostream& operator << (std::ostream& os, const Cartridge& f)
{
    os << std::hex << std::setfill('0') << std::endl << std::endl;
    os << "  Cartridge: " << f.name_ << "\n";
    os << "------------------------------\n";

    uint32_t sizeofprg = f.sizeof_prg_rom();

    os << "Format:\t" << magic_enum::enum_name<Cartridge::Format>(f.format_)  << std::endl;
    os << "prg-rom:\t0x" << sizeofprg << std::endl;
    os << "chr-rom:\t0x" << f.sizeof_chr_rom() << std::endl;
    os << "mapper:\t0x" << static_cast<int32_t>(f.mapper()) << std::endl;
    os << "battery:\t" << (f.has_battery() ? "yes" : "no") << std::endl;
    os << "trainer:\t" << (f.has_trainer() ? "yes" : "no") << std::endl;
    os << "nametable mirroring:\t" << (f.horizontal_nametable_mirroring() ? "horizontal" : "vertical") << std::endl;

    return os;
}

std::shared_ptr<Cartridge> Cartridge::create(std::filesystem::path path)
{
    std::shared_ptr<Cartridge> result_cartridge;
    std::shared_ptr<MappedFile> file = MappedFile::open(path.c_str());

    if (!file)
    {
        return result_cartridge;
    }

    std::span<uint8_t> buffer = file->buffer();
    std::string_view header_constant(reinterpret_cast<char*>(buffer.data()), 4);
    Format format = Format::Unknown;

    // note last character is not a space it is 0x1A, substitute / spacer
    if (header_constant.starts_with("NES"))
    {
        // iNES header
        if ((buffer[7] & 0xC0) == 0x80)
        {
            format = Format::iNES2;
        }
        else
        {
            format = Format::iNES;
        }
        // Layout
        // headers (16 bytes)
        // trainer (optional, 512 bytes)
        // prg-rom
        // chr-rom
        // misc-rom
    }

    if (format == Format::Unknown)
    {
        return result_cartridge;
    }

    uint8_t mapper_number = 0;
    mapper_number |= buffer[6] >> 4;
    mapper_number |= buffer[7] & 0xF0;

    switch(mapper_number)
    {
        case 0:
            result_cartridge.reset(new Cartridge_NROM(file, format, path.filename().string()));
            break;

        case 1:
            result_cartridge.reset(new Cartridge_MMC1(file, format, path.filename().string()));
            break;

        default:
            LOG(ERROR) << "Unsupported mapper (" << mapper_number << ")";
            return result_cartridge;
    }
    LOG(INFO) << *result_cartridge;

    return result_cartridge;
}
