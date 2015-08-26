:Author: The FreeRCT team
:Version: 2015-05-05

.. contents::
   :depth: 3

############################
File format of the save game
############################

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
This file documents the file format used by FreeRCT for saving games.

The file starts with a file header to identify the file as being a save game.
After the file header come data blocks of game elements that are stored.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0      12      1-     File header
  12      16      1-     Current date block
  28       ?      3-     Current basic world block.
   ?      16      1-     Current random number block
   ?       ?      2-     Current financial data.
   ?      28      4-     Current weather block.
   ?                     Total length of the save file.
======  ======  =======  ======================================================


File header
-----------
The file header consists of 3 parts. Current version number is 4.

======  ======  ======================================================
Offset  Length  Description
======  ======  ======================================================
   0       4    "FCTS".
   4       4    Version number of the file.
   8       4    "STCF"
  12            Total size.
======  ======  ======================================================

Version history
~~~~~~~~~~~~~~~

- 1 (20140410) Initial version.
- 2 (20140419) Added financial data.
- 3 (20140419) Added basic world data.
- 4 (20150505) Added weather data.


Current date block
------------------
The date block stores the current date. Current version is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "DATE".
   4       4      1-     Version number of the date block.
   8       4      1-     Current date, in compressed format.
  12       4      1-     "ETAD"
  16                     Total size.
======  ======  =======  ======================================================

where compressed format is an unsigned 32 bit number, with

- bit 0..4  day
- bit 5..8  month
- bit 9..15 year
- bit 16..25 fraction of the day.

Version history
~~~~~~~~~~~~~~~

- 1 (20140410) Initial version.


Random number block
-------------------
The random number block stores the current random seed. Current version is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "RAND".
   4       4      1-     Version number of the random number block.
   8       4      1-     Current random number.
  12       4      1-     "DNAR".
  16                     Total size.
======  ======  =======  ======================================================

Version history
~~~~~~~~~~~~~~~

- 1 (20140410) Initial version.


Financial block
---------------
The financial block stores the historic information about income and payments,
as well as the current loan and amount of available cash.
Current version of the block is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "FINA".
   4       4      1-     Version number of the financial block.
   8       1      1-     Number of available history data blocks.
   9       1      1-     Index into the current financial data bock.
  10       8      1-     Current cash.
  18     ?*112           'number available' history blocks, see below.
  12       4      1-     "ANIF".
  16                     Total size.
======  ======  =======  ======================================================

A history block looks like

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       8      1-     Construction costs of rides.
   8       8      1-     Running cost of rides.
  16       8      1-     Land purchase costs.
  24       8      1-     Landscaping costs.
  32       8      1-     Income from entrance tickets.
  40       8      1-     Income from ride tickets.
  48       8      1-     Income from non-food shop sales.
  56       8      1-     Stock costs from non-food shops.
  64       8      1-     Income from food shop sales.
  72       8      1-     Stock costs from food shops.
  80       8      1-     Wages of staff payments.
  88       8      1-     Marketing costs.
  96       8      1-     Research costs.
 104       8      1-     Loan interest.
 112                     Total length.
======  ======  =======  ======================================================

Version history
~~~~~~~~~~~~~~~

- 1 (20140419) Initial version.


Basic world block
-----------------
The basic world block contains voxel information about ground, foundations, and
small rides (paths etc). Voxel data of full rides and voxel objects are not
stored here, they are part of the full rides or persons. Current version of the
basic world block is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "WRLD".
   4       4      1-     Version number of the basic world block.
   8       2      1-     Length of the world in X direction.
  10       2      1-     Length of the world in Y direction.
  12       4      1-     "DLRW"
  16       ?      1-     Voxel stack blocks.
======  ======  =======  ======================================================

The voxel stack blocks store each voxel stack of the world, starting at
coordinate ``(0, 0)`` and ending at ``(max_x, max_y)``. The ``y`` coordinate
runs fastest.

Version history
~~~~~~~~~~~~~~~

- 1 (20140419) Initial version.


Voxel stack block
-----------------
A voxel stack block saves all voxels at a single ``(x, y)`` coordinate. Current
block number is 2, which has the following layout.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "VSTK".
   4       4      1-     Version number of the voxel stack block.
   8       2      1-     Height of bottom voxel of the stack.
  10       2      1-     Number of voxels available in this stack.
  12       1      1-     Owner of this park tile.
  13    ?*6/7     1-     Contents of "number" voxels.
   ?       4      1-     "KTSV"
======  ======  =======  ======================================================

A single voxel is stored as follows:

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Ground (+ slope + foundation + grass-length)
   4       1      1-     Instance for small rides, or 'free'.
   5      0/1     1-     If small ride instance, its instance data, else
                         this field is skipped.
  5/6      2      2-     Fence data.
======  ======  =======  ======================================================


Version history
~~~~~~~~~~~~~~~

- 1 (20140419) Initial version.
- 2 (20150410) Added fence data.
- 3 (20150428) Fences near the lowest corner of a steep slope moved from top voxel to base voxel.


Current weather block
-------------
The weather block stores the current weather. Current version is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "WTHR".
   4       4      1-     Version number of the weather block.
   8       4      1-     Current temperature, in 1/10 degrees Celsius.
  12       4      1-     Current weather type.
  16       4      1-     Next weather type.
  20       4      1-     Speed of change in the weather.
  24       4      1-     "RHTW"
  28                     Total size.
======  ======  =======  ======================================================

Version history
~~~~~~~~~~~~~~~

- 1 (20150505) Initial version.


Current guests block
-------------
The guests block stores all guests. Current version is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "GSTS".
   4       4      1-     Version number of the guests block.
   8       2      1-     Start voxel x coordinate.
  10       2      1-     Start voxel y coordinate.
  12       2      1-     Frame counter.
  14       2      1-     Next guest (index) to animate.
  16       4      1-     Lowest 'free' index for next new guest.
  20       4      1-     Number of active guests.
  24       ?      1-     Contents of "number" active guests.
   ?       4      1-     "STSG"
======  ======  =======  ======================================================

A single guest is stored as follows:

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       2      1-     Unique id of the guest.
   2       4      1-     Voxel and pixel position x coordinate values.
   6       4      1-     Voxel and pixel position y coordinate values.
  10       4      1-     Voxel and pixel position z coordinate values.
  14       1      1-     Type of the person.
  15       2      1-     Offset with respect to center of path/tile.
  17       4      1-     Length of the name string.
  21      ?*4     1-     Name characters.
   ?       4      1-     Recolour information.
   ?       2      1-     Current walk information (animation), in compressed format.
   ?       2      1-     Current displayed frame of the animation.
   ?       2      1-     Remaining displayed time of the current frame.
   ?       1      1-     Current activity.
   ?       2      1-     Current happiness.
   ?       2      1-     Sum of happiness for calculations once guest goes home.
   ?       8      1-     Cash on hand.
   ?       8      1-     Cash spent.
   ?       2      1-     Ride index.
   ?       1      1-     Whether or not the guest has a map.
   ?       1      1-     Whether or not the guest has an umbrella.
   ?       1      1-     Whether or not the guest has a food/drink wrapper.
   ?       1      1-     Whether or not the guest has a balloon.
   ?       1      1-     Whether or not the held food is salty.
   ?       1      1-     Number of souvenirs bought by the guest.
   ?       1      1-     Number of food units held.
   ?       1      1-     Number of drink units held.
   ?       1      1-     Hunger level.
   ?       1      1-     Thirst level.
   ?       1      1-     Stomach fill level.
   ?       1      1-     Waste level.
   ?       1      1-     Nausea level.
======  ======  =======  ======================================================

Walks on a path tile are stored in a 16 bit number, which contains the following information.

- bit 12..15 Normal (0) or centered (1) path tile walk.
- bit 8..11  The entry edge.
- bit 4..7   The exit edge.
- bit 0..3   The number of 90 degrees turns.

A normal walk uses the 'offset' of the person to make it move in the right area
of the tile (and the opposing direction uses the left area of the tile. A
centered walk is like a normal walk, but the person is gradually moved onto the
center of the path, to form a queue.

Within a tile, a person enters from the entry edge, and leaves at the exit
edge. (With 0=NE, 1=SE, 2=SW, and 3=NW for all edges.) If the walk at the tile
requires a change in direction, one or more 90 degrees turns are made around
the center of the tile (while respecting the offset in case of normal tile
walks), in counter-clockwise direction.

Version history
~~~~~~~~~~~~~~~

- 1 (20150823) Initial version.

.. vim: spell
