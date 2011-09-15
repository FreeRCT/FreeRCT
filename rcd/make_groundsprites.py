# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from PIL import Image

class Output(object):
    def __init__(self):
        pass

    def uint8(self, val):
        assert val >= 0 and val <= 255
        self.store(val)

    def int8(self, val):
        assert val >= -128 and val <= 127
        self.uint8(val & 0xFF)

    def uint16(self, val):
        assert val >= 0 and val < 0x10000
        self.uint8(val & 0xFF)
        self.uint8((val >> 8) & 0xFF)

    def int16(self, val):
        assert val >= -0x8000 and val <= 0x7fff
        self.uint8(val & 0xFF)
        self.uint8((val >> 8) & 0xFF)

    def uint32(self, val):
        assert val >= 0 and val <= 0x100000000
        self.uint8(val & 0xFF)
        self.uint8((val >> 8) & 0xFF)
        self.uint8((val >> 16) & 0xFF)
        self.uint8((val >> 24) & 0xFF)

    def int32(self, val):
        assert val >= -0x80000000 and val <= 0x7fffffff
        self.uint8(val & 0xFF)
        self.uint8((val >> 8) & 0xFF)
        self.uint8((val >> 16) & 0xFF)
        self.uint8((val >> 24) & 0xFF)

    def bytes(self, val):
        for b in val:
            self.uint8(b)


class MemoryOutput(Output):
    """
    Output class for storing data in memory.

    @ivar data: Stored data.
    @type data: C{list} of C{str}
    """
    def __init__(self):
        Output.__init__(self)
        self.data = []

    def get(self):
        """
        Return collected data.
        """
        return ''.join(self.data)

    def store(self, val):
        self.data.append(chr(val))

class FileOutput(Output):
    """
    Output to disk.
    """
    def __init__(self, fp = None):
        Output.__init__(self)
        self.fp = fp

    def set_file(self, fname):
        assert self.fp is None # Don't supply a fp to the constructor.
        self.fp = open(fname, 'wb')

    def close(self):
        self.fp.close()

    def store(self, val):
        self.fp.write(chr(val))


def encode(data, xoffset, yoffset):
    """
    data = list of rows, where each row is a tuple (y, list of seqs).
           a seq is a (x-start, pixel-list) tuple.
    """
    ybase = None
    ymax  = None
    xbase = None
    xmax  = None
    for y,seqs in data:
        if ybase is None or y < ybase: ybase = y
        if ymax  is None or y > ymax:  ymax  = y
        for idx,(x,pixs) in enumerate(seqs):
            assert len(pixs) > 0
            if xbase is None or x < xbase: xbase = x
            if xmax  is None or x + len(pixs) - 1 > xmax:  xmax  = x + len(pixs) - 1

    ylength = ymax - ybase + 1
    xlength = xmax - xbase + 1

    y_lines = {}
    for y,seqs in data:
        line = []
        offset = 0
        for idx,(x,pixs) in enumerate(seqs):
            x -= xbase
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


    pix_blk = Pixels8Bpp(xlength, ylength)
    for y in range(ybase, ybase+ylength):
        line = y_lines.get(y, [])
        if len(line) > 0:
            pix_blk.add_line(''.join(chr(k) for k in line))
        else:
            pix_blk.add_line(None)

    return xoffset + xbase, yoffset + ybase, pix_blk


def decode_row(im, bx, xsize, y, transparent):
    last = None
    start = None
    seqs = []
    for x in range(xsize):
        col = im.getpixel((x+bx, y))
        if last is None:
            if col == transparent:
                pass
            else:
                start = x
                last = [col]
        else:
            if col == transparent:
                seqs.append((start, last))
                last = None
            else:
                last.append(col)

    if last is not None:
        seqs.append((start,last))
    return seqs

def decode(im, bx, by, xsize, ysize):
    """
    Scan the image for non-transparent pixels, and collect them by lists of
    sequences (x-offset, [pixels]) for each line.
    """
    transparent = im.getpixel((bx, by))
    rows = []
    for y in range(ysize):
        seqs = decode_row(im, bx, xsize, by+y, transparent)
        if len(seqs) > 0:
            rows.append((y,seqs))
    return rows


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

    def write(self, output):
        for n in self.name:
            output.uint8(ord(n)) # Looks somewhat stupid, converting strings to numbers and back
        output.uint32(self.version)


class FileHeader(Block):
    """
    Header of the file.
    """
    def __init__(self):
        Block.__init__(self, 'RCDF', 1)


class DataBlock(Block):
    """
    A data block.
    """
    def __init__(self, name, version):
        Block.__init__(self, name, version)

    def write(self, output):
        Block.write(self, output)
        output.uint32(self.get_size())

    def get_size(self):
        """
        Compute size of the block, and return it to the caller.
        """
        raise NotImplementedError("Implement me in %s" % type(self))


class Palette8Bpp(DataBlock):
    """
    8PAL block.
    """
    def __init__(self):
        DataBlock.__init__(self, '8PAL', 1)
        self.rgbs = []

    def add_rgb(self, rgb):
        self.rgbs.append(rgb)

    def get_size(self):
        assert len(self.rgbs) >= 1 and len(self.rgbs) <= 256
        return 2 + 3 * len(self.rgbs)

    def write(self, output):
        DataBlock.write(self, output)
        output.uint16(len(self.rgbs))
        for r,g,b in self.rgbs:
            output.uint8(r)
            output.uint8(g)
            output.uint8(b)

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

    def write(self, output):
        assert len(self.line_data) == self.height
        DataBlock.write(self, output)
        output.uint16(self.width)
        output.uint16(self.height)
        offset = 4 * self.height
        for line in self.line_data:
            if line is not None:
                output.uint32(offset)
                offset = offset + len(line)
            else:
                output.uint32(0)
        for line in self.line_data:
            if line is not None:
                # XXX It should be possible to dump the whole string in one go.
                # XXX In particular since the back-end converts it back to characters :p
                output.bytes(ord(b) for b in line)

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

    def write(self, output):
        DataBlock.write(self, output)
        output.int16(self.xoff)
        output.int16(self.yoff)
        output.uint32(self.img_block)
        output.uint32(self.palette_block)

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

    def write(self, output):
        DataBlock.write(self, output)
        output.uint16(self.tile_width)
        output.uint16(self.z_height)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('n')],
                          output)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('e')],
                          output)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('s')],
                          output)
        self.write_blocks([self.sprites[self.sort_name(name)] for name in self.spr_names('w')],
                          output)

    def write_blocks(self, blocks, output):
        for block in blocks:
            if block is None:
                output.uint32(0)
            else:
                output.uint32(block)


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

class RCD(object):
    """
    A RCD (Roller Coaster Data) file.
    """
    def __init__(self):
        self.file_header = FileHeader()
        self.blocks = []
        self.count = 0

    def add_block(self, block):
        self.count = self.count + 1
        self.blocks.append(block)
        return self.count

    def write(self, output):
        self.file_header.write(output)
        for block in self.blocks:
            block.write(output)

    def to_file(self, fname):
        output = FileOutput()
        output.set_file(fname)
        self.write(output)
        output.close()

def dump_line(y, line, width):
    xpos = 0
    offset = 0
    l = "%2d" % y
    while True:
        relpos = ord(line[offset])
        count  = ord(line[offset+1])
        xpos = xpos + (relpos & 127)
        newx = xpos + count
        bad = ""
        if newx > width:
            bad = "<==========="
        print "%s: %d-%d%s" % (l, xpos, newx, bad)
        l = "  "
        xpos = newx
        offset = offset + 2 + count
        if (relpos & 128): break


#
# Main program
#

im = Image.open('../sprites/GroundTileTemplate64px_8bpp.png')
#print "mode", im.mode
#print "size", im.size, (im.size[0]/4.0, im.size[1]/6.0)
#print "palette", im.palette
assert im.mode == 'P'


tiles = [['',   '',   '',   ''   ],
         ['n',  'e',  's',  'w'  ],
         ['ne', 'es', 'sw', 'nw' ],
         ['ns', 'ew', 'ns', 'ew' ],
         ['nes','esw','nsw','new'],
         ['N',  'E',  'S',  'W']]

tdict = {}
for ridx,trow in enumerate(tiles):
    for cidx,tcol in enumerate(trow):
        data = decode(im, cidx*64, ridx*64, 64, 64)
        out = MemoryOutput()
        tdict[tcol] = encode(data, -32, -33) # xoffset, yoffset, 8PXL block.


file_data = RCD()

# kill im?!
lut = im.resize((256, 1))
lut.putdata(range(256))
lut = lut.convert("RGB").getdata()
pb = Palette8Bpp()
for rgb in lut:
    pb.add_rgb(rgb)
palb_num = file_data.add_block(pb)

spr = {}
for name, blkdata in tdict.iteritems():
    xoff, yoff, pixblk = blkdata
    print "%4s: width=%d, height=%d, xoff=%d, yoff=%d" % (name, pixblk.width, pixblk.height, xoff, yoff)
    #for idx, d in enumerate(pixblk.line_data):
    #    dump_line(idx, d, pixblk.width)
    #print
    pix_blknum = file_data.add_block(pixblk)
    spr_blk = Sprite(xoff, yoff, pix_blknum, palb_num)
    spr[name] = file_data.add_block(spr_blk)

surf = Surface(64, 16)
for name, sprb_num in spr.iteritems():
    surf.add_sprite(name, sprb_num)
file_data.add_block(surf)

file_data.to_file('groundsprites.rcd')
