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

cxx = meson.get_compiler('cpp')

if cxx.get_id() != 'clang' and cxx.get_id() != 'gcc'
    error('Compiler not supported')
endif

if host_machine.cpu_family() != 'x86_64'
    error('Host machine cpy family is required to be x86_64 (required by Grid::vol_scan)')
endif

if host_machine.endian() != 'little'
    error('Host machine endianness is required to be litte (required by Grid::from_tiff)')
endif

# Project sources & resources

sources = [
    'src/main.cpp',
    'src/main_loop.cpp',
    'src/sysinfo.cpp',
    'src/convert.cpp',
    'src/core/Logger.cpp',
    'src/core/Parser.cpp',
    'src/core/arg_parse.cpp',
    'src/graphics/core/Instance.cpp',
    'src/graphics/core/PhysicalDevice.cpp',
    'src/graphics/core/Device.cpp',
    'src/graphics/core/Swapchain.cpp',
    'src/graphics/memory/Image.cpp',
    'src/graphics/memory/Texture3D.cpp',
    'src/graphics/shader/Shader.cpp',
    'src/graphics/command/CommandPool.cpp',
    'src/graphics/utility.cpp',
    'src/render/Renderer.cpp',
    'src/render/RenderContext.cpp',
    'src/render/MultiplexRenderer.cpp',
    'src/render/SvoRaytraceAlgorithm.cpp',
    'src/render/DdaRaytraceAlgorithm.cpp',
    'src/render/RenderStats.cpp',
    'src/camera/OrbitCameraController.cpp',
    'src/camera/ScriptCameraController.cpp',
    'src/backend/backend.cpp',
    'src/backend/SwapImage.cpp',
    'src/backend/RenderDevice.cpp',
    'src/backend/headless/headless.cpp',
    'src/backend/headless/HeadlessDisplay.cpp',
    'src/backend/headless/HeadlessConfig.cpp',
    'src/backend/headless/HeadlessOutput.cpp',
    'src/model/Grid.cpp',
    'src/model/Octree.cpp'
]

shaders = [
    'resources/dda.comp',
    'resources/svo_naive.comp',
    'resources/esvo.comp',
    'resources/svo_df.comp',
    'resources/svo_rope.comp'
]

resources = [
    'resources/help.txt',
    'resources/help/help.txt',
    'resources/help/sysinfo.txt',
    'resources/help/convert.txt',
    'resources/help/render.txt',
    'resources/help/xorg_multi_gpu.txt',
    'resources/help/headless_config.txt',
    'resources/help/direct_config.txt',
]

add_project_arguments(
    [
        '-Wconversion',
        '-Wall',
        '-Wextra',
        '-Wno-unused-parameter' # silence warnigns generated by vulkan.hpp
    ],
    language: 'cpp'
)

# C++17 <filesystem> support
add_project_link_arguments(
    '-lstdc++fs',
    language: 'cpp'
)

# Project-wide dependencies
vk_dep = dependency('vulkan', method: 'system')
dependencies = [
    vk_dep,
    dependency('libtiff-4'),
    subproject('fmt').get_variable('fmt_dep'),
    subproject('lodepng').get_variable('lodepng_dep')
]

# Generate version.h
versions = meson.project_version().split('.')
conf = configuration_data()
conf.set_quoted('NAME', meson.project_name())
conf.set('MAJOR', versions[0])
conf.set('MINOR', versions[1])
conf.set('PATCH', versions[2])

configure_file(
    input: 'version.in.h',
    output: 'version.h',
    configuration: conf
)

# Add configuration for the direct backend
if get_option('present-direct').enabled()
    message('Building with direct backend')

    # Direct presenting requires raw linux input, and is thus
    # not supported on other platforms
    if host_machine.system() != 'linux'
        error('Direct display system requires linux')
    endif

    add_project_arguments('-DXENODON_PRESENT_DIRECT', language: 'cpp')

    sources += [
        'src/backend/direct/direct.cpp',
        'src/backend/direct/DirectDisplay.cpp',
        'src/backend/direct/DirectOutput.cpp',
        'src/backend/direct/ScreenGroup.cpp',
        'src/backend/direct/DirectConfig.cpp',
        'src/backend/direct/input/LinuxInput.cpp',
        'src/backend/direct/input/linux_translate_key.cpp'
    ]

    dependencies += [
        dependency('threads')
    ]
endif

# Add configuration for the xorg backend
if get_option('present-xorg').enabled()
    message('Building with xorg presenting support')

    # Even though were not using X11 but XCB, we still need this header for the
    # key symbol definitions.
    if not cxx.has_header('X11/keysym.h')
        error('X11 keysym header not found (X11/keysym.h)')
    endif

    add_project_arguments(
        [
            '-DXENODON_PRESENT_XORG',
            '-DVK_USE_PLATFORM_XCB_KHR'
        ],
        language: 'cpp'
    )

    sources += [
        'src/backend/xorg/xorg.cpp',
        'src/backend/xorg/XorgDisplay.cpp',
        'src/backend/xorg/XorgOutput.cpp',
        'src/backend/xorg/Window.cpp',
        'src/backend/xorg/XorgMultiGpuConfig.cpp',
        'src/backend/xorg/xorg_translate_key.cpp'
    ]

    dependencies += [
        dependency('xcb', method: 'pkg-config'),
        dependency('xcb-keysyms', method: 'pkg-config'),
        dependency('xcb-xkb', method: 'pkg-config')
    ]
endif

# Silence gcc warnings about vulkan-hpp's memcpy
if cxx.get_id() == 'gcc'
    add_project_arguments(['-Wno-class-memaccess'], language: 'cpp')
endif

if not cxx.has_header('vulkan/vulkan.h', dependencies: vk_dep)
    error('Vulkan C headers not found (vulkan/vulkan.h)')
endif

# Check if vulkan-hpp is installed.
if not cxx.has_header('vulkan/vulkan.hpp', dependencies: vk_dep)
    error('Vulkan C++ headers not found (vulkan/vulkan.hpp)')
endif

# GLSL -> SPIR-V compiler
# glslc should be included in the LunarG SDK
glslc = find_program('glslc')
spv_gen = generator(
    glslc,
    output: '@PLAINNAME@.spv',
    arguments: ['--target-env=vulkan1.1', '@INPUT@', '-o', '@OUTPUT@']
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

resources_host = custom_target(
    'gen-resources',
    input: inputs,
    output: ['resources.h', 'resources.S'],
    command: resources_command
)

dependencies += declare_dependency(
    sources: resources_host
)

# Main binary
executable('xenodon', sources,
    install: true,
    build_by_default: true,
    dependencies: dependencies,
    include_directories: include_directories('src'),
    link_args: '-g'
)
