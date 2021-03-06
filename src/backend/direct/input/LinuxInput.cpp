#include "backend/direct/input/LinuxInput.h"
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include "core/Error.h"
#include "backend/direct/input/linux_translate_key.h"

FileDescriptor::FileDescriptor(const char* path):
    fd(open(path, O_RDONLY)) {

    if (this->fd == -1) {
        throw Error("Failed to open file descriptor '{}'", path);
    }
}

FileDescriptor::FileDescriptor(FileDescriptor&& other):
    fd(other.fd) {
    other.fd = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) {
    std::swap(this->fd, other.fd);
    return *this;
}

FileDescriptor::~FileDescriptor() {
    if (this->fd != -1)
        close(this->fd);
}

LinuxInput::LinuxInput(EventDispatcher& dispatcher, const DirectConfig::Input& cfg):
    dispatcher(&dispatcher),
    kbd_fd(cfg.kbd_dev.c_str()) {
    ioctl(this->kbd_fd.fd, EVIOCGRAB, 1);
}

void LinuxInput::poll_events() {
    struct pollfd fd_info {
        this->kbd_fd.fd,
        POLLIN,
        0
    };

    while (true) {
        int nready = poll(&fd_info, 1, 0);
        if (nready == 0)
            break;

        struct input_event buff[16];

        ssize_t n = read(this->kbd_fd.fd, &buff, sizeof buff);
        if (n < ssize_t{0}) {
            throw Error("Error reading device data: {} (code {})", strerror(errno), errno);
        }

        size_t m = static_cast<size_t>(n) / sizeof(struct input_event);
        for (size_t i = 0; i < m; ++i) {
            this->handle_event(buff[i]);
        }
    }
}

void LinuxInput::handle_event(struct input_event& event) {
    if (event.type == EV_KEY) {
        auto action = Action::Press;
        if (event.value == 0)
            action = Action::Release;

        Key key = linux_translate_key(event.code);
        this->dispatcher->dispatch_key_event(key, action);
    }
}
