#!/usr/bin/env python3

# Quick utility to convert TNG datasets to 3D stacked TIFF images.
# This script is by no means optimized.
# Usage make-tng-volume.py <shapshots base directory> <snapshot>
# <volume side dimension> <output tif> <lower percentile> <upper percentile>
# The latter two parameters specify the percentiles between which to color the data.
# Because a lot of voxels will be empty, this would otherwise make a pretty bad volume.
# Parameters used for TNG100-2:
# make-tng-volume.py TNG100-2 98 2048 TNG100.tif 96 99
# Parameters used for TNG300-3:
# make-tng-volume.py TNG300-3 99 2048 TNG300.tif 90 99
# Note that this script can take up quite some resources and time.
# It took about 30 min for TNG100 for me.

import numpy as np
from PIL import Image
import math
import sys
import matplotlib as mp
import matplotlib.pyplot as plt
import illustris_python as il
import tifffile as tif

snaps = sys.argv[1]
snapshot = int(sys.argv[2])
dim = int(sys.argv[3])
out = sys.argv[4]
lower = float(sys.argv[5])
upper = float(sys.argv[6])

print('Loading dataset...')
gas = il.snapshot.loadSubset(snaps, shapshot, 'gas', ['Coordinates', 'Density'])

print('Creating histogram...')
a = np.histogramdd(gas['Coordinates'], bins=(dim, dim, dim), weights=gas['Density'])[0]

print('Normalizing...')
c = np.log(a + sys.float_info.min)
l = np.percentile(c.flatten(), lower)
h = np.percentile(c.flatten(), upper)

c = mp.colors.Normalize(l, h)(c)

print('Coloring...')
cm = plt.get_cmap('magma')
d = cm(c, None, True)

print('Saving image...')
tif.imsave(out, d, bigtiff=True)
print(f'Written to {out}')
