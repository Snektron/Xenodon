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

# Project sources & resources

sources = [
    'src/main.cpp',
    'src/main_loop.cpp',
    'src/graphics/Device.cpp',
    'src/graphics/support.cpp',
    'src/graphics/Swapchain.cpp',
    'src/render/Renderer.cpp',
    'src/render/DeviceRenderer.cpp',
    'src/render/TestRenderer.cpp',
    'src/Config.cpp'
]

shaders = [
    'resources/test.frag',
    'resources/test.vert'
]

resources = [
    'resources/help.txt'
]

add_project_arguments(
    [
        '-march=native',
        '-Wconversion',
    ],
    language: 'cpp'
)

# Project-wide dependencies
vk_dep = dependency('vulkan', method: 'system')
dependencies = [vk_dep]

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

# Add configuration for the bare presenting backend
if (get_option('present-direct').enabled())
    message('Building with direct presenting support')

    # Direct presenting requires raw linux input, and is thus
    # not supported on other platforms
    if target_machine.system() != 'linux'
        error('Direct display system requires linux')
    endif

    add_project_arguments('-DXENODON_PRESENT_DIRECT', language: 'cpp')

    sources += [
        'src/present/direct/direct_main.cpp',
        'src/present/direct/DirectDisplay.cpp',
        'src/present/direct/DirectScreen.cpp',
        'src/present/direct/ScreenGroup.cpp',
        'src/present/direct/DirectConfig.cpp',
        'src/present/direct/input/LinuxInput.cpp',
        'src/present/direct/input/linux_translate_key.cpp'
    ]

    dependencies += [
        dependency('threads')
    ]
endif

# Add configuration for the xorg presenting backend
if (get_option('present-xorg').enabled())
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
        'src/present/xorg/xorg_main.cpp',
        'src/present/xorg/XorgDisplay.cpp',
        'src/present/xorg/XorgScreen.cpp',
        'src/present/xorg/XorgWindow.cpp',
        'src/present/xorg/xorg_translate_key.cpp'
    ]

    dependencies += [
        dependency('xcb'),
        dependency('xcb-keysyms'),
        dependency('xcb-xkb')
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

dependencies += declare_dependency(
    sources: resources_target
)

# Main binary
executable('xenodon', sources,
    install: true,
    build_by_default: true,
    dependencies: dependencies,
    include_directories: include_directories('src'),
    link_args: '-g'
)
