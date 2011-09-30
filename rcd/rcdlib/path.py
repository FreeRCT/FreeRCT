# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, blocks

# Sprites as laid out in the source image.
std_layout = [['empty',             'empty',             'empty',             'empty'],
              ['ne',                'se',                'sw',                'nw'],
              ['ne_se',             'se_sw',             'nw_sw',             'ne_nw'],
              ['ne_sw',             'nw_se',             'ne_sw',             'nw_se'],
              ['ne_se_sw',          'nw_se_sw',          'ne_nw_sw',          'ne_nw_se'],
              ['ne_nw_se_sw',       'ne_nw_se_sw',       'ne_nw_se_sw',       'ne_nw_se_sw'],
              ['ne_se_e',           'se_sw_s',           'nw_sw_w',           'ne_nw_n'],
              ['ne_se_sw_e',        'nw_se_sw_s',        'ne_nw_sw_w',        'ne_nw_se_n'],
              ['ne_se_sw_s',        'nw_se_sw_w',        'ne_nw_sw_n',        'ne_nw_se_e'],
              ['ne_se_sw_e_s',      'nw_se_sw_s_w',      'ne_nw_sw_n_w',      'ne_nw_se_n_e'],
              ['ne_nw_se_sw_n_s_w', 'ne_nw_se_sw_n_e_w', 'ne_nw_se_sw_n_e_s', 'ne_nw_se_sw_e_s_w'],
              ['ne_nw_se_sw_n_w',   'ne_nw_se_sw_n_e',   'ne_nw_se_sw_e_s',   'ne_nw_se_sw_s_w'],
              ['ne_nw_se_sw_n_s',   'ne_nw_se_sw_e_w',   'ne_nw_se_sw_n_s',   'ne_nw_se_sw_e_w'],
              ['ne_nw_se_sw_n',     'ne_nw_se_sw_e',     'ne_nw_se_sw_s',     'ne_nw_se_sw_w'],
              ['ne_nw_se_sw',       'ne_nw_se_sw',       'ne_nw_se_sw',       'ne_nw_se_sw'],
              ['ramp_sw',           'ramp_nw',           'ramp_ne',           'ramp_se']]

# Sprites in the order of the RCD file.
sprites = ['empty', 'ne', 'se', 'ne_se', 'ne_se_e', 'sw', 'ne_sw', 'se_sw',
    'se_sw_s', 'ne_se_sw', 'ne_se_sw_e', 'ne_se_sw_s', 'ne_se_sw_e_s', 'nw',
    'ne_nw', 'ne_nw_n', 'nw_se', 'ne_nw_se', 'ne_nw_se_n', 'ne_nw_se_e',
    'ne_nw_se_n_e', 'nw_sw', 'nw_sw_w', 'ne_nw_sw', 'ne_nw_sw_n',
    'ne_nw_sw_w', 'ne_nw_sw_n_w', 'nw_se_sw', 'nw_se_sw_s', 'nw_se_sw_w',
    'nw_se_sw_s_w', 'ne_nw_se_sw', 'ne_nw_se_sw_n', 'ne_nw_se_sw_e',
    'ne_nw_se_sw_n_e', 'ne_nw_se_sw_s', 'ne_nw_se_sw_n_s', 'ne_nw_se_sw_e_s',
    'ne_nw_se_sw_n_e_s', 'ne_nw_se_sw_w', 'ne_nw_se_sw_n_w',
    'ne_nw_se_sw_e_w', 'ne_nw_se_sw_n_e_w', 'ne_nw_se_sw_s_w',
    'ne_nw_se_sw_n_s_w', 'ne_nw_se_sw_e_s_w', 'ne_nw_se_sw_n_e_s_w',
    'ramp_ne', 'ramp_nw', 'ramp_se', 'ramp_sw']

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

CONCRETE = 16

def write_pathRCD(images, path_type, tile_width, tile_height, dest_fname):
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

    @param dest_fname: Name of RCD file to write.
    @type  dest_fname: C{str}
    """
    file_data = blocks.RCD()

    spr = {} # name -> sprite block number
    for name, img_obj in images.iteritems():
        print "%4s: width=%d, height=%d, xoff=%d, yoff=%d" \
                % (name, img_obj.xsize, img_obj.ysize, img_obj.xoffset, img_obj.yoffset)
        pxl_blk = img_obj.make_8PXL(skip_crop = True)
        pix_blknum = file_data.add_block(pxl_blk)
        spr_blk = blocks.Sprite(img_obj.xoffset, img_obj.yoffset, pix_blknum)
        spr[name] = file_data.add_block(spr_blk)

    # XXX missing sprites go unnoticed!!
    spr_blocks = [spr.get(name) for name in sprites]
    surf = blocks.Paths(path_type, tile_width, tile_height, spr_blocks)
    file_data.add_block(surf)

    file_data.to_file(dest_fname)

