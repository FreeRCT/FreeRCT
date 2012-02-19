# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import blocks


def write_cornerselectRCD(images, tile_width, tile_height, verbose, dest_fname):
    """
    Write an RCD file with ground sprites.

    @param images: Images of the ground. Mapping of name to image.
    @type  images: C{dict} of C{str} to L{ImageObject}

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

    spr = {} # name -> sprite block number
    for name, img_obj in images.iteritems():
        if verbose:
            print "%6s: width=%d, height=%d, xoff=%d, yoff=%d" \
                % (name, img_obj.xsize, img_obj.ysize, img_obj.xoffset, img_obj.yoffset)
        pxl_blk = img_obj.make_8PXL(skip_crop = True)
        if pxl_blk is not None:
            pix_blknum = file_data.add_block(pxl_blk)
            spr_blk = blocks.Sprite(img_obj.xoffset, img_obj.yoffset, pix_blknum)
        else:
            spr_blk = None
        spr[name] = file_data.add_block(spr_blk)

    spr['tile_width'] = tile_width
    spr['tile_height'] = tile_height
    tcor = blocks.CornerTile(spr)
    file_data.add_block(tcor)

    file_data.to_file(dest_fname)

