#pragma once

#include <filesystem>
#include <optional>
#include <span>

#include <unistd.h>

class FdGuard
{
public:
	FdGuard(int fd)
	{
		fd_ = fd;
	}
	FdGuard(FdGuard&& other)
	{
		fd_ = other.fd_;
		other.fd_ = -1;
	}
	FdGuard& operator = (FdGuard&& other)
	{
		fd_ = other.fd_;
		other.fd_ = -1;
		return *this;
	}
	FdGuard(const FdGuard&) = delete;

	FdGuard& operator = (int fd)
	{
		reset();
		fd_ = fd;
		return *this;
	}

	~FdGuard()
	{
		close();
	}
	int fd() { return fd_; }
	void reset() { close(); }

private:
	void close()
	{
		if (fd_ > 0)
		{
			::close(fd_);
		}
		fd_ = -1;
	}

	int fd_{-1};
};

class MappedFile
{
public:
	static std::shared_ptr<MappedFile> open(std::filesystem::path path);

	MappedFile();
	~MappedFile();

	std::span<uint8_t> buffer() const { return std::span<uint8_t>(buffer_, length_); }

private:
	int fd_{-1};

	size_t length_{0};
	uint8_t* buffer_{nullptr};
};

class NesFileParser
{
	// Parser for the .nes file format type: https://www.nesdev.org/wiki/INES
public:
	NesFileParser(std::filesystem::path path);
	NesFileParser(std::span<uint8_t> buffer);

	bool valid() const;

	uint8_t mapper() const;

	// PRG ROM is specified in terms of 16kb units
	uint8_t num_prg_rom_units() const { return buffer_[4]; }
	uint16_t sizeof_prg_rom() const{ return num_prg_rom_units() * 16 * 1024; }

	// PRG ROM is specified in terms of 8kb units
	uint8_t num_chr_rom_units() const{ return buffer_[5]; }
	uint16_t sizeof_chr_rom() const{ return num_chr_rom_units() * 8 * 1024; }

	std::span<uint8_t> prg_rom() const;
	std::optional<std::span<uint8_t>> chr_rom() const;

private:
	bool has_trainer() const;
	
	std::span<uint8_t> buffer_;
	std::shared_ptr<MappedFile> file_;
};
