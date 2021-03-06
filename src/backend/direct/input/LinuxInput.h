#ifndef _XENODON_BACKEND_DIRECT_INPUT_LINUXINPUT_H
#define _XENODON_BACKEND_DIRECT_INPUT_LINUXINPUT_H

#include "backend/Event.h"
#include "backend/direct/DirectConfig.h"

struct FileDescriptor {
    int fd;

    FileDescriptor(const char* path);

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other);
    FileDescriptor& operator=(FileDescriptor&& other);

    ~FileDescriptor();
};

class LinuxInput {
    EventDispatcher* dispatcher;
    FileDescriptor kbd_fd;

public:
    LinuxInput(EventDispatcher& dispatcher, const DirectConfig::Input& cfg);

    void poll_events();
    void handle_event(struct input_event& event);
};

#endif
