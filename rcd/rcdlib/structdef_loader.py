# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
"""
Data structure definition of rcd files.
"""
from rcdlib import datatypes

from xml.dom import minidom
from xml.dom.minidom import Node

class Magic(object):
    """
    Magic data.

    @ivar magic: Magic string (4 letters).
    @type magic: C{unicode}

    @ivar version: Version number of the magic.
    @type version: C{int}
    """
    def __init__(self):
        self.magic = None
        self.version = None

    def loadfromDOM(self, node):
        self.magic = node.getAttribute("magic")
        self.version = int(node.getAttribute("version"))


class Structures(Magic):
    """
    Definition of file structures.

    @ivar blocks: Defined blocks, ordered by magic.
    @type blocks: C{dict} of L{Block}
    """
    def __init__(self):
        Magic.__init__(self)
        self.blocks = None

    def loadfromDOM(self, node):
        Magic.loadfromDOM(self, node)

        # Load enum definitions into the data type factory.
        enum_nodes = node.getElementsByTagName("enum")
        for e in enum_nodes:
            ed = EnumDefinition()
            ed.loadfromDOM(e)

            # Convert to a data type in the data type factory.
            dt_enum = datatypes.EnumerationDataType(ed.name, ed.values, ed.type)
            datatypes.factory.add_type(dt_enum)

        # Load actual data
        struct_nodes = node.getElementsByTagName("block")
        self.blocks = {}
        for b in struct_nodes:
            block = Block()
            block.loadfromDOM(b)
            assert block.magic not in self.blocks
            self.blocks[block.magic] = block

    def get_block(self, magic):
        """
        Get the game block with the right magic.

        @param magic: Requested magic name.
        @type  magic: C{unicode}

        @return: The requested block, if available.
        @rtype:  L{Block} or C{None}
        """
        return self.blocks.get(magic)

class EnumDefinition(object):
    """
    Enumeration definition.

    @ivar name: Name of the enumeration.
    @type name: C{unicode}

    @ivar type: Underlying numeric type.
    @type type: C{unicode}

    @ivar values: Enumeration values.
    @type values: C{dict} of C{unicode} to C{unicode}
    """
    def __init__(self):
        self.name = None
        self.type = None
        self.values = {}

    def loadfromDOM(self, node):
        self.name = node.getAttribute("name")
        self.type = node.getAttribute("type")
        self.values = {}
        val_nodes = node.getElementsByTagName("value")
        for vn in val_nodes:
            val_name  = vn.getAttribute("name")
            val_value = vn.getAttribute("value")
            assert val_name not in self.values
            self.values[val_name] = val_value

class Block(object):
    """
    Single block.

    @ivar magic: Magic of the block.
    @type magic: C{unicode}

    @ivar minversion: Smallest (oldest) supported version.
    @type minversion: C{int}

    @ivar maxversion: Biggest (newest) supported version.
    @type maxversion: C{int}

    @ivar fields: Fields of the block.
    @type fields: C{list} of L{Field}
    """
    def __init__(self):
        self.magic = None
        self.minversion = None
        self.maxversion = None
        self.fields = []

    def loadfromDOM(self, node):
        self.magic = node.getAttribute("magic")
        self.minversion = int(node.getAttribute("minversion"))
        self.maxversion = int(node.getAttribute("maxversion"))
        nodes = node.getElementsByTagName("field")
        for n in nodes:
            field = Field()
            field.loadfromDOM(n)
            self.fields.append(field)

    def get_fields(self, version):
        """
        Filter the fields for the given version.

        @param version: Version number of the game data.
        @type  version: C{int}

        @return: If the version is valid, the fields which are valid for the
                 given version in the same order as in the structure definition.
        @rtype:  C{None} or C{list} of L{Field}
        """
        if self.minversion > version: return None
        if self.maxversion < version: return None

        flds = []
        fld_names = set() # Paranoia checking for unique field-names.
        for fld in self.fields:
            if fld.minversion is not None and fld.minversion > version: continue
            if fld.maxversion is not None and fld.maxversion < version: continue
            assert fld.name not in fld_names
            flds.append(fld)
            fld_names.add(fld.name)
        return flds

    def __str__(self):
        if len(self.fields) == 0:
            txt = "None"
        else:
            txt = "\n\t".join(str(f) for f in self.fields)
        return "Block:\t" + txt


class Field(object):
    """
    Field in a block.

    @ivar name: Name of the field.
    @type name: C{unicode}

    @ivar description: Description of the field.
    @type description: C{unicode}

    @ivar type: Type of the field, one of the fundamental types, or another block.
    @type type: C{unicode}

    @ivar minversion: Block version where the field exists for the first time.
                      Default C{None}, meaning '-infinite'.
    @type minversion: C{None} or C{int}

    @ivar maxversion: Block version where the field exists for the last time.
                      Default C{None}, meaning 'infinite'.
    @type maxversion: C{None} or C{int}
    """
    def __init__(self):
        self.name = None
        self.description = None
        self.type = None
        self.minversion = None
        self.maxversion = None

    def loadfromDOM(self, node):
        self.name = node.getAttribute("name")
        self.description = "".join(n.data for n in node.childNodes if n.nodeType == Node.TEXT_NODE)
        self.type = node.getAttribute("type")
        self.minversion = None
        if node.hasAttribute("minversion"): self.minversion = int(node.getAttribute("minversion"))
        self.maxversion = None
        if node.hasAttribute("maxversion"): self.maxversion = int(node.getAttribute("maxversion"))

    def __str__(self):
        txt = "%s:%s" % (self.name, self.type)
        if self.minversion is not None: txt = txt + (" >=%d" % self.minversion)
        if self.maxversion is not None: txt = txt + (" <=%d" % self.maxversion)
        return txt


def loadfromDOM(filename):
    dom = minidom.parse(filename)
    rcdstructure = Structures()
    rcdstructure.loadfromDOM(dom.getElementsByTagName("structures").item(0))
    return rcdstructure

