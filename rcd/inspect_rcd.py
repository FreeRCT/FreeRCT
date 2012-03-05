#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
"""
Try to load an RCD file.
Code is separate from the RCD creation by design.
"""
from PIL import Image
import sys

palette = [
    (  0,   0, 255), (255, 255, 255), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0),
    (  0,   0,   0), (  0,   0,   0), ( 23,  35,  35), ( 35,  51,  51), ( 47,  67,  67), ( 63,  83,  83), ( 75,  99,  99), ( 91, 115, 115),
    (111, 131, 131), (131, 151, 151), (159, 175, 175), (183, 195, 195), (211, 219, 219), (239, 243, 243), ( 51,  47,   0), ( 63,  59,   0),
    ( 79,  75,  11), ( 91,  91,  19), (107, 107,  31), (119, 123,  47), (135, 139,  59), (151, 155,  79), (167, 175,  95), (187, 191, 115),
    (203, 207, 139), (223, 227, 163), ( 67,  43,   7), ( 87,  59,  11), (111,  75,  23), (127,  87,  31), (143,  99,  39), (159, 115,  51),
    (179, 131,  67), (191, 151,  87), (203, 175, 111), (219, 199, 135), (231, 219, 163), (247, 239, 195), ( 71,  27,   0), ( 95,  43,   0),
    (119,  63,   0), (143,  83,   7), (167, 111,   7), (191, 139,  15), (215, 167,  19), (243, 203,  27), (255, 231,  47), (255, 243,  95),
    (255, 251, 143), (255, 255, 195), ( 35,   0,   0), ( 79,   0,   0), ( 95,   7,   7), (111,  15,  15), (127,  27,  27), (143,  39,  39),
    (163,  59,  59), (179,  79,  79), (199, 103, 103), (215, 127, 127), (235, 159, 159), (255, 191, 191), ( 27,  51,  19), ( 35,  63,  23),
    ( 47,  79,  31), ( 59,  95,  39), ( 71, 111,  43), ( 87, 127,  51), ( 99, 143,  59), (115, 155,  67), (131, 171,  75), (147, 187,  83),
    (163, 203,  95), (183, 219, 103), ( 31,  55,  27), ( 47,  71,  35), ( 59,  83,  43), ( 75,  99,  55), ( 91, 111,  67), (111, 135,  79),
    (135, 159,  95), (159, 183, 111), (183, 207, 127), (195, 219, 147), (207, 231, 167), (223, 247, 191), ( 15,  63,   0), ( 19,  83,   0),
    ( 23, 103,   0), ( 31, 123,   0), ( 39, 143,   7), ( 55, 159,  23), ( 71, 175,  39), ( 91, 191,  63), (111, 207,  87), (139, 223, 115),
    (163, 239, 143), (195, 255, 179), ( 79,  43,  19), ( 99,  55,  27), (119,  71,  43), (139,  87,  59), (167,  99,  67), (187, 115,  83),
    (207, 131,  99), (215, 151, 115), (227, 171, 131), (239, 191, 151), (247, 207, 171), (255, 227, 195), ( 15,  19,  55), ( 39,  43,  87),
    ( 51,  55, 103), ( 63,  67, 119), ( 83,  83, 139), ( 99,  99, 155), (119, 119, 175), (139, 139, 191), (159, 159, 207), (183, 183, 223),
    (211, 211, 239), (239, 239, 255), (  0,  27, 111), (  0,  39, 151), (  7,  51, 167), ( 15,  67, 187), ( 27,  83, 203), ( 43, 103, 223),
    ( 67, 135, 227), ( 91, 163, 231), (119, 187, 239), (143, 211, 243), (175, 231, 251), (215, 247, 255), ( 11,  43,  15), ( 15,  55,  23),
    ( 23,  71,  31), ( 35,  83,  43), ( 47,  99,  59), ( 59, 115,  75), ( 79, 135,  95), ( 99, 155, 119), (123, 175, 139), (147, 199, 167),
    (175, 219, 195), (207, 243, 223), ( 63,   0,  95), ( 75,   7, 115), ( 83,  15, 127), ( 95,  31, 143), (107,  43, 155), (123,  63, 171),
    (135,  83, 187), (155, 103, 199), (171, 127, 215), (191, 155, 231), (215, 195, 243), (243, 235, 255), ( 63,   0,   0), ( 87,   0,   0),
    (115,   0,   0), (143,   0,   0), (171,   0,   0), (199,   0,   0), (227,   7,   0), (255,   7,   0), (255,  79,  67), (255, 123, 115),
    (255, 171, 163), (255, 219, 215), ( 79,  39,   0), (111,  51,   0), (147,  63,   0), (183,  71,   0), (219,  79,   0), (255,  83,   0),
    (255, 111,  23), (255, 139,  51), (255, 163,  79), (255, 183, 107), (255, 203, 135), (255, 219, 163), (  0,  51,  47), (  0,  63,  55),
    (  0,  75,  67), (  0,  87,  79), (  7, 107,  99), ( 23, 127, 119), ( 43, 147, 143), ( 71, 167, 163), ( 99, 187, 187), (131, 207, 207),
    (171, 231, 231), (207, 255, 255), ( 63,   0,  27), ( 91,   0,  39), (119,   0,  59), (147,   7,  75), (179,  11,  99), (199,  31, 119),
    (219,  59, 143), (239,  91, 171), (243, 119, 187), (247, 151, 203), (251, 183, 223), (255, 215, 239), ( 39,  19,   0), ( 55,  31,   7),
    ( 71,  47,  15), ( 91,  63,  31), (107,  83,  51), (123, 103,  75), (143, 127, 107), (163, 147, 127), (187, 171, 147), (207, 195, 171),
    (231, 219, 195), (255, 243, 223), (255,   0, 255), (255, 183,   0), (255, 219,   0), (255, 255,   0), (  7, 107,  99), (  7, 107,  99),
    (  7, 107,  99), ( 27, 131, 123), ( 39, 143, 135), ( 55, 155, 151), ( 55, 155, 151), ( 55, 155, 151), (115, 203, 203), (155, 227, 227),
    ( 47,  47,  47), ( 87,  71,  47), ( 47,  47,  47), (  0,   0,  99), ( 27,  43, 139), ( 39,  59, 151), (  0,   0,   0), (  0,   0,   0),
    (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0), (  0,   0,   0),
]

nums = {'1': [   ' x',    ' x',    ' x',    ' x',    ' x'],
        '0': [ ' xxx',  ' x x',  ' x x',  ' x x',  ' xxx'],
        '2': [ ' xx ',  '   x',  '  x ',  ' x  ',  ' xxx'],
        '3': [ ' xxx',  '   x',  '  xx',  '   x',  ' xxx'],
        '4': ['   x ', '  xx ', ' x x ', ' xxxx', '   x '],
        '5': [ ' xxx',  ' x  ',  ' xx ',  '   x',  ' xx '],
        '6': [ '  xx',  ' x  ',  ' xxx',  ' x x',  ' xxx'],
        '7': [ ' xxx',  '   x',  '  x ',  '  x ',  '  x '],
        '8': [ ' xxx',  ' x x',  ' xxx',  ' x x',  ' xxx'],
        '9': [ ' xxx',  ' x x',  ' xxx',  '   x',  ' xx ']}

def numpix(number):
    lines = ['','','','','']
    for num in str(number):
        lines = [l+n for l,n in zip(lines, nums[num])]
    assert len(lines) == 5
    assert len(lines[0]) == len(lines[1])
    assert len(lines[0]) == len(lines[2])
    assert len(lines[0]) == len(lines[3])
    assert len(lines[0]) == len(lines[4])
    return lines

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

    def int16(self, offset):
        v = self.uint16(offset)
        if (v & (1 << 15)) != 0:
            v = -((v ^ 0xFFFF) + 1)
            return v
        return v

    def uint32(self, offset):
        val = self.uint16(offset)
        return val | (self.uint16(offset + 2) << 16)

def tcor_img(prefix, rcd, offset):
    sprites = 'flat n e ne s ns es nes w wn we wne ws wns wes steepN steepE steepS steepW'.split()
    for idx, sp in enumerate(sprites):
        print "    TCOR " + prefix + sp + ": " + str(rcd.uint32(offset + idx*4))

class PictureDump(object):
    IM_HEIGHT=600
    IM_WIDTH=800

    def __init__(self):
        self.im = None
        self.xpos = None
        self.ypos = None
        self.ybottom = None
        self.number = 1

    def new_image(self):
        self.im = Image.new("P", (PictureDump.IM_WIDTH, PictureDump.IM_HEIGHT), 0)
        self.im.putpalette([x for c in palette for x in c])
        self.xpos = 10
        self.ypos = 10
        self.ybottom = 10

    def plot(self, x, y, val):
        self.im.putpixel((x,y), val)

    def save(self):
        if self.im is None: return

        fname = "img%02d.png" % self.number
        self.number = self.number + 1
        self.im.save(fname)
        self.im = None

    def add_image(self, rcd, offset, number):
        w = rcd.uint16(offset + 12)
        h = rcd.uint16(offset + 14)
        if self.im is None:
            self.new_image()

        lines = numpix(number)
        for line in lines:
            if len(line) > w: w = len(line)

        if self.xpos + w > PictureDump.IM_WIDTH - 10:
            self.xpos = 10
            self.ypos = self.ybottom + 10
            self.ybottom = self.ypos

        if self.ybottom < self.ypos + h:
            self.ybottom = self.ypos + h
            assert self.ybottom < PictureDump.IM_HEIGHT - 10

        for y, line in enumerate(lines):
            y = self.ypos - 6 + y
            x = self.xpos
            for c in line:
                if c != ' ': self.plot(x, y, 1)
                x = x + 1

        offset = offset + 16
        for y in range(h):
            ptr = rcd.uint32(offset + y * 4)
            if ptr == 0: continue
            x = 0
            while True:
                offs  = rcd.uint8(offset + ptr)
                count = rcd.uint8(offset + ptr + 1)
                ptr += 2
                x = x + (offs & 127)
                while count > 0:
                    assert x < w
                    pix = rcd.uint8(offset + ptr)
                    self.plot(self.xpos + x, self.ypos + y, pix)
                    ptr = ptr + 1
                    x = x + 1
                    count = count - 1
                if (offs & 128) != 0: break


        self.xpos = self.xpos + w + 10


def list_blocks(rcd):
    stat = {}
    sz = rcd.get_size()

    pd = PictureDump()

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
    number = 1
    while offset < sz:
        name = rcd.name(offset)
        version = rcd.uint32(offset + 4)
        length = rcd.uint32(offset + 8)
        print "%06X (block %d)" % (offset, number)
        print "    Name: " + repr(name)
        print "    Version: " + str(version)
        print "    Length: " + str(length) + "(+12)"
        if repr(name) in stat:
            stat[repr(name)] = stat[repr(name)] + 1
        else:
            stat[repr(name)] = 1

        if name == "8PXL" and version == 1:
            pd.add_image(rcd, offset, number)

        elif name == 'SPRT' and version == 2:
            print "    SPRT block %d" % number
            print "    SPRT x-offset: " + str(rcd.int16(offset + 12))
            print "    SPRT y-offset: " + str(rcd.int16(offset + 14))
            print "    SPRT 8pxl: " + str(rcd.uint32(offset + 16))

        elif name == 'FUND' and version == 1:
            print "    FUND type: "   + str(rcd.uint16(offset + 12))
            print "    FUND width: "  + str(rcd.uint16(offset + 14))
            print "    FUND height: " + str(rcd.uint16(offset + 16))
            print "    FUND se_e: "   + str(rcd.uint32(offset + 18))
            print "    FUND se_s: "   + str(rcd.uint32(offset + 22))
            print "    FUND se_es: "  + str(rcd.uint32(offset + 26))
            print "    FUND ws_s: "   + str(rcd.uint32(offset + 30))
            print "    FUND ws_w: "   + str(rcd.uint32(offset + 34))
            print "    FUND ws_ws: "  + str(rcd.uint32(offset + 38))

        elif name == "TCOR" and version == 1:
            print "    TCOR width: "  + str(rcd.uint16(offset + 12))
            print "    TCOR height: " + str(rcd.uint16(offset + 14))
            tcor_img('n#', rcd, offset + 16)
            tcor_img('e#', rcd, offset + 16 + 76)
            tcor_img('s#', rcd, offset + 16 + 76 + 76)
            tcor_img('w#', rcd, offset + 16 + 76 + 76 + 76)

        print

        assert length > 0
        offset = offset + 12 + length
        number = number + 1
    assert offset == sz
    pd.save()
    print "Statistics"
    for i in stat:
        print i,stat[i]

if len(sys.argv) == 1:
    print "Missing RCD file argument."
    sys.exit(1)

for f in sys.argv[1:]:
    rcd = RCD(f)
    list_blocks(rcd)
