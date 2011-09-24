from rcdlib import spritegrid, blocks

tiles = [['',   '',   '',   ''   ],
         ['n',  'e',  's',  'w'  ],
         ['ne', 'es', 'sw', 'nw' ],
         ['ns', 'ew', 'ns', 'ew' ],
         ['nes','esw','nsw','new'],
         ['N',  'E',  'S',  'W']]

def split_image(fname, xoffset, yoffset, xsize, ysize):
    """
    Split the ground tiles sprite into pieces.
    """
    images = spritegrid.split_spritegrid(fname, xoffset, yoffset, xsize, ysize, tiles)
    for img in images.itervalues():
        img.crop()

    return images


def write_groundRCD(images, dest_fname):
    """
    Write an RCD file with ground sprites.
    """
    file_data = blocks.RCD()

    pb = blocks.Palette8Bpp()
    pb.set_palette(images[''].im)
    palb_num = file_data.add_block(pb)

    spr = {}
    for name, img_obj in images.iteritems():
        print "%4s: width=%d, height=%d, xoff=%d, yoff=%d" \
                % (name, img_obj.xsize, img_obj.ysize, img_obj.xoffset, img_obj.yoffset)
        pxl_blk = img_obj.make_8PXL(skip_crop = True)
        pix_blknum = file_data.add_block(pxl_blk)
        spr_blk = blocks.Sprite(img_obj.xoffset, img_obj.yoffset, pix_blknum, palb_num)
        spr[name] = file_data.add_block(spr_blk)

    surf = blocks.Surface(64, 16)
    for name, sprb_num in spr.iteritems():
        surf.add_sprite(name, sprb_num)
    file_data.add_block(surf)

    file_data.to_file(dest_fname)

