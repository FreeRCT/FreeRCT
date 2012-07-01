#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from xml.dom import minidom
from xml.dom.minidom import Node

from rcdlib import spritegrid, structdef_loader, data_loader, blocks, datatypes
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

class FrameData(object):
    """
    Data of a frame in the RCD file.

    @ivar duration: Duration of the frame in milli seconds.
    @type duration: C{int}

    @ivar game_dx: Signed movement in X direction after displaying the frame.
    @type game_dx: C{int}

    @ivar game_dy: Signed movement in Y direction after displaying the frame.
    @type game_dy: C{int}
    """
    def __init__(self):
        self.duration = None
        self.game_dx = None
        self.game_dy = None

    def setup(self, data):
        """
        Initialize the frame.

        @param data: The L{RcdFrame} to load the data from.
        @type  data: L{RcdFrame}
        """
        self.duration = data.duration
        self.game_dx = data.game_dx
        self.game_dy = data.game_dy

def get_rcdfile_nodes(data_fname):
    """
    Extract the XML nodes representing the output file to generate.

    @param data_fname: Filename of the file containing the game data.
    @type  data_fname: C{str}

    @return: Nodes representing the files to generate.
    @rtype:  C{list} of L{xml.dom.minidom.Node}
    """
    dom = minidom.parse(data_fname)
    root = datatypes.get_single_child_node(dom, u"rcdfiles")
    return datatypes.get_child_nodes(root, u'file')

def convert(def_fname, data_fname):
    struct_defs = blocks.block_factory.struct_def

    data_file_nodes = get_rcdfile_nodes(data_fname)
    for dfile_node in data_file_nodes:
        dfile = data_loader.RcdFile()
        dfile.loadfromDOM(dfile_node)

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
            fields = []
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
                    value = flddef.type.convert(data.value)
                    if value is None:
                        raise ValueError("Value %r of field %r in data block %r is incorrect" %
                                         (data.value, flddef.name, block.magic))
                    blockdata[data.name] = value

                elif isinstance(data, data_loader.RcdFrameDefinitions):
                    assert flddef.type.name == 'frame_defs'
                    frames = []
                    for fr in data.frames:
                        new_fr = FrameData()
                        new_fr.setup(fr)
                        frames.append(new_fr)
                    blockdata[data.name] = frames

                elif isinstance(data, data_loader.RcdFrameImages):
                    assert flddef.type.name == 'frame_images'
                    imgs = []
                    for img in data.images:
                        im = spritegrid.image_loader.get_img(img.fname)
                        im_obj = spritegrid.ImageObject(im, img.x_offset, img.y_offset,
                                                    img.x_base, img.y_base, img.width, img.height)
                        pxl_blk = im_obj.make_8PXL()
                        imgs.append(file_blocks.add_block(pxl_blk))
                    blockdata[data.name] = imgs

                else:
                    assert flddef.type.name == 'sprite'
                    im = spritegrid.image_loader.get_img(data.fname)
                    im_obj = spritegrid.ImageObject(im, data.x_offset, data.y_offset,
                                                    data.x_base, data.y_base, data.width, data.height)
                    pxl_blk = im_obj.make_8PXL()
                    blockdata[data.name] = file_blocks.add_block(pxl_blk)

                fields.append((flddef.name, flddef.type))

            dblk = blocks.GeneralDataBlock(block.magic, block.version, fields, None)
            dblk.set_values(blockdata)
            file_blocks.add_block(dblk)

            if len(avail) > 0:
                for nm in avail:
                    if not nm.startswith('_'):
                        print "Warning: field %r is not used in data block %r" % (nm, block.magic)


        file_blocks.to_file(dfile.target)

if __name__ == '__main__':
    main()
