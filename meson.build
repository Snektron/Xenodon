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

add_project_arguments(
    [
        '-march=native',
        '-DGLFW_INCLUDE_NONE',
        '-Wconversion'
    ],
    language: 'cpp'
)

# Silence gcc warnings about vulkan-hpp's memcpy
if cxx.get_id() == 'gcc'
    add_project_arguments(['-Wno-class-memaccess'], language: 'cpp')
endif

vk_dep = dependency('vulkan', method: 'system')
glfw_dep = dependency('glfw3', version: '>=3.2.1', fallback: ['glfw3', 'glfw_dep'])

if not cxx.has_header('vulkan/vulkan.h', dependencies: vk_dep)
    error('Vulkan C headers not found (vulkan/vulkan.h)')
endif

if not cxx.has_header('vulkan/vulkan.hpp', dependencies: vk_dep)
    error('Vulkan C++ headers not found (vulkan/vulkan.hpp)')
endif

include = include_directories('src')

sources = [
    'src/main.cpp',
]

executable('xenodon', sources,
    install: true,
    build_by_default: true,
    dependencies: [vk_dep, glfw_dep],
    include_directories: include
)
