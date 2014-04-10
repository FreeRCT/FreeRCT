:Author: The FreeRCT team
:Version: $Id$

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
  12              1-     Current date block
                  1-     Current random number block
======  ======  =======  ======================================================


File header
-----------
The file header consists of 3 parts. Current version number is 1.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "FCTS".
   4       4      1-     Version number of the file header.
   8       4      1-     "STCF"
  12                     Total size.
======  ======  =======  ======================================================

Version history
~~~~~~~~~~~~~~~

- 1 (20140410) Initial version.


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

.. vim: spell
