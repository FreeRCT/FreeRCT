# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
"""
Game data loaded from file.
"""
from rcdlib import datatypes

from xml.dom import minidom
from xml.dom.minidom import Node

# {{{ class RcdMagic(object):
class RcdMagic(object):
    """
    Data and block magic numbers.

    @ivar magic: Magic name.
    @type magic: C{unicode}, 4 letters

    @ivar version: Used version.
    @type version: C{int}
    """
    def __init__(self):
        self.magic = None
        self.version = None

    def loadfromDOM(self, node):
        self.magic = node.getAttribute("magic")
        self.version = int(node.getAttribute("version"))

# }}}
# {{{ class RcdFile(RcdMagic):
class RcdFile(RcdMagic):
    """
    Class representing a single RCD data file.

    @ivar target: Target filename.
    @type target: C{unicode}

    @ivar block_nodes: XML nodes of the game blocks in the file.
    @type block_nodes: C{list} of L{xml.dom.minidom.Node}
    """
    def __init__(self):
        RcdMagic.__init__(self)
        self.target = None
        self.block_nodes = []

    def loadfromDOM(self, node):
        """
        Load file contents from the DOM node.

        @param node: DOM node (points to a 'node' node tagged by 'file').
        @type  node: L{Node}
        """
        RcdMagic.loadfromDOM(self, node)
        self.target = node.getAttribute("target")
        self.block_nodes = datatypes.get_child_nodes(node, u"gameblock")

# }}}

class Name(object):
    """
    A name with optional extra data attached.

    @ivar name: Name of the object.
    @type name: C{unicode}

    @ivar data: Extra data.
    @type data: C{None} or a (C{int}, C{int}) tuple.
    """
    def __init__(self, name, data = None):
        self.name = name
        self.data = data

    def __cmp__(self, other):
        if not isinstance(other, Name): return -1
        if self.name == other.name: return 0
        if self.name < other.name: return -1
        return 1

    def __eq__(self, other):
        if not isinstance(other, Name): return False
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)


class NamedNode(object):
    """
    A node in an XML file with a tag and (one or more) names.

    @ivar node: Node in the XML file.
    @type node: L{xml.dom.minidom.Node}

    @ivar tag: Name of the tag.
    @type tag: C{unicode}

    @ivar names: Names of the node ('name' attribute).
    @type names: C{set} of L{Name}
    """
    def __init__(self, node, tag, names):
        self.node = node
        self.tag = tag
        self.names = names

def load_named_nodes(parent):
    """
    Load named nodes from the parent node.

    @param parent: Parent node containing the named nodes.
    @type  parent: L{xml.dom.minidom.Node}

    @return: Found named nodes.
    @rtype:  C{list} of L{NamedNode}
    """
    nodes = []
    for node in parent.childNodes:
        if node.nodeType != Node.ELEMENT_NODE: continue

        if node.tagName in (u"field", u"image", u"bitfield", u"struct", u"texts"):
            name = Name(node.getAttribute(u"name"))
            nodes.append(NamedNode(node, node.tagName, set([name])))
            continue

        if node.tagName == u"sheet":
            names = set()
            names_text = node.getAttribute(u"names")
            for ridx, rnames in enumerate(names_text.split(u"|")):
                for cidx, nm in enumerate(rnames.split(u",")):
                    names.add(Name(nm.strip(), (cidx, ridx)))
            nodes.append(NamedNode(node, node.tagName, names))
            continue

        # Unrecognized tag, report it
        print "WARNING: Skipping " + node.tagName

    return nodes

