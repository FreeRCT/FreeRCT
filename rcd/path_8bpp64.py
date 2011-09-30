#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import path
import argparse

parser = argparse.ArgumentParser(description='Process a path sprites image.')
parser.add_argument(dest='image_file', metavar='img-file', type=str,
                   help='Image file contains the path sprites')
parser.add_argument('--output', dest='output', action='store',
                   metavar='rcd-file', help='Output RCD file')
#parser.add_argument('-w, --width', dest='tile_width', action='store', default=64,
#                   metavar='tile_width', help='Width of a tile (left to right) in pixels.')
#parser.add_argument('-h, --height', dest='tile_height', action='store', default=16,
#                   metavar='tile_height', help='Height of a tile in pixels.')
args = parser.parse_args()

if args.output is None:
    if args.image_file.lower().endswidth('.png'):
        out_name = args.image_file[:-4] + '.rcd'
    else:
        out_name = args.image_file + '.rcd'
else:
    out_name = args.output


images = path.split_image(args.image_file, -32, -33, 64, 64)
path.write_pathRCD(images, path.CONCRETE, 64, 16, out_name)

