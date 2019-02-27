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
]

shaders = [
    'resources/test.frag',
    'resources/test.vert'
]

resources = [
    'resources/options.txt'
]

vk_dep = dependency('vulkan', method: 'system')

dependencies = [vk_dep]

add_project_arguments(
    [
        '-march=native',
        '-Wconversion',
    ],
    language: 'cpp'
)

# Add configuration for the bare presenting backend
if (get_option('present-bare').enabled())
    message('Building with bare presenting support')

    add_project_arguments('-DXENODON_PRESENT_BARE', language: 'cpp')

    sources += [
        'src/present/bare/BareDisplay.cpp'
    ]
endif

# Add configuration for the xorg presenting backend
if (get_option('present-xorg').enabled())
    message('Building with xorg presenting support')

    add_project_arguments(
        [
            '-DXENODON_PRESENT_XORG',
            '-DVK_USE_PLATFORM_XCB_KHR'
        ],
        language: 'cpp'
    )

    sources += [
        'src/present/xorg/XorgDisplay.cpp',
        'src/present/xorg/Keyboard.cpp'
    ]

    dependencies += [
        dependency('xcb'),
        dependency('xcb-keysyms'),
        dependency('xcb-xkb')
    ]

    # Even though were not using X11 but XCB, we still need this header for the
    # key symbol definitions.
    if not cxx.has_header('X11/keysym.h')
        error('X11 keysym header not found (X11/keysym.h)')
    endif
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
