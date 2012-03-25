# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
"""
Classes for outputting data to memory or file.
"""

class Output(object):
    """
    Base class for outputting binary data.
    """
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

    def store(self, val):
        """
        Store the given value.
        """
        raise NotImplementedError("Implement me in %r" % type(self))

    def store_text(self, txt):
        """
        Store the text as-is.
        """
        raise NotImplementedError("Implement me in %r" % type(self))


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

    def store_text(self, txt):
        self.data.append(txt)


class FileOutput(Output):
    """
    Output to disk.
    """
    def __init__(self, fp = None):
        Output.__init__(self)
        self.fp = fp
        self.magiccount = 0

    def set_file(self, fname):
        assert self.fp is None # Don't supply a fp to the constructor.
        self.fp = open(fname, 'wb')

    def close(self):
        self.fp.close()

    def store(self, val):
        self.fp.write(chr(val))

    def store_text(self, txt):
        self.fp.write(txt)

    def store_magic(self, mag):
        self.magiccount = self.magiccount + 1
        self.store_text(mag)
