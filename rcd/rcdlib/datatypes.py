# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
import re
from xml.dom import minidom
from xml.dom.minidom import Node

# {{{ def get_opt_DOMattr(node, name, default):
def get_opt_DOMattr(node, name, default):
    """
    Get an optional value from the DOM node.

    @param node: DOM node being read.
    @type  node: L{xml.dom.minidom.ElementNode}

    @param name: Name of the value.
    @type  name: C{str}

    @param default: Default value as string.
    @type  default: C{unicode}

    @return: The requested value.
    @rtype:  C{unicode}
    """
    if node.hasAttribute(name): return node.getAttribute(name)
    return default

# }}}
# {{{ def collect_text_DOM(node):
def collect_text_DOM(node):
    """
    Collect all text of this node.

    @param node: DOM node being read.
    @type  node: L{xml.dom.minidom.ElementNode}

    @return: The collected text.
    @rtype:  C{unicode}
    """
    return u"".join(n.data for n in node.childNodes if n.nodeType == Node.TEXT_NODE)
# }}}

def get_child_nodes(node, name):
    """
    Get all direct child nodes with a given name.

    @param node: DOM node being read.
    @type  node: L{xml.dom.minidom.ElementNode}

    @param name: Name of the child node.
    @type  name: C{unicode}

    @return: All direct child nodes with the given name.
    @rtype:  C{list} of L{xml.dom.minidom.Node}
    """
    if not node.hasChildNodes(): return []
    result = []
    for n in node.childNodes:
        if n.nodeType != Node.ELEMENT_NODE: continue
        if n.tagName == name:
            result.append(n)
    return result

def get_single_child_node(node, name, optional=False):
    """
    Get the child node with the given name (there should be exactly one).

    @param node: DOM node being read.
    @type  node: L{xml.dom.minidom.ElementNode}

    @param name: Name of the child node.
    @type  name: C{unicode}

    @param optional: Child node may be optional.
    @type  optional: C{bool}

    @return: The unique child node with the given name (or C{None}).
    @rtype:  L{xml.dom.minidom.Node} or C{None}
    """
    result = get_child_nodes(node, name)
    if len(result) == 1: return result[0]
    if optional and len(result) == 0: return None

    print "ERROR: Failed to find precisely one child node named \"" + name + "\" in"
    print node.toxml()
    sys.exit(1)


class DataType(object):
    """
    Data type of a field.

    @ivar name: Name of the data type.
    @type name: C{unicode}
    """
    def __init__(self, name):
        self.name = name

    def get_size(self):
        """
        Retrieve the size of the data type.

        @return: Size of the data type in the RCD file (in bytes).
        @rtype:  C{int}
        """
        raise NotImplementedError("Implement me in %r" % type(self))

    def write(self, out, value):
        """
        Write the value onto the output.

        @param out: Output stream.
        @type  out: L{Output}

        @param value: Value to write.
        @type  value: Varies with each type
        """
        raise NotImplementedError("Implement me in %r" % type(self))


num_pat = re.compile(u'\\d+$')


class NumericDataType(DataType):
    """
    Simple numeric data type.
    """
    SIZES = {'int8': 1, 'uint8': 1, 'int16': 2, 'uint16': 2, 'int32': 4, 'uint32': 4,}
    _writer = {  'int8' : lambda out, value: out.int8(value),
                'uint8' : lambda out, value: out.uint8(value),
                 'int16': lambda out, value: out.int16(value),
                'uint16': lambda out, value: out.uint16(value),
                'uint32': lambda out, value: out.uint32(value), }

    def __init__(self, name):
        DataType.__init__(self, name)

    def get_size(self, value):
        return self.SIZES[self.name]

    def convert(self, value):
        """
        Convert textual value to numeric value (which can be written to the output).

        @param value: Textual value
        @type  value: C{unicode}

        @return: Numeric value, ready for writing, if syntax is ok (else C{None}).
        @rtype:  C{int} or C{None}

        @todo: Performing conversion as part of the data type seems more generic than just with numeric types.
        """
        m = num_pat.match(value)
        if m:
            return int(value)
        return None

    def write(self, out, value):
        self._writer[self.name](out, value)

class EnumerationDataType(DataType):
    """
    Data type of an enumeration. Thin layer on top of a normal numeric data type.

    @ivar values: Mapping of names to values.
    @type values: C{dict} of C{unicode} to C{int}

    @ivar data_type: Underlying data type.
    @type data_type: L{DataType}
    """
    def __init__(self, name, values, data_type):
        """
        Construct a L{EnumerationDataType}.

        @param name: Name of the enumeration.
        @type  name: C{unicode}

        @param values: Mapping of names to values.
        @type  values: C{dict} of C{unicode} to C{unicode}

        @param data_type: Name of the underlying data type.
        @type  data_type: L{DataType}
        """
        DataType.__init__(self, name)
        self.data_type = data_type
        self.values = {}
        for n,v in values.iteritems():
            self.values[n] = self.data_type.convert(v)

    def get_size(self, value):
        value = self.values.get(value, value)
        return self.data_type.get_size(value)

    def convert(self, value):
        v = self.values.get(value)
        if v is not None:
            return v
        return self.data_type.convert(value)

    def write(self, out, value):
        self.data_type.write(out, value)

class FrameDefinitions(DataType):
    """
    Data type of a sequence of frame definitions.
    """
    def __init__(self):
        DataType.__init__(self, 'frame_defs')

    def get_size(self, value):
        return 2 + 6 * len(value)

    def write(self, out, value):
        out.uint16(len(value))
        for fr_data in value:
            out.uint16(fr_data.duration)
            out.int16(fr_data.game_dx)
            out.int16(fr_data.game_dy)

class ListType(DataType):
    """
    Generic 'list' type.

    @ivar elm_type: Element type.
    @type elm_type: L{DataType}

    @ivar count_type: Storage type of the list count.
    @type count_type: L{DataType}
    """
    def __init__(self, elm_type, count_type):
        DataType.__init__(self, u"_list_")
        self.elm_type = elm_type
        self.count_type = count_type

    def get_size(self, value):
        total = self.count_type.get_size(len(value))
        for v in value:
            total = total + self.elm_type.get_size(v)
        return total

    def write(self, out, value):
        self.count_type.write(out, len(value))
        for v in value:
            self.elm_type.write(out, v)

class BlockReference(DataType):
    """
    Data type of a block reference.
    """
    def __init__(self, name):
        DataType.__init__(self, name)

    def get_size(self, value):
        return 4

    def write(self, out, value):
        if value is None:
            value = 0
        out.uint32(value)


class ImageDataType(DataType):
    """
    Data type of image data.
    """
    def __init__(self):
        DataType.__init__(self, 'image_data')

    def get_size(self, line_data):
        """
        Compute the size (in bytes) for the pixel data.

        @param line_data: Data of each line, C{None} means the line is empty.
        @type  line_data: C{list} of (C{str} or C{None})

        @return: The size of the data field in the rcd file.
        @rtype:  C{int}
        """
        total = 0
        for line in line_data:
            total = total + 4
            if line is not None:
                total = total + len(line)
        return total

    def write(self, out, line_data):
        """
        Write the image data.

        @param out: Output stream.
        @type  out: L{Output}

        @param line_data: Data of each line, C{None} means the line is empty.
        @type  line_data: C{list} of (C{str} or C{None})
        """
        offset = 4 * len(line_data)
        for line in line_data:
            if line is not None:
                out.uint32(offset)
                offset = offset + len(line)
            else:
                out.uint32(0)
        for line in line_data:
            if line is not None:
                out.store_text(line)

_types = { 'uint8':  NumericDataType('uint8'),
           'int8':   NumericDataType('int8'),
           'uint16': NumericDataType('uint16'),
           'int16':  NumericDataType('int16'),
           'uint32': NumericDataType('uint32'),
           'int32':  NumericDataType('int32'),
           'sprite': BlockReference('sprite'),
           'block':  BlockReference('block'),
           'image_data': ImageDataType(),
           'frame_defs': FrameDefinitions(),
         }

UINT16_TYPE = _types['uint16']
INT16_TYPE  = _types['int16']
IMAGE_DATA_TYPE = _types['image_data']


def load_enum_definition(node):
    """
    Add an 'enum' definition from the xml node to the data types.

    @param node: XML node pointing to an enum definition.
    @type  node: L{xml.dom.minidom.Node}
    """
    name = node.getAttribute("name")
    storage_type = _types[node.getAttribute("type")]
    values = {}
    val_nodes = get_child_nodes(node, u"value")
    for vn in val_nodes:
        val_name  = vn.getAttribute("name")
        val_value = vn.getAttribute("value")
        assert val_name not in values
        values[val_name] = val_value

    _types[name] = EnumerationDataType(name, values, storage_type)


def read_type(node, blk_name, fld_name):
    """
    Read the 'type' XML child node, and load it.

    @param node: XML node pointing to node with a C{<type ..} child node.
    @type  node: L{xml.dom.minidom.Node}

    @param blk_name: Name of the block being loaded.
    @type  blk_name: C{unicode}

    @param fld_name: Name of the field being loaded.
    @type  fld_name: C{unicode}

    @return: Loaded data type.
    @rtype:  L{DataType}
    """
    type_node = get_single_child_node(node, u"type")
    name = type_node.getAttribute(u"name")
    if name == 'list':
        count_type = _types[type_node.getAttribute(u"count")]
        elm_type = read_type(type_node, blk_name + "/" + fld_name, '_list_')
        return ListType(elm_type, count_type)
    return _types[name]

