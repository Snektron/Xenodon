#include "backend/backend.h"
#include "backend/headless/headless.h"
#include "core/Error.h"

#if defined(XENODON_PRESENT_XORG)
    #include "backend/xorg/xorg.h"
#endif

#if defined(XENODON_PRESENT_DIRECT)
    #include "backend/direct/direct.h"
#endif

std::unique_ptr<Display> create_headless_backend(EventDispatcher& dispatcher, std::filesystem::path config, std::optional<std::filesystem::path> output) {
    return create_headless_display(dispatcher, config, output);
}

std::unique_ptr<Display> create_xorg_backend(EventDispatcher& dispatcher, std::filesystem::path multi_gpu_config) {
    #if defined(XENODON_PRESENT_XORG)
        return create_xorg_display(dispatcher, multi_gpu_config);
    #else
        throw Error("Not compiled with xorg presenting backend");
    #endif
}

std::unique_ptr<Display> create_direct_backend(EventDispatcher& dispatcher, std::filesystem::path config) {
    #if defined(XENODON_PRESENT_XORG)
        return create_direct_display(dispatcher, config);
    #else
        throw Error("Not compiled with direct presenting backend");
    #endif
}
