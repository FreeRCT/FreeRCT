#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, foundations
import argparse

parser = argparse.ArgumentParser(description='Process a foundation image.')
parser.add_argument(dest='image_file', metavar='img-file', type=str,
                   help='Image file contains the foundation sprites')
parser.add_argument('--output', dest='output', action='store',
                   metavar='rcd-file', help='Output RCD file')
parser.add_argument('--verbose', action='store_true', default=False,
                    help="Output size and offset of the sprites.")
args = parser.parse_args()

if args.output is None:
    if args.image_file.lower().endswidth('.png'):
        out_name = args.image_file[:-4] + '.rcd'
    else:
        out_name = args.image_file + '.rcd'
else:
    out_name = args.output

fname = '../sprites/foundationtemplate8bpp64_masked.png'

std_layout = [['se_se', 'sw_sw'],
              ['se_e',  'sw_w'],
              ['se_s',  'sw_s']]

images = spritegrid.split_spritegrid(args.image_file, -32, -33, 64, 64, std_layout)
foundations.write_foundationRCD(images, 64, 16, foundations.GROUND, args.verbose, out_name)

