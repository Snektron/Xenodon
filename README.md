# Xenodon
Volumetric Ray Tracer written in C++17 using Vulkan.

Xenodon offers several different render output backends:
- X.org: Render to a common window, or to a set of windows
- Headless: Render to a file
- Direct: Render to monitors without a display server.

## Usage
Basic usage of rendering with the X.org backend can be achieved by
```
$ build/xenodon render --xorg <volume>
```
See `xenodon help` for a detailed explanation on how to operate the program.

## Volumes
Xenodon can render 2 types of volumes:
- Uniform Grids. These can be passed to Xenodon in 3D stacked TIFF files. Each pixel of an image represents the emission color of a voxel, and each layer of the TIFF image should have the same dimensions.
- Sparse Voxel Octrees. These can be converted by Xenodon from 3D stacked TIFF files, see `xenodon help convert` for details on that operation.

## Traversal
5 Different traversal algorithms are implemented:
- DDA, implemented as a modified version of [A Fast Voxel Traversal Algorithm for Ray Tracing](https://www.researchgate.net/publication/2611491_A_Fast_Voxel_Traversal_Algorithm_for_Ray_Tracing) by Amanatides & woo.
- A naive sparse voxel octree traversal algorithm.
- A depth-first sparse voxel octree traversal algorithm.
- A modified version of [Efficient Sparse Voxel Octrees](https://research.nvidia.com/publication/efficient-sparse-voxel-octrees) by Laine and Kerras.
- A sparse voxel octree version of [Ray Tracing with Rope Trees](https://www.researchgate.net/publication/2691301_Ray_Tracing_with_Rope_Trees) by Havran, Bittner and Zara.

## Screenshots
![Stanford bunny](screenshots/bunny.png)

A rendering of the CT-Scan of the stanford bunny. The original volume is 512 by 361 by 512 voxels.

![TNG-100](screenshots/TNG100.gif)

Rendering of the TNG100-2 volume from the [TNG project](http://tng-project.org/). This volume is synthesized almost 700 million gas cells from snapshot 99, processed to a volume of 2048³ voxels.

![TNG-300](screenshots/TNG300.png)

Rendering of the TNG300-3 volume, also from the TNG project. 1024³ voxels constructed from over 224 million gas cells.

It should be noted that the latter two do not reach interactive framerates. Tests results from a Nvidia Titan X (pascal) 12 GB reach about 20 FPS with the DDA algorithm on the TNG300 volume, and below that for the TNG100 volume.

## Compiling
Several dependencies are automatically downloaded and compiled, however, the X.org and Direct render output backends in particular require some additional system libraries:
- X.org requires xcb, xcb-keysyms and xcb-xkb
- Direct depends on linux for getting input events.

For this reason, the X.org and Direct backends can be disabled at compile time by passing some flags to meson. To disable X.org, pass -Dpresent-xorg=disabled, and to disable Direct, pass -Dpresent-direct=disabled.

Additional dependencies required are Vulkan development files and libraries (including Vulkan-Hpp), and libtiff4. The former can for example be downloaded from [LunarG](https://www.lunarg.com/).

To compile the project, please run
```
$ mkdir build
$ cd build
$ meson ..
$ ninja
```

## Creating volumes
The volumes tested with are created with the utility scripts make-tng-volume.py and make-bunny-volume located in tools/. See the comments in those files for further details.

## Creating screenshots and movies/gifs
Creating screenshot images can be done by rendering with the headless backend. Included in the project is a camera file which can be used to take a single screenshot. As an example:
```
$ build/xenodon render --headless headless.conf bunny.tif --camera ./camera-single.txt -e 10
```
Creating movies or GIFs can be done by rendering an image for each frame. Included is a camera file which perform a complete rotation around the volume in 150 frames:
```
$ build/xenodon render --headless headless.conf bunny.tif --camera ./camera-rotate.txt -e 10 --output 'out-{:0>3}.png'
```
Images can then be converted to an MP4 file with ffmpeg:
```$ ffmpeg -r 30 -f image2 -i out-%03d.png -vcodec libx264 -crf 25 -pix_fmt yuv420p out.mp4```
Or to a gif:
```
$ ffmpeg -i out-%3d.png -vf palettegen palette.png
$ ffmpeg -i out-%3d.png -i palette.png -filter_complex "scale=320:-1:flags=lanczos[x];[x][1:v]paletteuse" out.gif
```

## Benchmarking
The program can be benchmarked with the headless backend by omitting saving images. Per-frame statistics and summaries can be saved with the `--stats-output` option. Included in the project is also a camera file which provides a good benchmark bases: the camera is rotated around the volume, zoomed in and rotated inside the volume.
```
$ build/xenodon render --headless headless.conf bunny.tif --camera ./camera.txt -e 10 --discard-output --stats-output stats.txt'
```