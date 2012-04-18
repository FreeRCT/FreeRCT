
Author: Alberth
Version: $Id$

RCD data file format
====================

License
~~~~~~~
This file is part of FreeRCT.
FreeRCT is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, version 2.
FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
See the GNU General Public License for more details. You should have received a
copy of the GNU General Public License along with FreeRCT. If not, see
<http://www.gnu.org/licenses/>.

Introduction
~~~~~~~~~~~~
FreeRCT is a game where you build roller-coasters in your theme park, hoping to
get lots of visitors paying for your investments. If you do it well, you might
get rich.

The program uses so-called RCD files, data files with a binary file format to
pack graphics and other game data, ready for use by the FreeRCT program. This
document describes the format, acting as an independent definition of the file
format.


File header
~~~~~~~~~~~
Each data file starts with a file header indicating it is an RCD file.
The format is as follows

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'RCDF'.
   4       4    Value '1', version number of the data file format.
   8            Total length.
======  ======  ==========================================================


Data blocks
~~~~~~~~~~~
After the file header come the various data blocks.
The goal of data blocks is to provide blobs of information that are somewhat independent.
The data blocks are referenced by game blocks by their ID. The first data block
gets number 1, the second block number 2, etc.

A reference to data block 0 means 'not present'.


Sprite Pixels
-------------
A data block containing the actual image of a sprite (in 8bpp).

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string '8PXL'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Width of the image.
  14       2    Height of the image, called 'h' below.
  16     4*h    Jump table to pixel data of each line. Offset is relative
                  to the first entry of the jump table. Value 0 means there
                  is no data for that line.
   ?       ?    Pixels of each line.
   ?            Variable length.
======  ======  ==========================================================


Line data is a sequence of pixels with an offset. Its format is

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       1    Relative offset (0-127), bit 7 means 'last entry of the
                  line'.
   1       1    Number of pixels that follow this count, called n (0-255).
   2       n    Pixels, 1 byte per pixel (as it is 8bpp).
   ?            Variable length.
======  ======  ==========================================================

The offset byte is relative to the end of the previous pixels, thus an offset
of 0 means no gap between the pixels. A count of 0 is useful if the gap at a
line is longer than 127 pixels.

To decide: Some simple form of compressing may be useful in the pixels as it
           decreases the amount of memory transfers.


Sprite block
------------
Data of a single sprite. Version 2 is currently supported by FreeRCT.

======  ======  =======  =================================================
Offset  Length  Version  Description
======  ======  =======  =================================================
   0       4      1-2    Magic string 'SPRT'.
   4       4      1-2    Version number of the block.
   8       4      1-2    Length of the block excluding magic string,
                  1-2    version, and length.
  12       2      1-2    (signed) X-offset.
  14       2      1-2    (signed) Y-offset.
  16       4      1-2    Sprite image data, reference to a 8PXL block.
  20     768       1     (removed in version 2) Palette data.
  20                     Total length of version 2
======  ======  =======  =================================================


Game blocks
~~~~~~~~~~~
A game block is a piece of data that relates closely to a concept in the
game, like 'path' or 'roller coaster'. Normally it refers to one or more
data blocks.

Tile surface sprite sub-block
-----------------------------
In several game blocks you can find a set of sprite for the ground. Below is
the layout of such a sub-block.
Note that the sprites should look to the north (thus, the sprite at 4 has its
back corner up).

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Flat surface tile.
   4       4    North corner up.
   8       4    East corner up.
  12       4    North, east corners up.
  16       4    South corner up.
  20       4    North, south corners up.
  24       4    East, south corners up.
  28       4    North, east, south corners up.
  32       4    West corner up.
  36       4    West, north corners up.
  40       4    West, east corners up.
  44       4    West, north, east corners up.
  48       4    West, south corners up.
  52       4    West, north, south corners up.
  56       4    West, east, south corners up.
  60       4    Steep north slope.
  64       4    Steep east slope.
  68       4    Steep south slope.
  72       4    Steep west slope.
  76            Total length of the sub-block.
======  ======  ==========================================================


Ground tiles block
------------------
A set of ground tiles that form a smooth surface. Current version in
FreeRCT is 3.

======  ======  =======  =================================================
Offset  Length  Version  Description
======  ======  =======  =================================================
   0       4      1-3    Magic string 'SURF'.
   4       4      1-3    Version number of the block.
   8       4      1-3    Length of the block excluding magic string,
                           version, and length.
  12       2      2-3    Type of ground.
  14       2      1-3    Zoom-width of a tile of the surface.
  16       2      1-3    Change in Z height (in pixels) when going up or
                           down a tile level.
  18      76      1-3    Tile surface sprite sub-block for north viewing
                           direction.
  94      76      1-2    Tile surface sprite sub-block for east viewing
                           direction.
  94      76      1-2    Tile surface sprite sub-block for south viewing
                           direction.
  94      76      1-2    Tile surface sprite sub-block for west viewing
                           direction.
  94                     Total length of version 3.
======  ======  =======  =================================================

Known types of ground:
 - Empty  (0), do not use in the RCD file.
 - Grass  (16-19,) Green grass ground, with increasing length grass on it.
 - Sand   (32), desert 'ground'.
 - Cursor (48), cursor test tiles. Internal use. Defines what part of a
   tile is selected. Colour 181 means 'north corner', 182 means 'east corner',
   184 means 'west corner', 185 means 'south corner', and 183 means 'entire
   tile'.

To do: Move the cursor tile to another position.


Tile selection
--------------
A tile selection cursor. It is very similar to ground tiles, except there is
no type.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'TSEL'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom-width of a tile of the surface.
  14       2    Change in Z height (in pixels) when going up or down a
                  tile level.
  16      76    Tile surface sprite sub-block.
  92            Total length.
======  ======  ==========================================================


Tile area selection
-------------------

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'TARE'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    zoom-width of a tile of the surface.
  14       2    Change in Z height (in pixels) when going up or down a
                  tile level.
  16      76    Tile surface sprite sub-block.
  92            Total length.
======  ======  ==========================================================


Patrol area selection
---------------------

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'PARE'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom-width of a tile of the surface.
  14       2    Change in Z height (in pixels) when going up or down a
                  tile level.
  16      76    Tile surface sprite sub-block.
  92            Total length.
======  ======  ==========================================================


Tile corner selection block
---------------------------
Sprites for pointing to a single corner of a surface tile.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'TCOR'
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom-width of a tile of the surface.
  14       2    Change in Z height (in pixels) when going up or down a
                  tile level.
  16      76    Tile surface sprite sub-block for selected corner pointing
                  north.
  92      76    Tile surface sprite sub-block for selected corner pointing
                  east.
 168      76    Tile surface sprite sub-block for selected corner pointing
                  south.
 244      76    Tile surface sprite sub-block for selected corner pointing
                  west.
 320            Total length.
======  ======  ==========================================================


Shops/stalls
------------
One tile objects.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'SHOP'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom-width of a tile of the surface.
  14       2    Height of the shop in voxels.
  16       4    View to the north where the entrance is at the NE edge.
  20       4    View to the north where the entrance is at the SE edge.
  24       4    View to the north where the entrance is at the SW edge.
  28       4    View to the north where the entrance is at the NW edge.
  32            Total length.
======  ======  ==========================================================


Build direction arrows
----------------------

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'BDIR'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom-width of a tile of the surface.
  14       4    Arrow pointing to NE edge.
  18       4    Arrow pointing to SE edge.
  22       4    Arrow pointing to SW edge.
  26       4    Arrow pointing to NW edge.
  30            Total length.
======  ======  ==========================================================


Foundations block
-----------------
Vertical foundations to close gaps in the smooth surface.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'FUND'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Type of foundation.
  14       2    Zoom-width of a tile.
  16       2    Change in Z height of the tiles.
  18       4    Vertical south-east foundation, east up, south down.
  22       4    Vertical south-east foundation, east down, south up.
  26       4    Vertical south-east foundation, east up, south up.
  30       4    Vertical south-west foundation, south up, west down.
  34       4    Vertical south-west foundation, south down, west up.
  38       4    Vertical south-west foundation, south up, west up.
  42            Total length
======  ======  ==========================================================


Known types of foundation:
 - Empty (0) Reserved, do not use in the RCD file.
 - Ground (16)
 - Wood (32)
 - Brick (48)

The tile width and z-height are used to ensure the foundations match with the
surface tiles.


Path block
----------
Path coverage is a set of at most 47 flat images. Paths can connect to
neighbouring tiles through four edges, optionally also covering the corner
between two connecting edges.

Starting at offset 14 are the sprite block numbers of each sprite. As normal,
use 0 to denote absence of a sprite. Two letter words in the description
denote an edge connects, one letter words denote the corner is covered.

Besides the maximal 47 flat sprites there are also 4 sprites with one edge
raised.

 - Empty (0) Reserved, do not use in the RCD file.
 - Concrete (16)


======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'PATH'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and length.
  12       2    Type of path surface.
  14       2    Zoom-width of a tile.
  16       2    Change in Z height of the tiles.
  18       4    (empty).
  22       4    NE.
  26       4    SE.
  30       4    NE, SE.
  34       4    NE, SE, E.
  38       4    SW.
  42       4    NE, SW.
  46       4    SE, SW.
  50       4    SE, SW, S.
  54       4    NE, SE, SW.
  58       4    NE, SE, SW, E.
  62       4    NE, SE, SW, S.
  66       4    NE, SE, SW, E, S.
  70       4    NW.
  74       4    NE, NW.
  78       4    NE, NW, N.
  82       4    NW, SE.
  86       4    NE, NW, SE.
  90       4    NE, NW, SE, N.
  94       4    NE, NW, SE, E.
  98       4    NE, NW, SE, N, E.
 102       4    NW, SW.
 106       4    NW, SW, W.
 110       4    NE, NW, SW.
 114       4    NE, NW, SW, N.
 118       4    NE, NW, SW, W.
 122       4    NE, NW, SW, N, W.
 126       4    NW, SE, SW.
 130       4    NW, SE, SW, S.
 134       4    NW, SE, SW, W.
 138       4    NW, SE, SW, S, W.
 142       4    NE, NW, SE, SW.
 146       4    NE, NW, SE, SW, N.
 150       4    NE, NW, SE, SW, E.
 154       4    NE, NW, SE, SW, N, E.
 158       4    NE, NW, SE, SW, S.
 162       4    NE, NW, SE, SW, N, S.
 166       4    NE, NW, SE, SW, E, S.
 170       4    NE, NW, SE, SW, N, E, S.
 174       4    NE, NW, SE, SW, W.
 178       4    NE, NW, SE, SW, N, W.
 182       4    NE, NW, SE, SW, E, W.
 186       4    NE, NW, SE, SW, N, E, W.
 190       4    NE, NW, SE, SW, S, W.
 194       4    NE, NW, SE, SW, N, S, W.
 198       4    NE, NW, SE, SW, E, S, W.
 202       4    NE, NW, SE, SW, N, E, S, W.
 206       4    NE edge up.
 210       4    NW edge up.
 214       4    SE edge up.
 218       4    SW edge up.
 222            Length of one view direction.
======  ======  ==========================================================


Platforms
---------
Platforms put up in the air, to carry the weight of a path.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'PLAT'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom-width of a tile of the surface.
  14       2    Change in Z height (in pixels) when going up or down a
                  tile level.
  16       2    Platform type.
  18       4    Flat platform for north and south view.
  22       4    Flat platform for east and west view.
  26       4    Platform is raised at the NE edge.
  30       4    Platform is raised at the SE edge.
  34       4    Platform is raised at the SW edge.
  38       4    Platform is raised at the NW edge.
  42            Total length.
======  ======  ==========================================================


Platform type:
 - Empty 0, do not use.
 - Wood 16.


Platform supports
-----------------
Structures to support platforms, so they don't fall down.

Currently largely undecided.

-rotation?
-steep slopes?



GUI
~~~
GUI sprites, in various forms.

Generic GUI border sprites
--------------------------
The most common form of a widget is a rectangular shape.
To draw such a shape, nine sprites are needed around the border of the
rectangle.

        +-------------+---------------+--------------+
        | top-left    | top-middle    | top-right    |
        | left        | middle        | right        |
        | bottom-left | bottom-middle | bottom-right |
        +-------------+---------------+--------------+


The 'top-left', 'top-right', 'bottom-left' and 'bottom-right' sprites are used
for the corners of the widget or window. The 'top-middle', 'middle', and
'bottom-middle' should be equally wide, and are used to insert horizontal
space between the left and the right part (with step size equal to the width
of the sprites. The 'left', 'middle', and 'right' do the same, except their
common height is used for vertical resizing.

Except for the 'top-left' sprite any of the sprites can be dropped. If you
leave out 'top-middle', 'middle', or 'bottom-middle', horizontal resizing is
not possible. If you leave out 'left', 'middle', or 'right' vertical resizing
is not possible.
If you leave out 'top-right', the 'top-right', 'right', and 'bottom-right'
sprites are considered not needed. Similarly for the 'bottom-left' sprite.
Supplying the 'top-right' sprite but leaving out 'bottom-right' (and similarly
for 'bottom-left' and 'bottom-right') gives undefined behaviour.

A sprite coverage of the edge has four border width parameters (top, left,
right, and bottom), measured in pixels.
In addition, a horizontal and a vertical
offset needs to be specified relative to the bounding box of the widget
contents.

That leads to the following block:

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GBOR'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Widget type.
  14       1    Border width of the top edge.
  15       1    Border width of the left edge.
  16       1    Border width of the right edge.
  17       1    Border width of the bottom edge.
  18       1    Minimal width of the border.
  19       1    Minimal height of the border.
  20       1    Horizontal stepsize of the border.
  21       1    Vertical stepsize of the border.
  22       4    Top-left sprite.
  26       4    Top-middle sprite.
  30       4    Top-right sprite.
  34       4    Left sprite.
  38       4    Middle sprite.
  42       4    Right sprite.
  46       4    Bottom-left sprite.
  50       4    Bottom-middle sprite.
  54       4    Bottom-right sprite.
  58            Total length.
======  ======  ==========================================================

Known widget types:
 - 0 Invalid, do not use.
 - 16 Window border.
 - 32 Title bar.
 - 48 button, 49 pressed button, 52 rounded button, 53 pressed rounded button.
 - 64 frame.
 - 68 panel.
 - 80 inset frame.


Check box and radio buttons
---------------------------

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GCHK'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Widget type.
  14       4    Empty.
  18       4    Filled.
  22       4    Empty pressed.
  26       4    Filled pressed.
  30       4    Shaded empty button.
  34       4    Shaded filled button.
  38            Total length.
======  ======  ==========================================================

Known widget types:
 - 96 Check box.
 - 112 Radio-button.


Slider-bar elements
-------------------
For slider-bar GUI elements, the following block should be used.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GSLI'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       1    Minimal length of the bar.
  13       1    Stepsize of the bar.
  14       1    Width of the slider button.
  15       2    Widget type.
  17       4    Left sprite.
  21       4    Middle sprite.
  25       4    Right sprite.
  29       4    Slider button.
  33            Total length.
======  ======  ==========================================================

Known slider-bar widget types:
 - 128 Horizontal slider bar + button.
 - 129 Shaded horizontal slider bar + button.
 - 144 Vertical slider bar + button.
 - 145 Shaded vertical slider bar + button.


Scroll-bar elements
-------------------
For scroll-bar GUI elements, the following block should be used.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GSCL'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       1    Minimal length scrollbar.
  13       1    Stepsize of background.
  14       1    Minimal length bar.
  15       1    Stepsize of bar.
  16       2    Widget type.
  18       4    Left/up button.
  22       4    Right/down button.
  26       4    Left/up pressed button.
  30       4    Right/down pressed button.
  34       4    Left/top bar bottom (the background).
  38       4    Middle bar bottom (the background).
  42       4    Right/down bar bottom (the background).
  46       4    Left/top bar top.
  50       4    Middle bar top.
  54       4    Right/down bar top.
  58       4    Left/top pressed bar top.
  62       4    Middle pressed bar top.
  66       4    Right/down pressed bar top.
  70            Total length.
======  ======  ==========================================================

Known scroll-bar widget types:
 - 160 Horizontal scroll bar + button.
 - 161 Shaded horizontal scroll bar + button.
 - 176 Vertical scroll bar + button.
 - 177 Shaded vertical scroll bar + button.


Animation
~~~~~~~~~
Animation sequences are defined using the 'ANIM' block.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'ANIM'.
   4       4    Version number of the block '1'.
   8       4    Length of the block excluding magic string, version, and
                  length.
  12       2    Zoom width of a tile.
  14       1    Person type.
  15       2    Animation type.
  17       2    Frame count (called 'f').
  19     f*10   Data of all frames.
   ?            Variable length.
======  ======  ==========================================================

A person type defines for which kind of persons the animation can be used:
 - Any (0) Any kind of person (eg persons are not shown).
 - Pillar (8) Guests from the Pillar Planet (test graphics).
 - Eartch (16) More familiar kinds of persons,

The animation type defines what the animation really shows. Currently, the
following animations exist:
 - Walk in north-east direction (1). May be looped.
 - Walk in south-east direction (2). May be looped.
 - Walk in south-west direction (3). May be looped.
 - Walk in north-west direction (4). May be looped.

Finally the actual frames of the animation are listed, prefixed by how
many frames to expect. The animation type decides whether or not an animation
can be repeated by looping.
A single frame consists of the following data.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Sprite.
   4       2    Duration of the frame in milli seconds.
   6       2    (signed) X position change after displaying the frame.
   8       2    (signed) Y position change after displaying the frame.
  10       2    Frame number.
  12            Total length.
======  ======  ==========================================================

The frame number should correspond with frame numbers of the same animation,
but at a different zoom width. It allows for looking up an equivalent stage in
another animation, for nice transitions, in case the zoom width changes during
the game.


.. vim: set spell tw=78
