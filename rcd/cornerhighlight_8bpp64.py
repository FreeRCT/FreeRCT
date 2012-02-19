#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, corner_tiles
import argparse

parser = argparse.ArgumentParser(description='Process a corner select image.')
parser.add_argument(dest='image_file', metavar='img-file', type=str,
                   help='Image file contains the corner selection sprites')
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

std_layout = [['n#',   'e#',   's#',   'w#'   ],
              ['n#n',  'e#e',  's#s',  'w#w'  ],
              ['n#ne', 'e#es', 's#sw', 'w#nw' ],
              ['n#ns', 'e#ew', 's#ns', 'w#ew' ],
              ['n#nes','e#esw','s#nsw','w#new'],
              ['n#N',  'e#E',  's#S',  'w#W'],
              ['e#',   's#',   'w#',   'n#'   ],
              ['e#n',  's#e',  'w#s',  'n#w'  ],
              ['e#ne', 's#es', 'w#sw', 'n#nw' ],
              ['e#ns', 's#ew', 'w#ns', 'n#ew' ],
              ['e#nes','s#esw','w#nsw','n#new'],
              ['e#N',  's#E',  'w#S',  'n#W'],
              ['s#',   'w#',   'n#',   'e#'   ],
              ['s#n',  'w#e',  'n#s',  'e#w'  ],
              ['s#ne', 'w#es', 'n#sw', 'e#nw' ],
              ['s#ns', 'w#ew', 'n#ns', 'e#ew' ],
              ['s#nes','w#esw','n#nsw','e#new'],
              ['s#N',  'w#E',  'n#S',  'e#W'],
              ['w#',   'n#',   'e#',   's#'   ],
              ['w#n',  'n#e',  'e#s',  's#w'  ],
              ['w#ne', 'n#es', 'e#sw', 's#nw' ],
              ['w#ns', 'n#ew', 'e#ns', 's#ew' ],
              ['w#nes','n#esw','e#nsw','s#new'],
              ['w#N',  'n#E',  'e#S',  's#W']]

images = spritegrid.split_spritegrid(args.image_file, -32, -33, 64, 64, std_layout)
corner_tiles.write_cornerselectRCD(images, 64, 16, args.verbose, out_name)

