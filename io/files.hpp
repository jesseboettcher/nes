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
