#include "files.hpp"

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
}

NesFileParser::NesFileParser(std::span<uint8_t> buffer)
: buffer_(buffer)
{

}

bool NesFileParser::valid() const
{
	std::string_view header_constant(reinterpret_cast<char*>(buffer_.data()), 4);

	// note last character is not a space it is 0x1A, substitute / spacer
	return buffer_.size() && header_constant.starts_with("NES");
}

uint8_t NesFileParser::mapper() const
{
	uint8_t mapper_number = 0;
	mapper_number |= buffer_[6] >> 8;
	mapper_number |= buffer_[7] & 0xF0;

	return mapper_number;
}

std::span<uint8_t> NesFileParser::prg_rom() const
{
	uint32_t header_size = 16;
	uint32_t trainer_size = has_trainer() ? 512 : 0;
	uint32_t prg_rom_size = 16 * 1024 * buffer_[4];
	return std::span<uint8_t>(&buffer_[header_size + trainer_size], prg_rom_size);
}

std::optional<std::span<uint8_t>> NesFileParser::chr_rom() const
{
	uint32_t header_size = 16;
	uint32_t chr_rom_size = 8 * 1024 * buffer_[5];
	uint32_t prg_rom_size = 16 * 1024 * buffer_[4];
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
