# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from PIL import Image
from rcdlib import output, blocks
import sys

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


    pix_blk = blocks.Pixels8Bpp(xlength, ylength)
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
        out = output.MemoryOutput()
        tdict[tcol] = encode(data, -32, -33) # xoffset, yoffset, 8PXL block.


file_data = blocks.RCD()

pb = blocks.Palette8Bpp()
pb.set_palette(im)
palb_num = file_data.add_block(pb)

spr = {}
for name, blkdata in tdict.iteritems():
    xoff, yoff, pixblk = blkdata
    print "%4s: width=%d, height=%d, xoff=%d, yoff=%d" % (name, pixblk.width, pixblk.height, xoff, yoff)
    #for idx, d in enumerate(pixblk.line_data):
    #    dump_line(idx, d, pixblk.width)
    #print
    pix_blknum = file_data.add_block(pixblk)
    spr_blk = blocks.Sprite(xoff, yoff, pix_blknum, palb_num)
    spr[name] = file_data.add_block(spr_blk)

surf = blocks.Surface(64, 16)
for name, sprb_num in spr.iteritems():
    surf.add_sprite(name, sprb_num)
file_data.add_block(surf)

file_data.to_file('groundsprites.rcd')

