# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, blocks

CONCRETE = 16

def write_pathRCD(images, path_type, tile_width, tile_height, verbose, dest_fname):
    """
    Write an RCD file with path sprites.

    @param images: Images of the path. Mapping of name to image.
    @type  images: C{dict} of C{str} to L{ImageObject}

    @param path_type: Type of the path.
    @type  path_type: C{int}

    @param tile_width: Width of a tile (from left to right) in pixels.
    @type  tile_width: C{int}

    @param tile_height: Height of a Z level in pixels.
    @type  tile_height: C{int}

    @param verbose: Be verbose about size and offsets of generated sprites.
    @type  verbose: C{bool}

    @param dest_fname: Name of RCD file to write.
    @type  dest_fname: C{str}
    """
    file_data = blocks.RCD()

    spr = spritegrid.save_sprites(file_data, images, verbose)
    spr['path_type'] = path_type
    spr['tile_width'] = tile_width
    spr['z_height'] = tile_height
    surf = blocks.Paths(spr)
    file_data.add_block(surf)

    file_data.to_file(dest_fname)

