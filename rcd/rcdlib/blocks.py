# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import output

class Block(object):
    """
    A block in the rcd file (base class).

    @ivar name: Name of the block (4 letters).
    @type name: C{str}

    @ivar version: Version number of the block.
    @type version: C{int}
    """
    def __init__(self, name, version):
        self.name = name
        self.version = version

        assert len(name) == 4

    def write(self, out):
        out.store_text(self.name)
        out.uint32(self.version)


class FileHeader(Block):
    """
    Header of the file.
    """
    def __init__(self):
        Block.__init__(self, 'RCDF', 1)


class DataBlock(Block):
    """
    Base class for a data block.
    """
    def __init__(self, name, version):
        Block.__init__(self, name, version)

    def write(self, out):
        Block.write(self, out)
        out.uint32(self.get_size())

    def get_size(self):
        """
        Compute size of the block, and return it to the caller.
        """
        raise NotImplementedError("Implement me in %s" % type(self))

    def __cmp__(self, other):
        raise NotImplementedError("No general comparison available, only equality!")

    def __ne__(self, other):
        return not (self == other)

    def __eq__(self, other):
        if type(self) is not type(other):
            return False
        return self.is_equal(other)

    def is_equal(self, other):
        """
        Check whether you are the same as L{other}.
        @pre Classes are the same.
        @return Equality between self and other.
        """
        raise NotImplementedError("Implement me in %s" % type(self))


class Palette8Bpp(DataBlock):
    """
    8PAL block (not used any more).
    """
    def __init__(self):
        DataBlock.__init__(self, '8PAL', 1)
        self.rgbs = []

    def add_rgb(self, rgb):
        self.rgbs.append(rgb)

    def set_palette(self, im):
        im = im.copy()
        lut = im.resize((256, 1))
        lut.putdata(range(256))
        lut = lut.convert("RGB").getdata()
        for rgb in lut:
            self.add_rgb(rgb)

    def get_size(self):
        assert len(self.rgbs) >= 1 and len(self.rgbs) <= 256
        return 2 + 3 * len(self.rgbs)

    def write(self, out):
        DataBlock.write(self, out)
        out.uint16(len(self.rgbs))
        for r,g,b in self.rgbs:
            out.uint8(r)
            out.uint8(g)
            out.uint8(b)

class Pixels8Bpp(DataBlock):
    """
    8PXL block.
    """
    def __init__(self, width, height):
        DataBlock.__init__(self, '8PXL', 1)
        self.width  = width
        self.height = height
        self.line_data = []

    def add_line(self, line):
        """
        Append the data for one scan line. Insert 'None' for absent data.
        @param line: Scanline data.
        @type  line: Either a C{str} or C{None}
        """
        self.line_data.append(line)

    def get_size(self):
        assert len(self.line_data) == self.height
        total = 0
        for line in self.line_data:
            if line is not None:
                total = total + len(line)

        return 2 + 2 + 4 * self.height + total

    def write(self, out):
        assert len(self.line_data) == self.height
        DataBlock.write(self, out)
        out.uint16(self.width)
        out.uint16(self.height)
        offset = 4 * self.height
        for line in self.line_data:
            if line is not None:
                out.uint32(offset)
                offset = offset + len(line)
            else:
                out.uint32(0)
        for line in self.line_data:
            out.store_text(line)

    def is_equal(self, other):
        return self.width == other.width and self.height == other.height \
                and self.line_data == other.line_data

class Sprite(DataBlock):
    """
    SPRT data block.
    """
    def __init__(self, xoff, yoff, img_block, palette_block):
        DataBlock.__init__(self, 'SPRT', 1)
        self.xoff = xoff # signed
        self.yoff = yoff # signed
        self.img_block = img_block
        self.palette_block = palette_block

    def get_size(self):
        return 2+2+4+4

    def write(self, out):
        DataBlock.write(self, out)
        out.int16(self.xoff)
        out.int16(self.yoff)
        out.uint32(self.img_block)
        out.uint32(self.palette_block)

    def is_equal(self, other):
        return self.xoff == other.xoff and self.yoff == other.yoff \
                and self.img_block == other.img_block \
                and self.palette_block == other.palette_block

class Surface(DataBlock):
    """
    Game block 'SURF'
    """
    def __init__(self, tile_width, z_height):
        DataBlock.__init__(self, 'SURF', 1)
        self.tile_width = tile_width
        self.z_height = z_height
        self.sprites = dict((self.sort_name(x), None) for x in self.spr_names())

    def add_sprite(self, name, spr_block):
        name = self.sort_name(name)
        assert name in self.sprites
        self.sprites[name] = spr_block

    def sort_name(self, name):
        tmp = list(name)
        tmp.sort()
        return ''.join(tmp)

    def get_size(self):
        return 2 + 2 + 4*19*4

    def write(self, out):
        DataBlock.write(self, out)
        out.uint16(self.tile_width)
        out.uint16(self.z_height)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('n')], out)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('e')], out)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('s')], out)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('w')], out)
        #['', 'n', 'e', 'en', 's', 'ns', 'es', 'ens', 'w', 'nw', 'ew', 'enw', 'sw', 'nsw', 'esw', 'N', 'E', 'S', 'W']
        #['', 'e', 's', 'es', 'w', 'ew', 'sw', 'esw', 'n', 'en', 'ns', 'ens', 'nw', 'enw', 'nsw', 'E', 'S', 'W', 'N']
        #['', 's', 'w', 'sw', 'n', 'ns', 'nw', 'nsw', 'e', 'es', 'ew', 'esw', 'en', 'ens', 'enw', 'S', 'W', 'N', 'E']
        #['', 'w', 'n', 'nw', 'e', 'ew', 'en', 'enw', 's', 'sw', 'ns', 'nsw', 'es', 'esw', 'ens', 'W', 'N', 'E', 'S']

    def write_blocks(self, blocks, out):
        for block in blocks:
            if block is None:
                out.uint32(0)
            else:
                out.uint32(block)


    def spr_names(self, orientation = 'n'):
        """
        Return names of the sprites in the right order.
        """
        if orientation == 'n':
            n,e,s,w = 'n', 'e', 's', 'w'
        elif orientation == 'e':
            n,e,s,w = 'e', 's', 'w', 'n'
        elif orientation == 's':
            n,e,s,w = 's', 'w', 'n', 'e'
        elif orientation == 'w':
            n,e,s,w = 'w', 'n', 'e', 's'
        else:
            assert 0 # Wrong orientation.

        sprites = []
        for i in range(15):
            txt = ''
            if i & 1: txt = txt + n
            if i & 2: txt = txt + e
            if i & 4: txt = txt + s
            if i & 8: txt = txt + w
            sprites.append(txt)
        return sprites + [n.upper(), e.upper(), s.upper(), w.upper()]

    def is_equal(self, other):
        return self.tile_width == other.tile_width \
                and self.z_height == other.z_height \
                and self.sprites == other.sprites

class RCD(object):
    """
    A RCD (Roller Coaster Data) file.
    """
    def __init__(self):
        self.file_header = FileHeader()
        self.blocks = []

    def add_block(self, block):
        for idx, blk in enumerate(self.blocks):
            if blk == block:
                return idx + 1

        self.blocks.append(block)
        return len(self.blocks)

    def write(self, out):
        self.file_header.write(out)
        for block in self.blocks:
            block.write(out)

    def to_file(self, fname):
        out = output.FileOutput()
        out.set_file(fname)
        self.write(out)
        out.close()

