#include "present/headless/headless.h"
#include "present/Event.h"
#include "core/Logger.h"

std::unique_ptr<HeadlessDisplay> make_headless_display(Span<const char*> args, EventDispatcher& dispatcher) {
    return nullptr;
}