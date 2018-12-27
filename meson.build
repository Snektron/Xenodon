project(
    'Xenodon',
    'cpp',
    version: '0.0.0',
    default_options: [
        'cpp_std=c++17',
        'cpp_args=[\'-march=native\']',
        'b_lto=true',
        'buildtype=debugoptimized',
    ]
)

dependencies = [
    dependency('threads'),
    dependency('GL'),
    dependency('glfw3', version: '>=3.2.1', fallback: ['glfw3', 'glfw_dep'])
]

sources = [
    'src/main.cpp'
]

executable('xenodon', sources,
    install: true,
    build_by_default: true,
    dependencies: dependencies
)
