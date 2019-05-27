#ifndef _XENODON_MAIN_LOOP_H
#define _XENODON_MAIN_LOOP_H

#include <string_view>
#include <filesystem>
#include "utility/Span.h"
#include "math/Vec.h"

struct EventDispatcher;
struct Display;

enum class CameraType {
    Orbit,
    Benchmark
};

struct RenderParameters {
    std::filesystem::path model_path;
    std::string_view model_type_override;
    std::string_view shader;
    std::filesystem::path stats_save_path;
    Vec3F voxel_ratio = Vec3F(1, 1, 1);
    CameraType camera_type;
    float density = 1.f;
};

void main_loop(EventDispatcher& dispatcher, Display* display, const RenderParameters& render_params);

#endif
