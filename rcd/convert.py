#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, structdef_loader, data_loader, blocks
import sys, getopt

def usage():
    print "convert.py [options]"
    print "with options:"
    print "    -h, --help                Online help."
    print "    -d DEF, --definition=DEF  Use DEF file as definition file."
    print "    -g DAT, --game-data=DAT   Use DAT file as game data file."
    print
    print "You must provide both a definition file and a game data file."

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:g:", ["help", "definition=", "game-data="])
    except getopt.GetoptError, err:
        print str(err)
        usage()
        sys.exit(2)
    def_fname = None
    data_fname = None
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit(0)
        elif o in ("-d", "--definition"):
            def_fname = a
        elif o in ("-g", "--game-data"):
            data_fname = a
        else:
            raise ValueError("Unrecognized option encountered")

    if def_fname is None:
        raise ValueError("Missing definition file name")

    if data_fname is None:
        raise ValueError("Missing game data file name")

    convert(def_fname, data_fname)
    sys.exit(0)


def convert(def_fname, data_fname):
    struct_defs = structdef_loader.loadfromDOM(def_fname)

    data_files = data_loader.loadfromDOM(data_fname)
    for dfile in data_files.files:
        if dfile.magic != 'RCDF' or dfile.version != 1:
            raise ValueError("Output file %r has wrong magic or version number" % dfile.dest_fname)

        file_blocks = blocks.RCD()
        print "Target file: " + dfile.target

        for block in dfile.blocks:
            blockdef = struct_defs.get_block(block.magic)
            if blockdef is None:
                raise ValueError("Data block %r does not exist in the definitions" % block.magic)

            flddefs = blockdef.get_fields(block.version)
            if flddefs is None:
                raise ValueError("Version %r of data block %r is not supported "
                                 "in the definitions" % (block.version, block.magic))

            avail = set(block.fields.iterkeys())
            seen = set()
            blockdata = {}
            for flddef in flddefs:
                if flddef.name not in avail:
                    raise ValueError("Field %r is missing in data block %r" % (flddef.name, block.magic))
                avail.remove(flddef.name)
                data = block.fields[flddef.name]
                data.field_def = flddef

                # Output double entries
                if flddef.name in seen:
                    print "Warning: field %r defined more than once in data block %r" % (flddef.name, block.magic)
                seen.add(flddef.name)


                if isinstance(data, data_loader.RcdDataField):
                    assert data.field_def.type in ('int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32')
                    blockdata[data.name] = int(data.value)
                else:
                    assert data.field_def.type == 'sprite'
                    im = spritegrid.image_loader.get_img(data.fname)
                    im_obj = spritegrid.ImageObject(im, data.x_offset, data.y_offset, data.x_base, data.y_base, data.width, data.height)
                    pxl_blk = im_obj.make_8PXL()
                    if pxl_blk is not None:
                        pix_blknum = file_blocks.add_block(pxl_blk)
                        spr_blk = blocks.Sprite(im_obj.xoffset, im_obj.yoffset, pix_blknum)
                        blockdata[data.name] = file_blocks.add_block(spr_blk)
                    else:
                        blockdata[data.name] = 0

            fields = [(fdef.name, fdef.type) for fdef  in flddefs]
            dblk = blocks.GeneralDataBlock(block.magic, block.version, fields, None)
            dblk.set_values(blockdata)
            file_blocks.add_block(dblk)

            if len(avail) > 0:
                for nm in avail:
                    print "Warning: field %r is not used in data block %r" % (nm, block.magic)


        file_blocks.to_file(dfile.target)

if __name__ == '__main__':
    main()
