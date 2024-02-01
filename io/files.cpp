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
