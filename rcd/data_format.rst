:Author: Alberth
:Version: $Id$

.. contents::
   :depth: 3

####################
RCD data file format
####################

.. Section levels  # = ~ .

License
=======
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
============
FreeRCT is a game where you build roller-coasters in your theme park, hoping to
get lots of visitors paying for your investments. If you do it well, you might
get rich.

The program uses so-called RCD files, data files with a binary file format to
pack graphics and other game data, ready for use by the FreeRCT program. This
document describes the format, acting as an independent definition of the file
format.

Money
~~~~~
All money amounts in RCD files are signed integer numbers, expressing cents.

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

Version history
...............

- 1 (20110915) Initial version.


Data blocks
===========
After the file header come the various data blocks.
The goal of data blocks is to provide blobs of information that are somewhat independent.
The data blocks are referenced by game blocks by their ID. The first data block
gets number 1, the second block number 2, etc.

A reference to data block 0 means 'not present'.


Sprite Pixels
~~~~~~~~~~~~~
A data block containing the actual image of a sprite (in 8bpp), and its
offset. Version 2 is supported by FreeRCT.

======  ======  =======  =================================================
Offset  Length  Version  Description
======  ======  =======  =================================================
   0       4      1-2    Magic string '8PXL'.
   4       4      1-2    Version number of the block.
   8       4      1-2    Length of the block excluding magic string,
                         version, and length.
  12       2      1-2    Width of the image.
  14       2      1-2    Height of the image, called 'h' below.
  16       2        2    (signed) X-offset.
  18       2        2    (signed) Y-offset.
  20     4*h      1-2    Jump table to pixel data of each line. Offset is
                         relative to the first entry of the jump table.
                         Value 0 means there is no data for that line.
   ?       ?      1-2    Pixels of each line.
   ?                     Variable length.
======  ======  =======  =================================================


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

Version history
...............

- 1 (20110915) Initial version.
- 2 (20120623) Moved the offset from the SPRT block into the 8PXL block.

Texts
~~~~~
Text in various forms and shapes is very common. In particular, it needs to
support translations, and eventually run-time composition of text with respect
to genders, plurals, and cases.
The latter will be encoded in the text itself, and does not need to be handled
here (except perhaps for some simple translations).

What remains is a collection of names that are attached to text (the game
queries text by name), where the latter may exist in several languages. All
text is assumed to be UTF-8 encoded, and 0-terminated.

FreeRCT can read version 1 text blocks, which look like

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'TEXT'.
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       ?    First string.
   ?       ?    Second string.
  ...     ...
======  ======  ==========================================================

A string has the following structure.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       2    Length of the entire string, including these length bytes.
   2       1    Length of the identification name of the string (incl 0).
   3       ?    Identification name itself (0 terminated)
   ?       ?    First translation.
   ?       ?    Second translation.
  ...     ...
   ?       ?    Default translation.
======  ======  ==========================================================

A translation has the following structure.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       2    Length of this translation (including these length bytes).
   2       1    Length of the language name (incl 0).
   3       ?    Language name itself (0 terminated).
   ?       ?    Text of the string in the indicated language (incl 0).
======  ======  ==========================================================

The default language has no language name ie it is "" (the empty string).
Other languages use one of the following tags (currently ``name of language -
name of country area`` but that may change in the future).

=====  =========================
Tag    Description
=====  =========================
en_GB  Great Britain.
nl_NL  The Netherlands.
=====  =========================

String parameters
~~~~~~~~~~~~~~~~~
A character sequence of the form "%...%" is an escape sequence. The following
sequences are recognized:

- "%%": The character "%".
- "%n%" with "n" a decimal number: Insert parameter 'n'.

Version history
...............

- 1 (20120714) Initial version.
-   (20121215) Added string parameters.


Game blocks
===========
A game block is a piece of data that relates closely to a concept in the
game, like 'path' or 'roller coaster'. Normally it refers to one or more
data blocks.

Tile surface sprite sub-block
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

Version history
...............

- 1 (20110915) Initial version.


Ground tiles block
~~~~~~~~~~~~~~~~~~
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

Version history
...............

- 1 (20110915) Initial version.
- 2 (20110918) Added ground type.
- 3 (20110925) Removed sprite blocks for viewing in east, south, and west direction.
-   (20111014) Added 'Cursor' tiles type.

Tile selection
~~~~~~~~~~~~~~
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

Version history
...............

- 1 (20110925) Initial version.


Tile area selection
~~~~~~~~~~~~~~~~~~~
Sprites to point out a (selection of) tile(s) to the user.

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

Version history
...............

- 1 (20110925) Initial version.


Patrol area selection
~~~~~~~~~~~~~~~~~~~~~

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

Version history
...............

- 1 (20110925) Initial version.


Tile corner selection block
~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

Version history
...............

- 1 (20110925) Initial version.


Shops/stalls
~~~~~~~~~~~~
One tile objects, selling useful things to guests. FreeRCT can read block version 4.

======  ======  =======  ===================================================================================
Offset  Length  Version  Description
======  ======  =======  ===================================================================================
   0       4      1-4    Magic string 'SHOP'.
   4       4      1-4    Version number of the block.
   8       4      1-4    Length of the block excluding magic string, version, and length.
  12       2      1-4    Zoom-width of a tile of the surface.
  14       1      1-4    Height of the shop in voxels. (versions 1-3 used a 16bit unsigned number).
  15       1       4     Shop flags.
  16       4      1-4    Unrotated view (ne).
  20       4      1-4    View after 1 quarter negative rotation (se).
  24       4      1-4    View after 2 quarter negative rotations (sw).
  28       4      1-4    View after 3 quarter negative rotations (nw).
  32       4      2-4    First recolouring specification.
  36       4      2-4    Second recolouring specification.
  40       4      2-4    Third recolouring specification.
  44       4       4     Cost of the first item.
  48       4       4     Cost of the second item.
  52       4       4     Monthly cost of having the shop.
  56       4       4     Additional monthly cost of having an opened shop.
  60       1       4     Item type of the first item.
  61       1       4     Item type of the second item.
  62       4      3-4    Text of the shop (reference to a TEXT block).
  66                     Total length.
======  ======  =======  ===================================================================================

Shop flags:

- bit 0 Set if the shop has an entrance to the NE in the unrotated view.
- bit 1 Set if the shop has an entrance to the SE in the unrotated view.
- bit 2 Set if the shop has an entrance to the SW in the unrotated view.
- bit 3 Set if the shop has an entrance to the NW in the unrotated view.

Item types:

- Nothing (0)
- A drink (8)
- An icecream (9)
- Non-salty food (16)
- Salty food (24)
- Umbrella (32)
- Map of the park (40)

Version history
...............

- 1 (20110925) Initial version.
- 2 (20120708) Added recolouring information.
- 3 (20120714) Added a TEXT block reference for the shop texts.
- 4 (20121005) Added Items to sell, and costs to pay.


Build direction arrows
~~~~~~~~~~~~~~~~~~~~~~
Arrows to point out direction of constructing new game elements. FreeRCT can
read blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'BDIR'.
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       2    Zoom-width of a tile of the surface.
  14       4    Arrow pointing to NE edge.
  18       4    Arrow pointing to SE edge.
  22       4    Arrow pointing to SW edge.
  26       4    Arrow pointing to NW edge.
  30            Total length.
======  ======  ==========================================================

Version history
...............

- 1 (20110925) Initial version.


Foundations block
~~~~~~~~~~~~~~~~~
Vertical foundations to close gaps in the smooth surface. FreeRCT can read
blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'FUND'.
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       2    Type of foundation.
  14       2    Zoom-width of a tile.
  16       2    Change in Z height of the tiles.
  18       4    Vertical south-east foundation, east  visible, south down.
  22       4    Vertical south-east foundation, east  down,    south visible.
  26       4    Vertical south-east foundation, east  visible, south visible.
  30       4    Vertical south-west foundation, south visible, west down.
  34       4    Vertical south-west foundation, south down,    west visible.
  38       4    Vertical south-west foundation, south visible, west visible.
  42            Total length
======  ======  ==========================================================

The semantics of 'visible' is that the foundation is visible for the entire
height of the foundation. The term 'up' means that just the point at the top is
used, and 'down' means the point at the bottom is used.

Note that the sprite-sheet also has 4 'up' sprites, but they are currently not
used.


Known types of foundation:

- Empty (0) Reserved, do not use in the RCD file.
- Ground (16)
- Wood (32)
- Brick (48)

The tile width and z-height are used to ensure the foundations match with the
surface tiles.

Version history
...............

- 1 (20110918) Initial version.
-   (20121207) Replaced sprites.


Path block
~~~~~~~~~~
Path coverage is a set of at most 47 flat images. Paths can connect to
neighbouring tiles through four edges, optionally also covering the corner
between two connecting edges.

Starting at offset 14 are the sprite block numbers of each sprite. As normal,
use 0 to denote absence of a sprite. Two letter words in the description
denote an edge connects, one letter words denote the corner is covered.

Besides the maximal 47 flat sprites there are also 4 sprites with one edge
raised. FreeRCT can read path blocks with version 1.

- Empty (0) Reserved, do not use in the RCD file.
- Concrete (16)


======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'PATH'.
   4       4    Version number of the block.
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

Version history
...............

- 1 (20110925) Initial version.
- 2 (20110930) Added tile width and z-height fields.


Platforms
~~~~~~~~~
Platforms put up in the air, to carry the weight of a path.
FreeRCT can read blocks with version 2.

======  ======  =======  =================================================
Offset  Length  Version  Description
======  ======  =======  =================================================
   0       4      1-2    Magic string 'PLAT'.
   4       4      1-2    Version number of the block.
   8       4      1-2    Length of the block excluding magic string,
                         version, and length.
  12       2      1-2    Zoom-width of a tile of the surface.
  14       2      1-2    Change in Z height (in pixels) when going up or
                         down a tile level.
  16       2      1-2    Platform type.
  18       4      1-2    Flat platform for north and south view.
  22       4      1-2    Flat platform for east and west view.
  26       4      1-2    Platform with two legs is raised at the NE edge.
  30       4      1-2    Platform with two legs is raised at the SE edge.
  34       4      1-2    Platform with two legs is raised at the SW edge.
  38       4      1-2    Platform with two legs is raised at the NW edge.
  42       4       2     Platform with right leg is raised at the NE edge.
  46       4       2     Platform with right leg is raised at the SE edge.
  50       4       2     Platform with right leg is raised at the SW edge.
  54       4       2     Platform with right leg is raised at the NW edge.
  58       4       2     Platform with left leg is raised at the NE edge.
  62       4       2     Platform with left leg is raised at the SE edge.
  66       4       2     Platform with left leg is raised at the SW edge.
  70       4       2     Platform with left leg is raised at the NW edge.
  74                     Total length.
======  ======  =======  =================================================


Platform type:

- Empty 0, do not use.
- Wood 16.


Version history
...............

- 1 (20110925) Initial version.
- 2 (20121128) Added platforms sprites with one leg, for the steep slopes.

Platform supports
~~~~~~~~~~~~~~~~~
Structures to support platforms, so they don't fall down.
FreeRCT can read blocks with version 1.


======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'SUPP'.
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       2    Type of support.
  14       2    Width of a tile.
  16       2    Change in Z height (in pixels) when going up or down a tile level.
  18       4    Single height for flat terrain, north and south view.
  22       4    Single height for flat terrain, east and west view.
  26       4    Double height for flat terrain, north and south view.
  30       4    Double height for flat terrain, east and west view.
  34       4    Double height for paths, north and south view.
  38       4    Double height for paths, east and west view.
  42       4    Single height, north leg up.
  46       4    Single height, east leg up.
  50       4    Single height, north, east legs up.
  54       4    Single height, south leg up.
  58       4    Single height, north, south legs up.
  62       4    Single height, east, south legs up.
  66       4    Single height, north, east, south legs up.
  70       4    Single height, west leg up.
  74       4    Single height, west, north legs up.
  78       4    Single height, west, east legs up.
  82       4    Single height, west, north, east legs up.
  86       4    Single height, west, south legs up.
  90       4    Single height, west, north, south legs up.
  94       4    Single height, west, east, south legs up.
  98       4    Double height for steep north slope.
 102       4    Double height for steep east slope.
 106       4    Double height for steep south slope.
 110       4    Double height for steep west slope.
 114            Total length of the sub-block.
======  ======  ==========================================================

Support type:

- Empty 0, do not use.
- Wood 16.

Version history
...............

- 1 (20121207) Initial version.

Roller coaster tracks
~~~~~~~~~~~~~~~~~~~~~
A ``RCST`` block contains all information of a single type of roller coaster.
It currently contains track piece definitions only. FreeRCT supports version 3
of the ``RCST`` block.

======  ======  =======  ==================  =================================================================
Offset  Length  Version  Field name          Description
======  ======  =======  ==================  =================================================================
   0       4      1-3                        Magic string 'RCST'.
   4       4      1-3                        Version number of the block.
   8       4      1-3                        Length of the block excluding magic string, version, and length.
  12       2      1-3    coaster_type        Type of roller coaster.
  14       1      2-3    platform_type       Platform type.
  15       4       3     texts               Texts of the coaster.
  19       2      1-3    <derived>           Number of track piece definitions (called 'n').
  21      4*n     1-3                        The track piece definitions (references to ``TRCK``).
21+4*n                                       Total length of the ``RCST`` block.
======  ======  =======  ==================  =================================================================

Currently defined coaster types:

- 1 Simple coaster tracks.

Currently define platform types:

- 1 Wood.


Version history
...............

- 1 (20130317) Initial version.
- 2 (20130430) Added type of platform.
- 3 (20130511) Added a TEXT reference.

Track pieces
~~~~~~~~~~~~

A track piece definition describes a single piece of track in a TRCK block. FreeRCT can read blocks with version 2. Each piece needs
one or more voxels. The first voxel it needs is called the *entry* voxel. The
other voxels have coordinates relative to the entry voxel. The last voxel is
called the *exit* voxel. The *entry* voxel of a track piece is at the *exit*
voxel of its predecessor.

To control which track pieces can connect to each other, both the entry and
the exit have a *connection code*. Two track pieces can be connected only when
the connection code of the exit of the first piece is the same as the
connection code of the entry of the second piece.

While the connection code is just a single number in the RCD file, in the input
it is split in a 'name' and a 'direction' while defining the track pieces.

=======  ======  =======  ==================  ================================================================
Offset   Length  Version  Field name          Description
=======  ======  =======  ==================  ================================================================
   0        4      1-2                        Magic string 'TRCK'.
   4        4      1-2                        Version number of the block.
   8        4      1-2                        Length of the block excluding magic string, version, and length.
  12        1      1-2    entry_connection    Entry connection code
  13        1      1-2    exit_connection     Exit connection code
  14        1       2     exit_dx             Relative X position of the exit voxel.
  15        1       2     exit_dy             Relative Y position of the exit voxel.
  16        1       2     exit_dz             Relative Z position of the exit voxel.
  17        1       2     speed               If non-zero, the minimal speed of cars at the track.
  18        1       2     track_flags         Flags of the track piece.
  19        4       2     cost                Cost of this track piece.
  23        2      1-2                        Number of voxels in this track piece (called 'n').
  25      36*n     1-2                        Voxel definitions
25+36*n                                       Total length of the ``TRCK`` block.
=======  ======  =======  ==================  ================================================================

The track flags are defined as follows:

- bit  0   This track piece has platforms next to the track.
- bits 1-2 Direction of the platform (if bit 0 is set).
- bit  3   This track piece may be used for initial placement.
- bit  4-5 Direction of initial placement (if bit 3 is set).


A voxel definition is

=======  ======  =======  ==================  ================================================================
Offset   Length  Version  Field name          Description
=======  ======  =======  ==================  ================================================================
   0       4       1-2    ne_back             Reference to the background tracks for north view.
   4       4        2     se_back             Reference to the background tracks for east view.
   8       4        2     sw_back             Reference to the background tracks for south view.
  12       4        2     nw_back             Reference to the background tracks for west view.
  16       4        2     ne_front            Reference to the front tracks for north view.
  20       4        2     se_front            Reference to the front tracks for east view.
  24       4        2     sw_front            Reference to the front tracks for south view.
  28       4        2     nw_front            Reference to the front tracks for west view.
  32       1       1-2    dx                  Relative X position of the voxel.
  33       1       1-2    dy                  Relative Y position of the voxel.
  34       1       1-2    dz                  Relative Z position of the voxel.
  35       1       1-2    space               Space requirements of the voxel.
  36                                          Total length of a voxel definition.
=======  ======  =======  ==================  ================================================================

The space requirements are defined as follows:

- bit 0: Northern quarter of the voxel is used by the piece.
- bit 1: Eastern quarter of the voxel is used by the piece.
- bit 2: Southern quarter of the voxel is used by the piece.
- bit 3: Western quarter of the voxel is used by the piece.

The remaining bits are reserved and should be ``0``.

Version history
...............

- 1 (20130317) Initial version.
- 2 (20130430) Entry and exit definitions, speed, flags, and sprites for other viewing directions added.

GUI
===
GUI sprites, in various forms.

All GUI sprites should use the BEIGE ranges, that is colours 214 to 225
(inclusive).

Generic GUI border sprites
~~~~~~~~~~~~~~~~~~~~~~~~~~
The most common form of a widget is a rectangular shape.
To draw such a shape, nine sprites are needed around the border of the
rectangle.

        +-------------+---------------+--------------+
        | top-left    | top-middle    | top-right    |
        +-------------+---------------+--------------+
        | left        | middle        | right        |
        +-------------+---------------+--------------+
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

That leads to the following block. FreeRCT can read these blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GBOR'.
   4       4    Version number of the block.
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

Version history
...............

- 1 (20111023) Initial version.
-   (20111029) Added 'panel' type.


Check box and radio buttons
~~~~~~~~~~~~~~~~~~~~~~~~~~~
FreeRCT can read blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GCHK'.
   4       4    Version number of the block.
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

Version history
...............

- 1 (20111023) Initial version.


Slider-bar elements
~~~~~~~~~~~~~~~~~~~
For slider-bar GUI elements, the following block should be used.
FreeRCT can read blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'GSLI'.
   4       4    Version number of the block.
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

Version history
...............

- 1 (20111023) Initial version.


Scroll-bar elements
~~~~~~~~~~~~~~~~~~~
For scroll-bar GUI elements, the following block should be used.
FreeRCT can read blocks with version 1.

======  ======  ==================  ================================================================
Offset  Length  Field name          Description
======  ======  ==================  ================================================================
   0       4                        Magic string 'GSCL'.
   4       4                        Version number of the block.
   8       4                        Length of the block excluding magic string, version, and length.
  12       1    min_length          Minimal length scrollbar.
  13       1    step_back           Stepsize of background.
  14       1    min_bar_length      Minimal length bar.
  15       1    bar_step            Stepsize of bar.
  16       2    widget_type         Widget type.
  18       4    left_button         Left/up button.
  22       4    right_button        Right/down button.
  26       4    left_pressed        Left/up pressed button.
  30       4    right_pressed       Right/down pressed button.
  34       4    left_bottom         Left/top bar bottom (the background).
  38       4    middle_bottom       Middle bar bottom (the background).
  42       4    right_bottom        Right/down bar bottom (the background).
  46       4    left_top            Left/top bar top.
  50       4    middle_top          Middle bar top.
  54       4    right_top           Right/down bar top.
  58       4    left_top_pressed    Left/top pressed bar top.
  62       4    middle_top_pressed  Middle pressed bar top.
  66       4    right_top_pressed   Right/down pressed bar top.
  70                                Total length.
======  ======  ==================  ================================================================

Known scroll-bar widget types:

- 160 Horizontal scroll bar + button.
- 161 Shaded horizontal scroll bar + button.
- 176 Vertical scroll bar + button.
- 177 Shaded vertical scroll bar + button.

Version history
...............

- 1 (20111023) Initial version.

Gui button sprites
==================
Sprites for use at buttons in the gui.

Gui Sprites
~~~~~~~~~~~
Several elements come with different slopes, and the user needs to select the
right one. Similarly, there are rotation sprites and texts that are displayed
in the gui.
FreeRCT can read blocks with version 4.

======  ======  =======  ==================  ================================================================
Offset  Length  Version  Field name          Description
======  ======  =======  ==================  ================================================================
   0       4      1-4                        Magic string 'GSLP' (Gui sprites).
   4       4      1-4                        Version number of the block.
   8       4      1-4                        Length of the block excluding magic string, version, and length.
  12       4      1-4    vert_down           Slope going vertically down.
  16       4      1-4    steep_down          Slope going steeply down.
  20       4      1-4    gentle_down         Slope going gently down.
  24       4      1-4    level               Level slope.
  28       4      1-4    gentle_up           Slope going gently up.
  32       4      1-4    steep_up            Slope going steeply up.
  36       4      1-4    vert_up             Slope going vertically up.
  40       4      2-4    pos_2d              Flat rotation positive direction (counter clock wise).
  44       4      2-4    neg_2d              Flat rotation negative direction (clock wise).
  48       4      2-4    pos_3d              Diametric rotation positive direction (counter clock wise).
  52       4      2-4    neg_3d              Diametric rotation negative direction (clock wise).
  56       4      3-4    close_button        Close Button.
  --       4       3                         Maximise button.
  --       4       3                         Minimise button.
  60       4       4     terraform_dot       Terraform dot.
  64       4      2-4    texts               Text of the guis (reference to a TEXT block).
  68                                         Total length.
======  ======  =======  ==================  ================================================================

Version history
...............

- 1 (20120612) Initial version.
- 2 (20120901) Moved GROT data into the block.
- 3 (20121110) Added close, maximise, and minimise buttons.
- 4 (20121202) Added terraform dot, removed maximise and minimise buttons.


Persons
=======
Persons are an important concept in the game. Their properties are defined in
the game blocks below.
FreeRCT can read blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'PRSG' (Person Graphics).
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       1    Number of person graphics in this block (called 'n').
  13     n*13   Graphics definitions of person types in this block.
   ?            Total length.
======  ======  ==========================================================

The person graphics of a person type is a set of colour range
recolourings.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       1    Person type being defined.
   1       4    First recolouring.
   5       4    Second recolouring.
   9       4    Third recolouring.
  13            Total length.
======  ======  ==========================================================

A person type defines the kind of persons:

- *Any* (0) Any kind of person (eg persons are not shown).
- *Pillar* (8) Guests from the Pillar Planet (test graphics).
- *Earth* (16) Earth-bound persons.

The *any* kind is used as fall back.

Recolouring definition
~~~~~~~~~~~~~~~~~~~~~~
The program has 18 colour ranges (0 to 17). A recolouring is a mapping of a
single range to a set of allowed destination ranges, encoded in 32 bit. Bits
24-31 state the single range (where a value other than 0..17 denotes an unused
recolouring), Each bit `i` in the range of bits 0..17 denotes whether range `i`
is allowed as replacement.


Version history
...............

- 1 (20120708) Initial version.


Animation
=========
Animations have two layers. The conceptual definition is in an 'ANIM'
block. This definition contains the number of frames the timing, and the
change in x and/or y position. These changes are in the internal voxel
coordinate system (256 units to get from one side to the opposite side).

The sprites associated with an animation (at a tile width) are in 'ANSP'
blocks. The latter get erased when the former is defined.
Since the 'ANIM' sequence has to be useful for the largest tile width, for
smaller tile sizes, an animation may contain more frames than really needed.
Also, some changes in x or y may not be visible as they are in the sub-pixel
range at the smaller tile size. The expected (and allowed) solution can be to
display the same sprite in more frames.


Animation sequences
~~~~~~~~~~~~~~~~~~~

Animation sequences (without the sprites) are defined using the 'ANIM' block.
FreeRCT can read blocks with version 2.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'ANIM'.
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       1    Person type.
  13       2    Animation type.
  15       2    Frame count (called 'f').
  17      f*6   Data of all frames.
   ?            Variable length.
======  ======  ==========================================================

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
   0       2    Duration of the frame in milli seconds.
   2       2    (signed) X position change after displaying the frame.
   4       2    (signed) Y position change after displaying the frame.
   6            Total length.
======  ======  ==========================================================

Position changes are in the 256 unit inside-voxel coordinate system.The z
position is derived from the world data.


Sprites of an animation sequence for a given tile width are then in an 'ANSP'
block, defined below. The frame count should match with the count in the
'ANIM' block.

Version history
...............

- 1 (20120418) Initial version.
- 2 (20120527) Removed tile width from 'ANIM' and sprite and frame number from the frame data.
  The sprite moved to the 'ANSP' block.

Animation sprites
~~~~~~~~~~~~~~~~~
FreeRCT can read blocks with version 1.

======  ======  ==========================================================
Offset  Length  Description
======  ======  ==========================================================
   0       4    Magic string 'ANSP'.
   4       4    Version number of the block.
   8       4    Length of the block excluding magic string, version, and
                length.
  12       2    Zoom-width of a tile.
  14       1    Person type.
  15       2    Animation type.
  17       2    Frame count (called 'f').
  19      f*4   Sprite for each frame.
   ?            Variable length.
======  ======  ==========================================================

Version history
...............

- 1 (20120527) Initial version.


Obsolete blocks
===============

The following blocks existed once, but are not needed any more

==== =====================  =====================================================================
Name Version                Description
==== =====================  =====================================================================
8PAL (20110915)-(20110925)  8bpp palette data (data is hard-coded in the program).
SPRT (20110915)-(20120623)  X and Y offset of a sprite (data has been moved to the 8PXL block).
GROT (20120612)-(20120901)  Rotation GUI sprites (data has been moved to the GSLP block)
==== =====================  =====================================================================

.. vim: set spell
