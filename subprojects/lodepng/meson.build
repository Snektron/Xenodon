project(
    'lodepng',
    'cpp',
    version: '20190210',
    license: 'zlib'
)

inc = include_directories('include')

lib = library('lodepng', ['lodepng.cpp'],
    include_directories: inc
)

lodepng_dep = declare_dependency(
    include_directories: inc,
    link_with: lib
)
