#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
# Try to load an RCD file.
# Code is separate from the RCD creation by design.
#
import sys

class RCD(object):
    def __init__(self, fname):
        self.fname = fname
        fp = open(fname, 'rb')
        self.data = fp.read()
        fp.close()

    def get_size(self):
        return len(self.data)

    def name(self, offset):
        return self.data[offset:offset+4]

    def uint8(self, offset):
        return ord(self.data[offset])

    def uint16(self, offset):
        val = self.uint8(offset)
        return val | (self.uint8(offset + 1) << 8)

    def uint32(self, offset):
        val = self.uint16(offset)
        return val | (self.uint16(offset + 2) << 16)

blocks = {'8PAL' : 1, '8PXL' : 1, 'SPRT' : 2, 'SURF' : 3}

def list_blocks(rcd):
    sz = rcd.get_size()

    rcd_name = rcd.name(0)
    rcd_version = rcd.uint32(4)
    print "%06X: (file header)" % 0
    print "    Name: " + repr(rcd_name)
    print "    Version: " + str(rcd_version)
    print
    if rcd_name != 'RCDF' or rcd_version != 1:
        print "ERROR"
        return

    offset = 8
    while offset < sz:
        name = rcd.name(offset)
        version = rcd.uint32(offset + 4)
        length = rcd.uint32(offset + 8)
        print "%06X" % offset
        print "    Name: " + repr(name)
        print "    Version: " + str(version)
        print "    Length: " + str(length)
        print
        if name not in blocks or blocks[name] != version:
            print "ERROR"
            return

        assert length > 0
        offset = offset + 12 + length
    assert offset == sz

if len(sys.argv) == 1:
    print "Missing RCD file argument."
    sys.exit(1)

for f in sys.argv[1:]:
    rcd = RCD(f)
    list_blocks(rcd)
