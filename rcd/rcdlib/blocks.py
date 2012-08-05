# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import output, datatypes

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

    def __cmp__(self, other):
        raise NotImplementedError("No general comparison available, only equality!")

    def __ne__(self, other):
        return not (self == other)

# }}}
# {{{ class FileHeader(Block):
class FileHeader(Block):
    """
    Header of the file.
    """
    def __init__(self):
        Block.__init__(self, 'RCDF', 1)
# }}}
# {{{ class GeneralDataBlock(Block):

class GeneralDataBlock(Block):
    """
    General data block class.

    @ivar fields: Mapping of name to type for the fields in the block.
    @type fields: C{list} of (C{str}, L{DataType})

    @ivar values: Mapping of fields to numeric values.
    @type values: C{dict} of C{str} to C{int}, or C{None} if not available
                  during construction
    """
    def __init__(self, name, version, fields, values):
        Block.__init__(self, name, version)
        self.fields = fields
        self.values = values

        # Check the values of the dictionary.
        for fn, fd in self.fields:
            if not isinstance(fd, datatypes.DataType):
                raise RuntimeError("field: name=%r, type=%r is wrong" % (fn, fd))

    def set_values(self, values):
        """
        Set the values of the block.

        @param values: Mapping of fields names to numeric values.
        @type  values: C{dict} of C{str} to C{int}
        """
        self.values = values

    def write(self, out):
        Block.write(self, out)
        out.uint32(self.get_size())
        for fldname, fldtype in self.fields:
            fldtype.write(out, self.values[fldname])

    def get_size(self):
        """
        Compute size of the block, and return it to the caller.
        """
        total = 0
        for fldname, fldtype in self.fields:
            total = total + fldtype.get_size(self.values[fldname])
        return total

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
        s_names = set(f[0] for f in self.fields)
        o_names = set(f[0] for f in other.fields)
        c_names = s_names.intersection(o_names)
        if len(c_names) != len(s_names) or len(c_names) != len(o_names): return False
        for fldname in c_names:
            if self.values[fldname] != other.values[fldname]:
                return False
        return True
# }}}
# {{{ class Pixels8Bpp(GeneralDataBlock):
class Pixels8Bpp(GeneralDataBlock):
    """
    8PXL block.
    """
    def __init__(self, values):
        fields = [('width',    datatypes.UINT16_TYPE),
                  ('height',   datatypes.UINT16_TYPE),
                  ('x_offset', datatypes.INT16_TYPE),
                  ('y_offset', datatypes.INT16_TYPE),
                  ('lines',    datatypes.IMAGE_DATA_TYPE)]
        GeneralDataBlock.__init__(self, '8PXL', 2, fields, values)
# }}}
# {{{ class TextBlock(GeneralDataBlock):
class TextBlock(Block):
    """
    TEXT block.
    """
    def __init__(self, value):
        Block.__init__(self, 'TEXT', 1)
        self.value = value

    def write(self, out):
        Block.write(self, out)
        out.uint32(len(self.value))
        out.store_text(self.value)

    def __eq__(self, other):
        if type(self) is not type(other):
            return False
        return self.value == other.value

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
        #if isinstance(block, Pixels8Bpp):
        #    print "Pixel block number %d" % len(self.blocks)
        #    print "width = %d, height = %d" % (block.values['width'], block.values['height'])
        #    for y, data in enumerate(block.values['lines']):
        #        if data is None:
        #            print "%2d: - empty -" % y
        #        else:
        #            print "%2d: %s" % (y, ' '.join('%02x' % ord(c) for c in data))
        #    print

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

