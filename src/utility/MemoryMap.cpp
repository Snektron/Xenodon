#include "utility/MemoryMap.h"
#include "core/Error.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

MemoryMap::MemoryMap(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        throw Error("Failed to open '{}'", path);
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) {
        close(fd);
        throw Error("Failed to stat '{}'", path);
    }

    this->size = static_cast<size_t>(statbuf.st_size);
    this->memory = static_cast<std::byte*>(mmap(0, this->size, PROT_READ, MAP_SHARED, fd, 0));
    close(fd);

    if (this->memory == MAP_FAILED) {
        throw Error("Failed to map '{}'", path);
    }
}

MemoryMap::~MemoryMap() {
    munmap(this->memory, this->size);
}
