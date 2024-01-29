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
	enum class Format
	{
		Unknown,
		iNES,
		iNES2
	};

	NesFileParser(std::filesystem::path path);
	NesFileParser(std::span<uint8_t> buffer);

	// Returns true if successful
	bool parse();

	bool valid() const;

	Format format() const { return format_; }

	uint8_t mapper() const;

	uint32_t sizeof_prg_rom() const;
	uint32_t sizeof_chr_rom() const;

	std::span<uint8_t> prg_rom() const;
	std::optional<std::span<uint8_t>> chr_rom() const;

	friend std::ostream& operator << (std::ostream& os, const NesFileParser &f);

private:
	bool has_trainer() const;
	bool has_battery() const { return buffer_[6] & 0x02; }
	bool horizontal_nametable_mirroring() const { return buffer_[6] & 0x01; }
	
	std::span<uint8_t> buffer_;
	std::shared_ptr<MappedFile> file_;

	Format format_{Format::Unknown};

	std::string name_;
};
