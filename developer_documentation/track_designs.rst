:Author: The FreeRCT Team
:Version: 2022-10-05

.. contents::
   :depth: 2

####################
FTK data file format
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
This file documents the FTK file format used by FreeRCT to store user-defined track designs for roller coasters and other tracked rides.

File Layout
===========

File Header
~~~~~~~~~~~

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "FTKD".
   4       4      1-     Version number of the file.
   8       ?      1-     Internal name of the ride.
   ?       ?      1-     Name of the design.
   ?       4      1-     Excitement rating.
   ?       4      1-     Intensity rating.
   ?       4      1-     Nausea rating.
   ?       4      1-     Number n of track pieces.
   ?     18*n     1-     Every `track piece`_.
   ?       4      1-     "DKTF".
======  ======  =======  ======================================================


Track Piece
~~~~~~~~~~~

======  ======  =======  ===========================================================================
Offset  Length  Version  Description
======  ======  =======  ===========================================================================
   0       4      1-     "trpc".
   4       4      1-     Track piece ID.
   8       2      1-     X coordinate of the piece, relative to the coaster's starting voxel.
  10       2      1-     Y coordinate of the piece, relative to the coaster's starting voxel.
  12       2      1-     Z coordinate of the piece, relative to the coaster's starting voxel.
  14       4      1-     "cprt".
  18              1-     Total length.
======  ======  =======  ===========================================================================

Version History
~~~~~~~~~~~~~~~

- 1 (20221005) Initial version.
