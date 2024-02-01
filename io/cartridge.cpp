#include "io/cartridge.hpp"

#include "lib/magic_enum.hpp"

#include <glog/logging.h>

bool CartridgeInterface::load(Processor6502& processor, NesPPU& ppu, const Cartridge& parsed_file)
{
	if (!parsed_file.valid())
	{
		LOG(ERROR) << "Invalid file, skipping load";
		return false;
	}
	if (parsed_file.mapper() != 0)
	{
		LOG(ERROR) << "Unsupported cartidge mapper";
		return false;
	}

	std::copy(parsed_file.prg_rom().begin(), parsed_file.prg_rom().end(),
			  processor.memory().begin() + 0x8000);

	if (parsed_file.sizeof_prg_rom() == 0x4000) // 16kb
	{
		// cartridges with 16kb of PRG ROM have their data mirrored at 0x8000, roms with 32kb will
		// full the entire space
		std::copy(parsed_file.prg_rom().begin(), parsed_file.prg_rom().end(),
				  processor.memory().begin() + 0x8000 + 0x4000);
	}

	if (auto maybe_chr_rom = parsed_file.chr_rom())
	{
		std::copy(maybe_chr_rom->begin(), maybe_chr_rom->end(), ppu.memory().begin());
	}
	return true;
}

void CartridgeInterface::load(Processor6502& processor, NesPPU&, const MappedFile& generic_rom_file)
{
	auto dest_location = processor.memory().end() - generic_rom_file.buffer().size();
	std::copy(generic_rom_file.buffer().begin(), generic_rom_file.buffer().end(), dest_location);
}

Cartridge::Cartridge(std::filesystem::path path)
{
	file_ = MappedFile::open(path.c_str());

	if (file_)
	{
		buffer_ = file_->buffer();
	}
	name_ = path.filename().string();

	parse();
}

Cartridge::Cartridge(std::span<uint8_t> buffer)
: buffer_(buffer)
{
	parse();
}

bool Cartridge::parse()
{
	std::string_view header_constant(reinterpret_cast<char*>(buffer_.data()), 4);

	if (!buffer_.size())
	{
		return false;
	}

	// note last character is not a space it is 0x1A, substitute / spacer
	if (header_constant.starts_with("NES"))
	{
		// iNES header
		if ((buffer_[7] & 0xC0) == 0x80)
		{
			format_ = Format::iNES2;
		}
		else
		{
			format_ = Format::iNES;
		}
		// Layout
		// headers (16 bytes)
		// trainer (optional, 512 bytes)
		// prg-rom
		// chr-rom
		// misc-rom
	}
	LOG(INFO) << *this;
	return true;
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

std::span<uint8_t> Cartridge::prg_rom() const
{
	uint32_t header_size = 16;
	uint32_t trainer_size = has_trainer() ? 512 : 0;
	uint32_t prg_rom_size = sizeof_prg_rom();
	return std::span<uint8_t>(&buffer_[header_size + trainer_size], prg_rom_size);
}

std::optional<std::span<uint8_t>> Cartridge::chr_rom() const
{
	uint32_t header_size = 16;
	uint32_t chr_rom_size = sizeof_chr_rom();
	uint32_t prg_rom_size = sizeof_prg_rom();
	uint32_t trainer_size = has_trainer() ? 512 : 0;

	if (chr_rom_size == 0)
	{
		return std::nullopt;
	}
	return std::span<uint8_t>(&buffer_[header_size + trainer_size + prg_rom_size], chr_rom_size);
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
