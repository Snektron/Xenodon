#!/usr/bin/env python3
# Quick utility to convert the stanford bunny ct-scan
# (https://graphics.stanford.edu/data/voldata/voldata.html#bunny)
# to a 3D stacked TIFF image. This script is by no means optimized.
# Usage make-bunny-volume.py <directory of bunny slices> <output directory>

import numpy as np
import struct
from PIL import Image
import tifffile as tif
import sys
import os

src_dir = sys.argv[1]
dst = sys.argv[2]

def read_slice(slice):
    with open(os.path.join(src_dir, str(slice)), 'rb') as f:
        a = list(struct.unpack('>' + 'H' * 512 * 512, f.read()))
        return np.array(a).reshape((512, 512)).astype('float32')

print('Reading volume...')
a = np.zeros((512, 362, 512))
for i in range(1, 362):
    a[:, i, :] = read_slice(i)

a = np.array(a)

a /= 4094/255
a = a.astype('uint8')

a = np.where(a > 5, a, 0)

print('Processing...')
def dist(x):
    return (x - 256)**2

for x, z in np.ndindex((512, 512)):
    if dist(x) + dist(z) > 250 ** 2:
        a[x, :, z] = 0

w,h,d = a.shape
b = np.zeros((w, h, d, 4), dtype='uint8')

b[:, :, :, 0] = a
b[:, :, :, 1] = a
b[:, :, :, 2] = a
b[:, :, :, 3] = 255

print('Saving...')
tif.imsave(dst, b, bigtiff=True)