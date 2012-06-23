# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from PIL import Image
from rcdlib import blocks

class ImageLoader(object):
    """
    Manager class for loaded images.
    """
    def __init__(self):
        self.last_fname = None
        self.last_img = None

    def get_img(self, fname):
        if fname == self.last_fname:
            return self.last_img

        self.last_fname = fname
        self.last_img = Image.open(fname)
        assert self.last_img.mode == 'P' # 'P' means 8bpp indexed
        return self.last_img

image_loader = ImageLoader()


class ImageObject(object):
    """
    Class representing an image of a sprite.

    @ivar im: Image of the whole picture (possibly much larger than the sprite).
    @type im: L{Pil.Image}

    @ivar xoffset: Horizontal offset to the reference point of the image.
    @type xoffset: C{int}

    @ivar yoffset: Vertical offset to the reference point of the image.
    @type yoffset: C{int}

    @ivar xpos: X position of top-left edge.
    @type xpos: C{int}

    @ivar ypos: Y position of top-left edge.
    @type ypos: C{int}

    @ivar xsize: Horizontal size of the image.
    @type xsize: C{int}

    @ivar ysize: Vertical size of the image.
    @type ysize: C{int}
    """
    def __init__(self, im, xoffset, yoffset, xpos, ypos, xsize=None, ysize=None):
        self.im = im
        self.xoffset = xoffset
        self.yoffset = yoffset
        self.xpos = xpos
        self.ypos = ypos

        if xsize is None:
            self.xsize = im.width - self.xpos
        else:
            self.xsize = xsize

        if ysize is None:
            self.ysize = im.height - self.ypos
        else:
            self.ysize = ysize

    def __cmp__(self, other):
        raise NotImplementedError("Equality only")

    def __ne__(self, other):
        return not (self == other)

    def __eq__(self, other):
        return self.im is other.im \
                and self.xoffset == other.xoffset and self.yoffset == other.yoffset \
                and self.xpos == other.xpos and self.ypos == other.ypos \
                and self.xsize == other.xsize and self.ysize == other.ysize


    def set_offset(self, xoffset=None, yoffset=None):
        """
        Set offset of the image relative to the reference point.

        @param xoffset: New horizontal offset (if not C{None}).
        @type  xoffset: C{int} or C{None}

        @param yoffset: New vertical offset (if not C{None}).
        @type  yoffset: C{int} or C{None}
        """
        if xoffset is not None: self.xoffset = xoffset
        if yoffset is not None: self.yoffset = yoffset


    def crop(self, trans=0):
        """
        Minimize image size by dropping transparent lines at the edges.

        @param trans: Transparent colour (0 by default).
        @type  trans: C{int}
        """
        # Check left vertical edge
        while self.xsize > 0 and self.ysize > 0:
            drop_edge = True
            for y in range(self.ypos, self.ypos + self.ysize):
                if self.im.getpixel((self.xpos, y)) != trans:
                    drop_edge = False
                    break
            if not drop_edge: break

            self.xpos = self.xpos + 1
            self.xsize = self.xsize - 1
            self.xoffset = self.xoffset + 1

        # Check top horizontal edge
        while self.xsize > 0 and self.ysize > 0:
            drop_edge = True
            for x in range(self.xpos, self.xpos + self.xsize):
                if self.im.getpixel((x, self.ypos)) != trans:
                    drop_edge = False
                    break
            if not drop_edge: break

            self.ypos = self.ypos + 1
            self.ysize = self.ysize - 1
            self.yoffset = self.yoffset + 1

        # Check right vertical edge
        while self.xsize > 0 and self.ysize > 0:
            drop_edge = True
            x = self.xpos + self.xsize - 1
            for y in range(self.ypos, self.ypos + self.ysize):
                if self.im.getpixel((x, y)) != trans:
                    drop_edge = False
                    break
            if not drop_edge: break

            self.xsize = self.xsize - 1

        # Check bottom horizontal edge
        while self.xsize > 0 and self.ysize > 0:
            drop_edge = True
            y = self.ypos + self.ysize - 1
            for x in range(self.xpos, self.xpos + self.xsize):
                if self.im.getpixel((x, y)) != trans:
                    drop_edge = False
                    break
            if not drop_edge: break

            self.ysize = self.ysize - 1


    def decode_row(self, y, transparent):
        """
        Find sequences of pixels to draw.
        """
        last = None
        start = None
        seqs = []
        for x in range(self.xsize):
            col = self.im.getpixel((self.xpos + x, y))
            if last is None:
                if col != transparent:
                    start, last = x, [col]
            else:
                if col == transparent:
                    seqs.append((start, last))
                    last = None
                else:
                    last.append(col)

        if last is not None:
            seqs.append((start,last))
        return seqs

    def make_8PXL(self, trans=0, skip_crop=False):
        """
        Encode the image to a 8PXL block.

        @param trans: Colour index of the transparent colour.
        @type  trans: C{int}

        @param skip_crop: If C{True}, skip cropping.
        @type  skip_crop: C{bool}

        @return: 8PXL block if it is a valid sprite, else C{None}
        @rtype:  :L{Pixels8Bpp} or C{None}
        """
        if not skip_crop: self.crop(trans)

        if self.xsize == 0 or self.ysize == 0:
            return None

        y_lines = {}
        for y in range(self.ysize):
            seqs = self.decode_row(self.ypos + y, 0)
            if len(seqs) == 0:
                continue

            line = []
            offset = 0
            for idx,(x,pixs) in enumerate(seqs):
                if idx + 1 == len(seqs):
                    add = 128
                else:
                    add = 0

                while x-offset > 127:
                    line = line + [127,0]
                    offset += 127
                while len(pixs) > 255:
                    line = line + [x-offset, 255] + pixs[:255]
                    x += 255
                    pixs = pixs[255:]
                    offset = x
                line = line + [add+x-offset, len(pixs)] + pixs
                offset = x + len(pixs)

            y_lines[y] = line


        image_data = []
        for y in range(self.ysize):
            line = y_lines.get(y, [])
            if len(line) > 0:
                image_data.append(''.join(chr(k) for k in line))
            else:
                image_data.append(None)

        values = {'width'    : self.xsize,
                  'height'   : self.ysize,
                  'x_offset' : self.xoffset,
                  'y_offset' : self.yoffset,
                  'lines'    : image_data}
        return blocks.Pixels8Bpp(values)



def split_spritegrid(fname, xoffset, yoffset, xsize, ysize, layout):
    """
    Split the sprites laid out in a grid in images.

    @param fname: Name of the image file.
    @type  fname: C{str}

    @param xoffset: Default x offset.
    @type  xoffset: C{int}

    @param yoffset: Default y offset.
    @type  yoffset: C{int}

    @param xsize: Horizontal size of a sprite in the grid. Use C{None} if you don't care.
    @type  xsize: C{int} or C{None}

    @param ysize: Vertical size of a sprite in the grid. Use C{Nome} if you don't care.
    @type  ysize: C{int} or C{None}

    @param layout: Layout of the images in the grid.
    @type  layout: C{list} or rows, where each row is a list of image names (C{list} of C{str}).

    @return: Mapping of image names to image objects.
    @rtype:  C{dict} of C{str} to L{ImageObject}
    """

    im = Image.open(fname)
    assert im.mode == 'P' # 'P' means 8bpp indexed

    ycount = len(layout)
    xcount = None
    for row in layout:
        if xcount is None:
            xcount = len(row)
        else:
            assert xcount == len(row) # Otherwise not a rectangular grid given

    if xsize is None: xsize = im.size[0] // xcount
    if ysize is None: ysize = im.size[1] // ycount

    if xcount * xsize != im.size[0]:
        print "Warning: File %s has width %s while using only %s*%s=%s pixel columns." \
                % (fname, im.size[0], xcount, xsize, xcount * xsize)
    if ycount * ysize != im.size[1]:
        print "Warning: File %s has height %s while using only %s*%s=%s pixel rows." \
                % (fname, im.size[1], ycount, ysize, ycount * ysize)

    imgs = {}
    for x in range(xcount):
        for y in range(ycount):
            img = ImageObject(im, xoffset, yoffset, x * xsize, y * ysize,
                              xsize, ysize)
            img.crop()
            imgs[layout[y][x]] = img

    return imgs

def save_sprites(file_data, images, verbose):
    """
    Save the sprites in L{images} is sprite blocks.

    @param file_data: RCD file being written.
    @type  file_data: L{blocks.RCD}

    @param images: Sprites to save, mapping of name to sprite.
    @type  images: C{dict} of C{str} to L{ImageObject}

    @param verbose: Be verbose about size and offsets of generated sprites.
    @type  verbose: C{bool}

    @return: Mapping of names to data block numbers in the file.
    @rtype:  C{dict} of C{str} to (C{int}, C{None} if not saved).
    """
    spr = {} # name -> sprite block number
    for name, img_obj in images.iteritems():
        if verbose:
            print "%6s: width=%d, height=%d, xoff=%d, yoff=%d" \
                % (name, img_obj.xsize, img_obj.ysize, img_obj.xoffset, img_obj.yoffset)
        pxl_blk = img_obj.make_8PXL(skip_crop = True)
        spr[name] = file_data.add_block(pxl_blk)

    return spr


def equal_shaped_images(im1, im2):
    """
    Verify whether both images are the same with respect to transparency.
    (That is, do they have the same non-transparent shape?)

    @param im1: First image.
    @type  im1: L{ImageObject}

    @param im2: Second image.
    @type  im2: L{ImageObject}

    @return: Whether both images have the same non-transparent shape.
    @rtype:  C{bool}
    """
    im1.crop()
    im2.crop()

    if im1.xsize != im2.xsize:
        print "Horizontal size not the same."
        return False
    if im1.ysize != im2.ysize:
        print "Vertical size not the same."
        return False

    for y in range(im1.ysize):
        for x in range(im1.xsize):
            p1 = im1.im.getpixel((im1.xpos + x, im1.ypos + y))
            p2 = im2.im.getpixel((im2.xpos + x, im2.ypos + y))
            if p1 == 0 and p2 == 0: continue
            if p1 != 0 and p2 != 0: continue

            print "Different at " + str((x, y))
            return False

    return True
