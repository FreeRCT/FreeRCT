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

def get_game_blocks(rcdfile_node):
    """
    Pull the file information from the node, as well as the game blocks to generate.

    @param rcdfile_node: XML node representing a file to generate.
    @type  rcdfile_node: L{xml.dom.minidom.Node} (a 'file' node)

    @return: Loaded information about the file.
    @rtype:  L{data_loader.RcdFile}
    """
    rf = data_loader.RcdFile()
    rf.loadfromDOM(rcdfile_node)
    return rf

def order_nodes(flddefs, named_nodes, blk_magic):
    """
    Order the nodes in the data file to match the order in the definition.

    @param flddefs: Fields needed according to the definition.
    @type  flddefs: C{list} of L{Field}

    @param named_nodes: Fields found in the game data file.
    @type  named_nodes: C{list} of L{NamedNode}

    @param blk_magic: Name of the block (for error output).
    @type  blk_magic: C{unicode}

    @return: Game data file nodes ordered by the field definition.
    @rtype:  C{list} of (L{Field}, L{Name}, L{NamedNode}) triplets.
    """
    gd_fields = {} # Mapping of C{unicode} to (L{Name}, L{NamedNode})
    for nn in named_nodes:
        for name in nn.names:
            text = name.name
            if text in gd_fields:
                print "ERROR: \"%s\" already defined in block \"%s\"." % (text, blk_magic)
                sys.exit(1)
            gd_fields[text] = (name, nn)

    fields = []
    for flddef in flddefs:
        if flddef.name not in gd_fields:
            print "ERROR: Field \"%s\" is missing in block \"%s\"." % (flddef.name, blk_magic)
            sys.exit(1)
        gd_name, gd_node = gd_fields[flddef.name]
        fields.append((flddef, gd_name, gd_node))
        del gd_fields[flddef.name]

    if len(gd_fields) > 0:
        for gdt in gd_fields.iterkeys():
            if gdt.startswith('_'):
                continue # Silently ignore double fields starting with _

            print "WARNING: Field \"%s\" not used in block \"%s\"." % (gdt, blk_magic)

    return fields

def sort_nodes(named_nodes):
    """
    Sort nodes on their name.

    @param named_nodes: Fields found in the game data file.
    @type  named_nodes: C{list} of L{NamedNode}

    @return: Game data file nodes ordered by their name.
    @rtype:  C{list} of (L{Name}, L{NamedNode}) tuples
    """
    sorted_nodes = []
    unique = set()
    doubles = set()
    for nn in named_nodes:
        for n in nn.names:
            sorted_nodes.append((n, nn))
            if n not in unique:
                unique.add(n)
            elif n not in doubles:
                print "WARNING: Node named \"%s\" added more than once." % n.name
                doubles.add(n)
    sorted_nodes.sort()
    return sorted_nodes

def make_sprite_block_reference(name, named_node, file_blocks):
    """
    Construct a block reference to a sprite block.

    @param name: Name inside the node to create.
    @type  name: L{Name}

    @param named_node: XML node describing the block.
    @type  named_node: L{NamedNode}

    @param file_blocks: Blocks already generated.
    @type  file_blocks: L{blocks.RCD}

    @return: Block number of the created block.
    @rtype:  C{int}
    """
    node = named_node.node
    if named_node.tag == 'image':
        x_base   = int(datatypes.get_opt_DOMattr(node, 'x-base', u"0"))
        y_base   = int(datatypes.get_opt_DOMattr(node, 'y-base', u"0"))
        width    = int(node.getAttribute("width"))
        height   = int(node.getAttribute("height"))
        x_offset = int(datatypes.get_opt_DOMattr(node, "x-offset", u"0"))
        y_offset = int(datatypes.get_opt_DOMattr(node, "y-offset", u"0"))
        fname    = node.getAttribute("fname")
        transp   = int(datatypes.get_opt_DOMattr(node, 'transparent', u"0"))

    elif named_node.tag == 'sheet':
        x_base   = int(node.getAttribute("x-base"))
        y_base   = int(node.getAttribute("y-base"))
        x_step   = int(node.getAttribute("x-step"))
        y_step   = int(node.getAttribute("y-step"))
        fname    = node.getAttribute("fname")
        names    = node.getAttribute("names")
        x_offset = int(node.getAttribute("x-offset"))
        y_offset = int(node.getAttribute("y-offset"))
        width    = int(node.getAttribute("width"))
        height   = int(node.getAttribute("height"))
        transp   = int(datatypes.get_opt_DOMattr(node, 'transparent', u"0"))

        x_base = x_base + x_step * name.data[0]
        y_base = y_base + y_step * name.data[1]

    else:
        print "ERROR: Unknown type of data for a sprite block %r" % named_node.tag
        sys.exit(1)

    im = spritegrid.image_loader.get_img(fname)
    im_obj = spritegrid.ImageObject(im, x_offset, y_offset, x_base, y_base, width, height)
    pxl_blk = im_obj.make_8PXL()
    return file_blocks.add_block(pxl_blk)

def convert_node(node, name, data_type, file_blocks):
    """
    Convert the XML data node to its value.

    @param node: The XML node of the data value/field.
    @type  node: L{xml.dim.minidom.Node}

    @param name: Name object (for sheets of images), if available.
    @type  name: L{Name} or C{None}

    @param data_type: Data type.
    @type  data_type: L{DataType}

    @param file_blocks: Blocks already generated.
    @type  file_blocks: L{blocks.RCD}

    @return: Converted data value.
    @rtype:  Varies
    """
    if isinstance(data_type, datatypes.NumericDataType):
        value = node.node.getAttribute(u'value')
        val = data_type.convert(value)
        if val is None:
            print "ERROR: %r is not a number" % value
            sys.exit(1)
        return val

    if isinstance(data_type, datatypes.BlockReference):
        if data_type.name == 'sprite':
            blknum = make_sprite_block_reference(name, node, file_blocks)
            return blknum

        print "ERROR: Unknown type of block reference %r" % data_type.name
        sys.exit(1)

    if isinstance(data_type, datatypes.EnumerationDataType):
        value = node.node.getAttribute(u'value')
        val = data_type.convert(value)
        if val is None:
            print "ERROR: %r is not an enumeration value" % value
            sys.exit(1)
        return val

    if isinstance(data_type, datatypes.ListType):
        named_nodes = data_loader.load_named_nodes(node.node)
        named_tuples = sort_nodes(named_nodes)
        result = []
        for n, nn in named_tuples:
            result.append(convert_node(nn, n, data_type.elm_type, file_blocks))
        return result

    if isinstance(data_type, datatypes.StructureDataType):
        result = []
        named_nodes = data_loader.load_named_nodes(node.node)
        for flddef, name, node in order_nodes(data_type.fields, named_nodes, data_type.name):
            res = convert_node(node, name, flddef.type, file_blocks)
            result.append(res)
        return result

    print "ERROR: Unknown field definition: %r" % data_type
    sys.exit(1)


def convert(def_fname, data_fname):
    """
    Convert game data to RCD file format.

    @param def_fname: Filename of the structure definitions.
    @type  def_fname: C{str}

    @param data_fname: Filename of the game data file.
    @type  data_fname: C{str}

    @todo: Actually use L{def_fname}.
    """
    struct_defs = blocks.block_factory.struct_def

    data_file_nodes = get_rcdfile_nodes(data_fname)
    for dfile_node in data_file_nodes:
        dfile = get_game_blocks(dfile_node)

        if dfile.magic != 'RCDF' or dfile.version != 1:
            raise ValueError("Output file %r has wrong magic or version number" % dfile.dest_fname)

        file_blocks = blocks.RCD()
        print "Target file: " + dfile.target

        for block_node in dfile.block_nodes:
            blk_magic = block_node.getAttribute(u"magic")
            blk_version = int(block_node.getAttribute(u"version"))

            blockdef = struct_defs.get_block(blk_magic)
            if blockdef is None:
                raise ValueError("Data block %r does not exist in the definitions" % blk_magic)

            flddefs = blockdef.get_fields(blk_version)
            if flddefs is None:
                raise ValueError("Version %r of data block %r is not supported "
                                 "in the definitions" % (blk_version, blk_magic))

            blockdata = {}
            fields = []

            named_nodes = data_loader.load_named_nodes(block_node)
            for flddef, name, node in order_nodes(flddefs, named_nodes, blk_magic):
                fields.append((flddef.name, flddef.type))

                blockdata[flddef.name] = convert_node(node, name, flddef.type, file_blocks)

            dblk = blocks.GeneralDataBlock(blk_magic, blk_version, fields, None)
            dblk.set_values(blockdata)
            file_blocks.add_block(dblk)


        file_blocks.to_file(dfile.target)

if __name__ == '__main__':
    main()
