# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, blocks

equal_shaped_images = spritegrid.equal_shaped_images


std_layout = [['',   '',   '',   ''   ],
              ['n',  'e',  's',  'w'  ],
              ['ne', 'es', 'sw', 'nw' ],
              ['ns', 'ew', 'ns', 'ew' ],
              ['nes','esw','nsw','new'],
              ['N',  'E',  'S',  'W']]

GRASS = 16
SAND  = 32
CURSOR_TEST = 48

def write_groundRCD(images, tile_width, tile_height, ground_type, verbose, dest_fname):
    """
    Write an RCD file with ground sprites.

    @param images: Images of the ground. Mapping of name to image.
    @type  images: C{dict} of C{str} to L{ImageObject}

    @param tile_width: Width of a tile (from left to right) in pixels.
    @type  tile_width: C{int}

    @param tile_height: Height of a Z level in pixels.
    @type  tile_height: C{int}

    @param ground_type: Type of ground.
    @type  ground_type: C{int}

    @param verbose: Be verbose about the size and offsets of the processed
                    sprites.
    @param verbose: C{bool}

    @param dest_fname: Name of RCD file to write.
    @type  dest_fname: C{str}
    """
    file_data = blocks.RCD()

    spr = spritegrid.save_sprites(file_data, images, verbose)

    values = dict(('n#' + d[0], d[1]) for d in spr.iteritems())
    values['ground_type'] = ground_type
    values['tile_width'] = tile_width
    values['z_height'] = tile_height
    surf = blocks.Surface(values)
    file_data.add_block(surf)

    file_data.to_file(dest_fname)

