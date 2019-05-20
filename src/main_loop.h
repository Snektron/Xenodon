#ifndef _XENODON_MAIN_LOOP_H
#define _XENODON_MAIN_LOOP_H

#include <string_view>
#include <filesystem>
#include "utility/Span.h"

struct EventDispatcher;
struct Display;

struct RenderParameters {
    std::filesystem::path model_path;
    std::string_view model_type_override;
    std::string_view shader;
    float density = 1.f;
};

void main_loop(EventDispatcher& dispatcher, Display* display, const RenderParameters& render_params);

#endif
