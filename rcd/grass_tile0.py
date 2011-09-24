#!/usr/bin/env python

from rcdlib import ground_tiles

fname = '../sprites/GroundTileTemplate64px_8bpp.png'

images = ground_tiles.split_image(fname, -32, -33, 64, 64)

ground_tiles.write_groundRCD(images, 'groundsprites_new.rcd')
