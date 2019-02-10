project(
    'Xenodon',
    'cpp',
    version: '0.0.0',
    default_options: [
        'cpp_std=c++17',
        'b_lto=true',
        'buildtype=debugoptimized',
    ]
)

# Project sources & resources

sources = [
    'src/headless/headless.cpp',
    'src/interactive/interactive.cpp',
    'src/interactive/Window.cpp',
    'src/interactive/EventLoop.cpp',
    'src/interactive/Swapchain.cpp',
    'src/interactive/DisplayArray.cpp',
    'src/interactive/Display.cpp',
    'src/interactive/WindowManager.cpp',
    'src/interactive/SurfaceInfo.cpp',
    'src/render/PhysicalDeviceInfo.cpp',
    'src/render/Renderer.cpp',
    'src/main.cpp'
]

shaders = [
    'resources/test.frag',
    'resources/test.vert'
]

resources = [
    'resources/options.txt'
]

add_project_arguments(
    [
        '-march=native',
        '-Wconversion',
        '-DVK_USE_PLATFORM_XCB_KHR'
    ],
    language: 'cpp'
)

cxx = meson.get_compiler('cpp')

# Silence gcc warnings about vulkan-hpp's memcpy
if cxx.get_id() == 'gcc'
    add_project_arguments(['-Wno-class-memaccess'], language: 'cpp')
endif

vk_dep = dependency('vulkan', method: 'system')
xcb_dep = dependency('xcb')

if not cxx.has_header('vulkan/vulkan.h', dependencies: vk_dep)
    error('Vulkan C headers not found (vulkan/vulkan.h)')
endif

# Check if vulkan-hpp is installed.
if not cxx.has_header('vulkan/vulkan.hpp', dependencies: vk_dep)
    error('Vulkan C++ headers not found (vulkan/vulkan.hpp)')
endif

# GLSL compiler
glslc = find_program('glslangValidator')
spv_gen = generator(
    glslc,
    output: '@PLAINNAME@.spv',
    arguments: ['-V', '@INPUT@', '-o', '@OUTPUT@']
)

# Compile resources into the binary
generate_resources = find_program('tools/generate_resources.py')
resources_command = [generate_resources, '-i', '@OUTPUT0@', '-s', '@OUTPUT1@']
inputs = []

foreach resource : resources
    resources_command += ['-f', '@INPUT@0@@'.format(inputs.length()), resource]
    inputs += resource
endforeach

foreach shader : shaders
    resources_command += ['-f', '@INPUT@0@@'.format(inputs.length()), shader]
    inputs += spv_gen.process(shader)
endforeach

resources_target = custom_target(
    'gen-resources',
    input: inputs,
    output: ['resources.h', 'resources.S'],
    command: resources_command
)

resources_dep = declare_dependency(
    sources: resources_target
)

# Main binary
include = include_directories('src')

executable('xenodon', sources,
    install: true,
    build_by_default: true,
    dependencies: [vk_dep, xcb_dep, resources_dep],
    include_directories: include,
    link_args: '-g'
)
