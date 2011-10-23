# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, blocks

# Sprites as laid out in the source image.
std_layout = [['ne', 'se', 'sw', 'nw']]

class Arrows(blocks.GeneralDataBlock):
    def __init__(self, tile_width, spr_blocks):
        fields = [('width', 'uint16'),
                  ('ne', 'block'),
                  ('se', 'block'),
                  ('sw', 'block'),
                  ('nw', 'block')]
        values = {'width': tile_width}
        values.update(spr_blocks)
        blocks.GeneralDataBlock.__init__(self, "BDIR", 1, fields, values)


# Sprites in the order of the RCD file.
sprites = ['ne', 'se', 'sw', 'nw']

def split_image(fname, xoffset, yoffset, xsize, ysize, layout=None):
    """
    Split the path tiles sprite into pieces.

    @param fname: Image file name.
    @type  fname: C{str}

    @param xoffset: Horizontal offset of the sprite for the top-left pixel.
    @type  xoffset: C{int}

    @param yoffset: Vertical offset of the sprite for the top-left pixel.
    @type  yoffset: C{int}

    @param xsize: Horizontal size of a sprite in the image.
    @type  xsize: C{int}

    @param ysize: Vertical size of a sprite in the image.
    @type  ysize: C{int}

    @param layout: Layout of sprites in the images as a 2D grid (by name).
                   If not specified it is L{std_layout}
    @type  layout: C{list} of C{list} of C{str}

    @return: Mapping of sprite names to sprite objects.
    """
    if layout is None: layout = std_layout
    images = spritegrid.split_spritegrid(fname, xoffset, yoffset, xsize, ysize, layout)
    for img in images.itervalues():
        img.crop()

    return images

def write_arrowRCD(images, tile_width, verbose, dest_fname):
    """
    Write an RCD file with path sprites.

    @param images: Images of the path. Mapping of name to image.
    @type  images: C{dict} of C{str} to L{ImageObject}

    @param tile_width: Width of a tile (from left to right) in pixels.
    @type  tile_width: C{int}

    @param verbose: Be verbose about size and offsets of generated sprites.
    @type  verbose: C{bool}

    @param dest_fname: Name of RCD file to write.
    @type  dest_fname: C{str}
    """
    file_data = blocks.RCD()

    spr = {} # name -> sprite block number
    for name, img_obj in images.iteritems():
        if verbose:
            print "%4s: width=%d, height=%d, xoff=%d, yoff=%d" \
                % (name, img_obj.xsize, img_obj.ysize, img_obj.xoffset, img_obj.yoffset)
        pxl_blk = img_obj.make_8PXL(skip_crop = True)
        if pxl_blk is not None:
            pix_blknum = file_data.add_block(pxl_blk)
            spr_blk = blocks.Sprite(img_obj.xoffset, img_obj.yoffset, pix_blknum)
        else:
            spr_blk = None
        spr[name] = file_data.add_block(spr_blk)

    spr_blocks = dict((name, spr.get(name)) for name in sprites)
    surf = Arrows(tile_width, spr_blocks)
    file_data.add_block(surf)

    file_data.to_file(dest_fname)

