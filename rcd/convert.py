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

def order_nodes(flddefs, named_nodes, blk_magic, allow_missing):
    """
    Order the nodes in the data file to match the order in the definition.

    @param flddefs: Fields needed according to the definition.
    @type  flddefs: C{list} of L{Field}

    @param named_nodes: Fields found in the game data file.
    @type  named_nodes: C{list} of L{NamedNode}

    @param blk_magic: Name of the block (for error output).
    @type  blk_magic: C{unicode}

    @param allow_missing: Allow for missing data fields (replace with None).
    @type  allow_missing: C{bool}

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
        if flddef.name in gd_fields:
            gd_name, gd_node = gd_fields[flddef.name]
        elif allow_missing:
            gd_name, gd_node = flddef.name, None
        else:
            print "ERROR: Field \"%s\" is missing in block \"%s\"." % (flddef.name, blk_magic)
            sys.exit(1)
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
        fname = datatypes.get_opt_DOMattr(node, "fname", None)
        if fname is None:
            # No file name -> 0 block reference
            return 0

        x_base   = int(datatypes.get_opt_DOMattr(node, 'x-base', u"0"))
        y_base   = int(datatypes.get_opt_DOMattr(node, 'y-base', u"0"))
        width    = int(node.getAttribute("width"))
        height   = int(node.getAttribute("height"))
        x_offset = int(datatypes.get_opt_DOMattr(node, "x-offset", u"0"))
        y_offset = int(datatypes.get_opt_DOMattr(node, "y-offset", u"0"))
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

RES_ROTATED = {'e':'s', 's':'w', 'w':'n', 'n':'e'}

def rotate_voxel(v, rot_count):
    """
    Construct a rotated voxel (a new one in case rotations were performed, otherwise L{v} may be returned).

    @param v: Original voxel.
    @type  v: L{TrackVoxel}

    @param rot_count: Number of 90 degrees rotations.
    @type  rot_count: C{int}

    @return: Rotated voxel.
    @rtype:  L{TrackVoxel}
    """
    assert rot_count >= 0
    rot_count = rot_count % 4
    if rot_count == 0: return v

    x, y = v.pos[0], v.pos[1]
    res = v.reserved

    cnt = 0
    while cnt < rot_count:
        res = ''.join(RES_ROTATED[part] for part in res)
        x, y = y, -x

        cnt = cnt + 1

    return TrackVoxel(v.graphics, (x, y, v.pos[2]), res)

def rotate_connection(conn, rot_count):
    """
    Rotate the connection to differentiate between directions of connection.

    @param conn: Connection to rotate.
    @type  conn: C{tuple} (C{int}, C{int})

    @param rot_count: Number of 90 degrees rotations.
    @type  rot_count: C{int}

    @return: Rotated connection.
    @rtype:  C{tuple} (C{int}, C{int})
    """
    return (conn[0], (conn[1] + rot_count) % 4)


DIRECTIONS = {'ne':0, 'se':1, 'sw':2, 'nw':3}

def make_trackpiece_block_references(tpiece, file_blocks):
    """
    Generate 8PXL and TRCK blocks for this track piece in every orientation.

    @param tpiece: Track piece to output.
    @type  tpiece: L{TrackPiece}

    @param file_blocks: Blocks already generated.
    @type  file_blocks: L{blocks.RCD}

    @return: Block numbers of TRCK blocks generated for this piece.
    @rtype:  C{list} of C{int}
    """
    results = []
    # A single track-piece defines 4 directions, so rotate the piece.
    for dir_name in DIRECTIONS.iterkeys():
        # Generate the voxels
        voxdata = []
        for v in tpiece.voxels:
            v = rotate_voxel(v, DIRECTIONS[dir_name])
            nn, n = v.graphics[DIRECTIONS[dir_name]]
            gblk = make_sprite_block_reference(n, nn, file_blocks)
            voxdata.append((gblk, v.pos, v.reserved))
        entry = rotate_connection(tpiece.entry, DIRECTIONS[dir_name])
        exit  = rotate_connection(tpiece.exit,  DIRECTIONS[dir_name])
        blk = blocks.TrackPieceBlock(entry, exit, voxdata)
        results.append(file_blocks.add_block(blk))

    return results

KNOWN_LANGUAGES = set(["", "nl_NL", "en_GB"])

def encode_translation(lang, tr_text):
    """
    Encode a translation of a string for a language.

    @param lang: Language being used.
    @type  lang: C{uniocde}

    @param tr_text: Translation text.
    @type  tr_text: C{unicode}

    @return: Encoded translation.
    @rtype:  C{str}
    """
    enc_lang = lang.encode('utf-8') + '\0'
    enc_text = tr_text.encode('utf-8') + '\0'

    enc = chr(len(enc_lang)) + enc_lang + enc_text
    v = len(enc) + 2
    assert v <= 0xffff
    return chr(v & 0xff) + chr(v >> 8) + enc

def saveTEXT(text_data, file_blocks):
    """
    Construct a TEXT block, save it to the file blocks, and return the assigned
    block number,

    @param text_data: Text data of the block, sequence of tuples (string name,
                      list of languages and translations)
    @type  text_data: C{list} of (C{unicode}, C{list} of (C{unicode}, C{unicode}))

    @param file_blocks: Blocks already generated.
    @type  file_blocks: L{blocks.RCD}

    @return: Assigned block number.
    @rtype:  C{int}
    """
    strings = []
    for name, trs in text_data:
        enc_name = name.encode('utf-8') + '\0'
        enc_name = chr(len(enc_name)) + enc_name
        enc_trs = "".join(encode_translation(lang, tr_text) for lang, tr_text in trs)
        enc_str = enc_name + enc_trs
        v = len(enc_str) + 2
        assert v <= 0xffff
        strings.append(chr(v & 0xff) + chr(v >> 8) + enc_str)

    strings = "".join(strings)
    text_blk = blocks.TextBlock(strings)
    return file_blocks.add_block(text_blk)

def sort_translations(tr_texts):
    """
    Sort the translations, with the default language at the end.

    @param tr_texts: Translated texts, mapping of language to text-strings.
    @type  tr_texts: C{dict} of C{unicode} to C{unicode}

    @return: Translations alphabetically sorted, with the default language at the end.
    @rtype:  C{list} of (C{unicode}, C{unicode}) tuples
    """
    texts = tr_texts.items()
    texts.sort()
    assert len(texts) > 0
    assert texts[0][0] == u""
    return texts[1:] + [texts[0]]

def collect_language_strings(str_names, node, is_texts, texts, lang):
    """
    Collect language strings from the node (either a 'texts' or a 'language' node).

    @param str_names: Expected string names (for printing warnings).
    @type  str_names: C{list} of C{unicode}

    @param node: The XML node of the data value/field.
    @type  node: L{xml.dim.minidom.Node}

    @param is_texts: This node is a 'texts' node.
    @type  is_texts: C{bool}

    @param texts: Collected strings so far, mapping of string names to
                  (mapping of language to actual text).
    @type  texts: C{dict} of C{unicode} to C{dict} of C{unicode} to C{unicode}

    @param lang: Language of the strings (should be a known language).
    @type  lang: C{unicode}
    """
    if not is_texts:
        str_lang = datatypes.get_opt_DOMattr(node, 'lang', None)
        if str_lang is not None:
            if str_lang not in KNOWN_LANGUAGES:
                print "WARNING: Language \"%s\" is unknown" % str_lang
                KNOWN_LANGUAGES.add(str_lang) # Just one warning is sufficient.
            lang = str_lang

    if not is_texts:
        file_attr = datatypes.get_opt_DOMattr(node, 'file', None)
        if file_attr is not None:
            # Load the xml file, process it, and return.
            dom = minidom.parse(file_attr)
            root = datatypes.get_single_child_node(dom, u"language")
            collect_language_strings(str_names, root, False, texts, lang)
            return

    # Process sub-'language' nodes.
    for lang_node in datatypes.get_child_nodes(node, u'language'):
        collect_language_strings(str_names, lang_node, False, texts, lang)

    # Process strings.
    for str_node in datatypes.get_child_nodes(node, u'string'):
        str_name = str_node.getAttribute(u'name')
        str_text = datatypes.collect_text_DOM(str_node)
        str_lang = datatypes.get_opt_DOMattr(str_node, u'lang', None)
        if str_lang is None:
            str_lang = lang
        else:
            if str_lang not in KNOWN_LANGUAGES:
                print "WARNING: Language \"%s\" is unknown" % str_lang
                KNOWN_LANGUAGES.add(str_lang) # Just one warning is sufficient.

        if str_name not in str_names:
            print "WARNING: String name \"%s\" not expected" % str_name
            str_names.add(str_name)

        trans = texts.get(str_name)
        if trans is None:
            trans = {}
            texts[str_name] = trans

        trans[str_lang] = str_text

    # Done!

def make_texts(named_node, str_names, file_blocks):
    """
    Make a 'texts' block, and return its block number.

    @param named_node: The named node of the data value/field.
    @type  named_node: L{NamedNode}

    @param str_names: Expected string names (for printing warnings).
    @type  str_names: C{list} of C{unicode}

    @param file_blocks: Blocks already generated.
    @type  file_blocks: L{blocks.RCD}

    @return: Block number of the created block.
    @rtype:  C{int}
    """
    str_names = set(str_names)

    texts = {} # Mapping of string names to (mapping of languages to actual text)
    collect_language_strings(str_names, named_node.node, True, texts, u"")

    for name in str_names:
        if name not in texts:
            print "WARNING: Expected name \"%s\" is not defined" % name
            texts[name] = {u"" : u""}

        trans = texts[name]
        if u"" not in trans:
            print "WARNING: Name \"%s\" is missing the default language entry" % name
            trans[u""] = u""

    str_names = sorted(str_names)
    result = [(name, sort_translations(texts[name])) for name in str_names]

    return saveTEXT(result, file_blocks)

class TrackVoxel(object):
    """
    Definition of a voxel in a track piece.

    @ivar graphics: Graphics to load.
    @type graphics: C{list} of L{NamedNodes} for the images.

    @ivar pos: Relative position of the voxel.
    @type pos: C{tuple} (C{int}, C{int}, C{int})

    @ivar reserved: Parts reserved for the tracks ('nesw' letters).
    @type reserved: C{str}
    """
    def __init__(self, graphics, pos, reserved):
        self.graphics = graphics
        self.pos = pos
        self.reserved = reserved


class TrackPiece(object):
    """
    Definition of a track piece.

    @ivar entry: Entry connection.
    @type entry: C{tuple} (C{int}, C{int})

    @ivar exit: Exit connection.
    @type exit: C{tuple} (C{int}, C{int})

    @ivar exit_pos: Relative position of the exit voxel.
    @type exit_pos: C{tuple} (C{int}, C{int}, C{int})

    @ivar voxels: Voxels of the track piece.
    @type voxels: C{list} of L{TrackVoxel}

    @ivar speed: Speed of powered track.
    @type speed: C{None} or C{int}
    """
    def __init__(self):
        self.entry = (63,3)
        self.exit = (63,3)
        self.exit_pos = (0, 0, 0)
        self.voxels = []
        self.speed = None


def convert_connection(node, prefixes):
    prefix = node.getAttribute(u'prefix')
    direction = node.getAttribute(u'direction')
    number = prefixes.get(prefix)
    if number is None:
        number = len(prefixes)
        prefixes[prefix] = number
    return (number, DIRECTIONS[direction])

def convert_position(node):
    xpos = int(node.getAttribute(u'dx'), 10)
    ypos = int(node.getAttribute(u'dy'), 10)
    zpos = int(node.getAttribute(u'dz'), 10)
    return (xpos, ypos, zpos)

def convert_trackpiece(node, prefixes):
    """
    Convert a trackpiece node.

    @param node: The XML node of the data value/field.
    @type  node: L{xml.dim.minidom.Node}

    @param prefixes: Used connection prefixes (and their number).
    @type  prefixes: C{map} of C{str} to C{int}

    @return: A track piece description.
    @rtype:  L{TrackPiece}
    """
    pdata = TrackPiece()
    pdata.entry = convert_connection(datatypes.get_single_child_node(node, u'entry'), prefixes)
    exit_node = datatypes.get_single_child_node(node, u'exit')
    pdata.exit = convert_connection(exit_node, prefixes)
    pdata.exit_pos = convert_position(exit_node)
    for vnode in datatypes.get_child_nodes(node, u'voxel'):
        vpos = convert_position(vnode)
        reserved = datatypes.get_opt_DOMattr(vnode, u'reserved', 'nesw')

        snames = [None, None, None, None]
        for snode in datatypes.get_child_nodes(node, 'sheet'):
            sname = data_loader.load_sheet_node(snode)
            for sn in sname.names:
                snames[DIRECTIONS[sn.name]] = (sname, sn)
        for snode in datatypes.get_child_nodes(node, 'image'):
            sname = data_loader.load_image_node(snode)
            for sn in sname.names:
                snames[DIRECTIONS[sn.name]] = (sname, sn)

        vdata = TrackVoxel(snames, vpos, reserved)
        pdata.voxels.append(vdata)

    powered = datatypes.get_single_child_node(node, u'powered', True)
    if powered is not None:
        pdata.speed = int(powered.getAttribute(u'speed'), 10)

    return pdata

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

        if data_type.name == 'TextType':
            blknum = make_texts(node, data_type.str_names, file_blocks)
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

    if isinstance(data_type, datatypes.TrackPiecesDataType):
        named_nodes = data_loader.load_named_nodes(node.node)
        prefixes = {}
        track_pieces = []
        for nn in named_nodes:
            tp = convert_trackpiece(nn.node, prefixes)
            blknums = make_trackpiece_block_references(tp, file_blocks)
            track_pieces.extend(blknums)
        return track_pieces


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
        for flddef, name, node in order_nodes(data_type.fields, named_nodes, data_type.name, False):
            res = convert_node(node, name, flddef.type, file_blocks)
            result.append(res)
        return result

    if isinstance(data_type, datatypes.BitSet):
        named_nodes = data_loader.load_named_nodes(node.node)
        value = 0
        for flddef, name, node in order_nodes(data_type.bitfields, named_nodes, data_type.name, True):
            if node is None:
                if not flddef.may_be_omitted():
                    print "ERROR: Missing field \"%s\" in bit-set \"%s\"" % (name, data_type.name)
                    sys.exit(1)
            else:
                res = convert_node(node, name, flddef, file_blocks)
                value = value | res
        return value

    if isinstance(data_type, datatypes.BitField):
        count = 0
        value = 0
        for n in datatypes.get_child_nodes(node.node, u'field'):
            nn = data_loader.NamedNode(n, n.tagName, data_type.name)
            res = convert_node(nn, name, data_type.src_type, file_blocks)
            if data_type.as_bit_index:
                res = 1 << res;
            res = res << data_type.start_bit
            count = count + 1
            value = value | res
        if count < data_type.min_count:
            print "ERROR: Expected at least %d values in the bitfield \"%s\", found %d" % (data_type.min_count, node.name, count)
            sys.exit(1)
        if data_type.max_count is not None and count > data_type.max_count:
            print "ERROR: Expected at most %d values in the bitfield \"%s\", found %d" % (data_type.min_count, node.name, count)
            sys.exit(1)
        return value

    print "ERROR: Unknown field definition: %r" % data_type
    print node.node.toxml()
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
    struct_defs = structdef_loader.loadfromDOM('structdef.xml')

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
            for flddef, name, node in order_nodes(flddefs, named_nodes, blk_magic, False):
                fields.append((flddef.name, flddef.type))

                blockdata[flddef.name] = convert_node(node, name, flddef.type, file_blocks)

            dblk = blocks.GeneralDataBlock(blk_magic, blk_version, fields, None)
            dblk.set_values(blockdata)
            file_blocks.add_block(dblk)


        file_blocks.to_file(dfile.target)

if __name__ == '__main__':
    main()
