# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import output

needed = ['', 'n', 'e', 'ne', 's', 'ns', 'es', 'nes', 'w', 'nw', 'ew', 'new', 'sw', 'nsw', 'esw', 'N', 'E', 'S', 'W']

# {{{ class Block(object):
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
# }}}
# {{{ class FileHeader(Block):
class FileHeader(Block):
    """
    Header of the file.
    """
    def __init__(self):
        Block.__init__(self, 'RCDF', 1)
# }}}
# {{{ class DataBlock(Block):
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
# }}}
# {{{ class GeneralDataBlock(Block):
class GeneralDataBlock(Block):
    """
    General data block class.

    @ivar fields: Mapping of name to type for the fields in the block, where type is one of
                  int8, uint8, int16 uint16 uint32 block.
    @type fields: C{list} of (C{str}, C{str})

    @ivar values: Mapping of fields to numeric values.
    @type values: C{dict} of C{str} to C{int}
    """
    def __init__(self, name, version, fields, values):
        Block.__init__(self, name, version)
        self.fields = fields
        self.values = values

    def write(self, out):
        Block.write(self, out)
        out.uint32(self.get_size())
        for fldname, fldtype in self.fields:
            if fldtype == 'int8':
                out.int8(self.values[fldname])
            elif fldtype == 'uint8':
                out.uint8(self.values[fldname])
            elif fldtype == 'int16':
                out.int16(self.values[fldname])
            elif fldtype == 'uint16':
                out.uint16(self.values[fldname])
            elif fldtype == 'uint32':
                out.uint32(self.values[fldname])
            elif fldtype == 'block':
                out.uint32(self.values[fldname])
            else:
                raise ValueError("Unknown field-type: %r" % fldtype)

    def get_size(self):
        """
        Compute size of the block, and return it to the caller.
        """
        sizes = {'int8':1, 'uint8':1, 'int16':2, 'uint16':2, 'uint32':4, 'block':4}
        total = 0
        for fldname, fldtype in self.fields:
            total = total + sizes[fldtype]
        return total

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
        for fldname, fldtype in self.fields:
            if self.values[fldname] != other.values[fldname]:
                return False
        return True

    def set_value(self, name, val):
        self.values[name] = val
# }}}
# {{{ class Foundation(GeneralDataBlock):
class Foundation(GeneralDataBlock):
    def __init__(self, found_type, tile_width, tile_height):
        fields = [('found_type', 'uint16'),
                  ('tile_width', 'uint16'),
                  ('tile_height', 'uint16'),
                  ('se_e', 'block'),
                  ('se_s', 'block'),
                  ('se_se', 'block'),
                  ('sw_s', 'block'),
                  ('sw_w', 'block'),
                  ('sw_sw', 'block')]
        values = {'found_type': found_type,
                  'tile_width': tile_width,
                  'tile_height': tile_height}
        GeneralDataBlock.__init__(self, 'FUND', 1, fields, values)
# }}}
# {{{ class CornerTile(GeneralDataBlock):
class CornerTile(GeneralDataBlock):
    def __init__(self, tile_width, tile_height):
        fields = [('tile_width', 'uint16'),
                  ('tile_height', 'uint16')]
        fields.extend([('n#'+n, 'block') for n in needed])
        fields.extend([('e#'+n, 'block') for n in needed])
        fields.extend([('s#'+n, 'block') for n in needed])
        fields.extend([('w#'+n, 'block') for n in needed])
        values = {'tile_width': tile_width,
                  'tile_height': tile_height}
        GeneralDataBlock.__init__(self, 'TCOR', 1, fields, values)
# }}}
# {{{ class Palette8Bpp(DataBlock):
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
# }}}
# {{{ class Pixels8Bpp(DataBlock):
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
            if line is not None:
                out.store_text(line)

    def is_equal(self, other):
        return self.width == other.width and self.height == other.height \
                and self.line_data == other.line_data
# }}}
# {{{ class Sprite(GeneralDataBlock):
class Sprite(GeneralDataBlock):
    """
    SPRT data block.
    """
    def __init__(self, xoff, yoff, img_block):
        fields = [('x_offset', 'int16'),
                  ('y_offset', 'int16'),
                  ('image', 'block')]
        values = {'x_offset' : xoff, 'y_offset' : yoff, 'image' : img_block}
        GeneralDataBlock.__init__(self, 'SPRT', 2, fields, values)
# }}}
# {{{ class Surface(GeneralDataBlock):
class Surface(GeneralDataBlock):
    """
    Game block 'SURF'
    """
    def __init__(self, tile_width, z_height, ground_type, sprites):
        fields = [('ground_type', 'uint16'),
                  ('tile_width', 'uint16'),
                  ('z_height', 'uint16')]
        fields.extend([('n#'+n, 'block') for n in needed])
        sprites['ground_type'] = ground_type
        sprites['tile_width'] = tile_width
        sprites['z_height'] = z_height
        GeneralDataBlock.__init__(self, 'SURF', 3, fields, sprites)
# }}}
# {{{ class TileSelection(GeneralDataBlock):
class TileSelection(GeneralDataBlock):
    """
    Game block 'TSEL'
    """
    def __init__(self, tile_width, z_height, sprites):
        fields = [('tile_width', 'uint16'), ('z_height', 'uint16')]
        fields.extend([('n#'+n, 'block') for n in needed])
        sprites['tile_width'] = tile_width
        sprites['z_height'] = z_height
        GeneralDataBlock.__init__(self, 'TSEL', 1, fields, sprites)
# }}}
# {{{ class Paths(GeneralDataBlock):
path_sprite_names = ['empty', 'ne', 'se', 'ne_se', 'ne_se_e', 'sw', 'ne_sw',
    'se_sw', 'se_sw_s', 'ne_se_sw', 'ne_se_sw_e', 'ne_se_sw_s', 'ne_se_sw_e_s',
    'nw', 'ne_nw', 'ne_nw_n', 'nw_se', 'ne_nw_se', 'ne_nw_se_n', 'ne_nw_se_e',
    'ne_nw_se_n_e', 'nw_sw', 'nw_sw_w', 'ne_nw_sw', 'ne_nw_sw_n', 'ne_nw_sw_w',
    'ne_nw_sw_n_w', 'nw_se_sw', 'nw_se_sw_s', 'nw_se_sw_w', 'nw_se_sw_s_w',
    'ne_nw_se_sw', 'ne_nw_se_sw_n', 'ne_nw_se_sw_e', 'ne_nw_se_sw_n_e',
    'ne_nw_se_sw_s', 'ne_nw_se_sw_n_s', 'ne_nw_se_sw_e_s', 'ne_nw_se_sw_n_e_s',
    'ne_nw_se_sw_w', 'ne_nw_se_sw_n_w', 'ne_nw_se_sw_e_w', 'ne_nw_se_sw_n_e_w',
    'ne_nw_se_sw_s_w', 'ne_nw_se_sw_n_s_w', 'ne_nw_se_sw_e_s_w',
    'ne_nw_se_sw_n_e_s_w', 'ramp_ne', 'ramp_nw', 'ramp_se', 'ramp_sw']

class Paths(GeneralDataBlock):
    """
    Game block 'PATH'
    """
    def __init__(self, path_type, tile_width, z_height, sprites):
        fields = [('path_type', 'uint16'),
                  ('tile_width', 'uint16'),
                  ('z_height', 'uint16')]
        fields.extend([(name, 'block') for name in path_sprite_names])
        sprites['path_type'] = path_type
        sprites['tile_width'] = tile_width
        sprites['z_height'] = z_height
        GeneralDataBlock.__init__(self, 'PATH', 1, fields, sprites)
# }}}
# {{{ class RCD(object):
class RCD(object):
    """
    A RCD (Roller Coaster Data) file.
    """
    def __init__(self):
        self.file_header = FileHeader()
        self.blocks = []

    def add_block(self, block):
        if block is None:
            return None

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
# }}}

