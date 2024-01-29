#include "files.hpp"

#include "lib/magic_enum.hpp"

#include <glog/logging.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

std::shared_ptr<MappedFile> MappedFile::open(std::filesystem::path path)
{
	std::shared_ptr<MappedFile> file = std::make_shared<MappedFile>();

	file->fd_ = ::open(path.c_str(), O_RDONLY);
	if (file->fd_ < 0)
	{
		LOG(ERROR) << "Failed to open file " << path << ": " << strerror(errno);
		return nullptr;
	}

	struct stat file_info;
	if (fstat(file->fd_, &file_info) != 0)
	{
		LOG(ERROR) << "Failed to fstat file " << strerror(errno);
		return nullptr;
	}
	file->length_ = file_info.st_size;

	file->buffer_ = static_cast<uint8_t*>(mmap(nullptr, file->length_, PROT_READ,
											  MAP_FILE | MAP_PRIVATE, file->fd_, 0));
	if (file->buffer_ == MAP_FAILED)
	{
		LOG(ERROR) << "mmap failed " << MAP_FAILED << " " <<" " << strerror(errno);
		return nullptr;
	}

	return file;
}

MappedFile::MappedFile()
{
}

MappedFile::~MappedFile()
{
	if (buffer_)
	{
		munmap(buffer_, length_);
	}

	if (fd_ > 0)
	{
		::close(fd_);
	}
	fd_ = -1;
}

NesFileParser::NesFileParser(std::filesystem::path path)
{
	file_ = MappedFile::open(path.c_str());

	if (file_)
	{
		buffer_ = file_->buffer();
	}
	name_ = path.filename().string();

	parse();
}

NesFileParser::NesFileParser(std::span<uint8_t> buffer)
: buffer_(buffer)
{
	parse();
}

bool NesFileParser::parse()
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

bool NesFileParser::valid() const
{
	return format_ == Format::iNES || format_ == Format::iNES2;
}

uint8_t NesFileParser::mapper() const
{
	uint8_t mapper_number = 0;
    mapper_number |= buffer_[6] >> 4;
	mapper_number |= buffer_[7] & 0xF0;

	return mapper_number;
}

uint32_t NesFileParser::sizeof_prg_rom() const
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

uint32_t NesFileParser::sizeof_chr_rom() const
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

std::span<uint8_t> NesFileParser::prg_rom() const
{
	uint32_t header_size = 16;
	uint32_t trainer_size = has_trainer() ? 512 : 0;
	uint32_t prg_rom_size = sizeof_prg_rom();
	return std::span<uint8_t>(&buffer_[header_size + trainer_size], prg_rom_size);
}

std::optional<std::span<uint8_t>> NesFileParser::chr_rom() const
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

bool NesFileParser::has_trainer() const
{
	return buffer_[6] & 0x04;
}

std::ostream& operator << (std::ostream& os, const NesFileParser& f)
{
	os << std::hex << std::setfill('0') << std::endl << std::endl;
	os << "  Cartridge: " << f.name_ << "\n";
	os << "------------------------------\n";

	uint32_t sizeofprg = f.sizeof_prg_rom();

	os << "Format:\t" << magic_enum::enum_name<NesFileParser::Format>(f.format_)  << std::endl;
	os << "prg-rom:\t0x" << sizeofprg << std::endl;
	os << "chr-rom:\t0x" << f.sizeof_chr_rom() << std::endl;
	os << "mapper:\t0x" << static_cast<int32_t>(f.mapper()) << std::endl;
	os << "battery:\t" << (f.has_battery() ? "yes" : "no") << std::endl;
	os << "trainer:\t" << (f.has_trainer() ? "yes" : "no") << std::endl;
	os << "nametable mirroring:\t" << (f.horizontal_nametable_mirroring() ? "horizontal" : "vertical") << std::endl;

	return os;
}
