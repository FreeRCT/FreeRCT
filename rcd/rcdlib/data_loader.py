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
from xml.dom import minidom
from xml.dom.minidom import Node

# {{{ def get_opt_DOM(node, name, default):
def get_opt_DOM(node, name, default):
    """
    Get an optional value from the DOM node.

    @param node: DOM node being read.
    @type  node: L{xml.dom.minidom.ElementNode}

    @param name: Name of the value.
    @type  name: C{str}

    @param default: Default value as string.
    @type  default: C{unicode}

    @return: The reuqested value.
    @rtype:  C{unicode}
    """
    if node.hasAttribute(name): return node.getAttribute(name)
    return default

# }}}
# {{{ class RcdFiles(object):
class RcdFiles(object):
    """
    Class representing the rcd files to create.

    @ivar files: Files to create.
    @type files: C{list} of L{RcdFile}
    """
    def __init__(self):
        self.files = []

    def loadfromDOM(self, node):
        nodes = node.getElementsByTagName("file")
        for n in nodes:
            f = RcdFile()
            f.loadfromDOM(n)
            self.files.append(f)

# }}}
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

    @ivar blocks: Game blocks of the file.
    @type blocks: C{list} of L{RcdGameBlock}
    """
    def __init__(self):
        RcdMagic.__init__(self)
        self.target = None
        self.blocks = []

    def loadfromDOM(self, node):
        """
        Load file contents from the DOM node.

        @param node: DOM node (points to a 'node' node tagged by 'file').
        @type  node: L{Node}
        """
        RcdMagic.loadfromDOM(self, node)
        self.target = node.getAttribute("target")
        nodes = node.getElementsByTagName("gameblock")
        for n in nodes:
            g = RcdGameBlock()
            g.loadfromDOM(n)
            self.blocks.append(g)

# }}}
# {{{ class RcdGameBlock(RcdMagic):
class RcdGameBlock(RcdMagic):
    """
    Game block of an rcd file.

    @ivar fields: Fields of the game block, ordered by name.
    @type fields: C{dict} of C{unicode to }L{RcdField}
    """
    def __init__(self):
        RcdMagic.__init__(self)
        self.fields = {}

    def loadfromDOM(self, node):
        """
        Load block contents from the DOM node.

        @param node: DOM node (points to a 'node' node tagged as 'gameblock').
        @type  node: L{Node}
        """
        RcdMagic.loadfromDOM(self, node)

        self.fields = {}
        for n in node.childNodes:
            if n.nodeType != Node.ELEMENT_NODE: continue

            if n.tagName == "field":
                fld = RcdDataField()
                fld.loadfromDOM(n)
                self.fields[fld.name] = fld

            elif n.tagName == "sheet":
                flds = get_sheet_images(n)
                for fld in flds:
                    self.fields[fld.name] = fld

            elif n.tagName == "image":
                img = RcdImageField()
                img.loadfromDOM(n)
                self.fields[img.name] = img

            elif n.tagName == "frames":
                fld = RcdFramesField()
                fld.loadfromDOM(n)
                self.fields[fld.name] = fld

# }}}

# {{{ class RcdField(object):
class RcdField(object):
    """
    Base class of a field in the game data file.

    @ivar field_def: Definition of the field (mostly the field type).
    @type field_def: L{Field}
    """
    def __init__(self):
        self.field_def = None

# }}}
# {{{ class RcdDataField(RcdField):
class RcdDataField(RcdField):
    """
    A 'simple' data field.

    @ivar name: Name of the field.
    @type name: C{unicode}

    @ivar value: Value of the field.
    @type value: C{unicode}
    """
    def __init__(self):
        RcdField.__init__(self)
        self.name = None
        self.value = None

    def loadfromDOM(self, node):
        """
        Fill the data of an L{RcdDataField} object from a DOM node.

        @param node: DOM node containing the data of the object.
        @type  node: L{xml.dom.minidom.ElementNode}
        """
        self.name = node.getAttribute("name")
        self.value = node.getAttribute("value")

# }}}
# {{{ class RcdImageField(RcdField):
class RcdImageField(RcdField):
    """
    An image in a field.

    @ivar name: Name of the image (sprite).
    @type name: C{unicode}

    @ivar x_base: Base X position of the image. Default 0.
    @type x_base: C{int}

    @ivar y_base: Base Y position of the image. Default 0.
    @type y_base: C{int}

    @ivar width: Width of the image.
    @type width: C{int}

    @ivar height: Height of the image.
    @type height: C{int}

    @ivar x_offset: X offset of the image. Default 0.
    @type x_offset: C{int}

    @ivar y_offset: Y offset of the image. Default 0.
    @type y_offset: C{int}

    @ivar fname: Filename.
    @type fname: C{unicode}

    @ivar transp: Transparent colour. Default 0.
    @type transp: C{int}

    @todo L{transp} is likely not implemented.
    """
    def __init__(self):
        RcdField.__init__(self)
        self.name = None
        self.x_base = 0
        self.y_base = 0
        self.width = None
        self.height = None
        self.fname = None
        self.x_offset = 0
        self.y_offset = 0
        self.transp = 0

    def loadfromDOM(self, node):
        self.name     = node.getAttribute("name")
        self.x_base   = int(get_opt_DOM(node, 'x-base', u"0"))
        self.y_base   = int(get_opt_DOM(node, 'y-base', u"0"))
        self.width    = int(node.getAttribute("width"))
        self.height   = int(node.getAttribute("height"))
        self.x_offset = int(get_opt_DOM(node, "x-offset", u"0"))
        self.y_offset = int(get_opt_DOM(node, "y-offset", u"0"))
        self.fname    = node.getAttribute("fname")
        self.transp   = int(get_opt_DOM(node, 'transparent', u"0"))

# }}}
# {{{ def get_sheet_images(node):
def get_sheet_images(node):
    """
    Construct the images that are laid out in a image sheet.

    @param node: DOM node containing the data of the object.
    @type  node: L{xml.dom.minidom.ElementNode}

    @return: Images of the sheet.
    @rtype:  C{list} of L{RcdImageField}
    """
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
    transp   = int(get_opt_DOM(node, 'transparent', u"0"))

    images = []
    for ridx, row in enumerate(names.split("|")):
        for cidx, col in enumerate(row.split(",")):
            img = RcdImageField()
            img.name = col.strip()
            img.x_base = x_base + x_step * cidx
            img.y_base = y_base + y_step * ridx
            img.width = width
            img.height = height
            img.x_offset = x_offset
            img.y_offset = y_offset
            img.fname = fname
            images.append(img)
    return images

# }}}
# {{{ class RcdFramesField(RcdField):
class RcdFramesField(RcdField):
    """
    A field containing a sequence of animation frames.

    @ivar name: Name of the frames.
    @type name: C{unicode}

    @ivar frames: Animation frames.
    @type frames: C{list} of L{RcdFrame}
    """
    def __init__(self):
        RcdField.__init__(self)
        self.frames = []

    def loadfromDOM(self, node):
        """
        Load the contents of the 'frames' node.

        @param node: DOM node containing the frames.
        @type  node: L{xml.dom.minidom.ElementNode}
        """
        self.name = node.getAttribute("name")
        self.frames = []
        for frame in node.getElementsByTagName("frame"):
            f = RcdFrame()
            f.loadfromDOM(frame)
            self.frames.append(f)

class RcdFrame(RcdField):
    """
    A class containing a single frame of an animation.

    @ivar x_base: Base X position of the image. Default 0.
    @type x_base: C{int}

    @ivar y_base: Base Y position of the image. Default 0.
    @type y_base: C{int}

    @ivar width: Width of the image.
    @type width: C{int}

    @ivar height: Height of the image.
    @type height: C{int}

    @ivar x_offset: X offset of the image. Default 0.
    @type x_offset: C{int}

    @ivar y_offset: Y offset of the image. Default 0.
    @type y_offset: C{int}

    @ivar fname: Filename.
    @type fname: C{unicode}

    @ivar duration: Length of the frame in milli seconds.
    @type duration: C{int}

    @ivar game_dx: X movement after displaying this frame.
    @type game_dx: C{int}

    @ivar game_dy: Y movement after displaying this frame.
    @type game_dy: C{int}

    @ivar number: Number indicating the frame ID.
                  Used to switch between animations when the zoom changes.
    @type number: C{int}

    @ivar transp: Transparent colour. Default 0.
    @type transp: C{int}
    """
    def __init__(self):
        RcdField.__init__(self)
        self.name = None
        self.x_base = 0
        self.y_base = 0
        self.width = None
        self.height = None
        self.fname = None
        self.x_offset = 0
        self.y_offset = 0
        self.duration = 0
        self.game_dx = 0
        self.game_dy = 0
        self.number = 0
        self.transp = 0

    def loadfromDOM(self, node):
        self.name     = node.getAttribute("name")
        self.x_base   = int(get_opt_DOM(node, 'x-base', u"0"))
        self.y_base   = int(get_opt_DOM(node, 'y-base', u"0"))
        self.width    = int(node.getAttribute("width"))
        self.height   = int(node.getAttribute("height"))
        self.x_offset = int(get_opt_DOM(node, "x-offset", u"0"))
        self.y_offset = int(get_opt_DOM(node, "y-offset", u"0"))
        self.fname    = node.getAttribute("fname")
        self.duration = int(node.getAttribute("duration"))
        self.game_dx  = int(node.getAttribute("game-dx"))
        self.game_dy  = int(node.getAttribute("game-dy"))
        self.number   = int(node.getAttribute("number"))
        self.transp   = int(get_opt_DOM(node, 'transparent', u"0"))

# }}}


def loadfromDOM(fname):
    """
    Load a game data xml file.

    @param fname: Filename of the data file to load.
    @type  fname: C{str}

    @return: Loaded data-file as a tree of data.
    @rtype:  L{RcdFiles}
    """
    dom = minidom.parse(fname)
    root = RcdFiles()
    root.loadfromDOM(dom.getElementsByTagName("rcdfiles").item(0))
    return root

