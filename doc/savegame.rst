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
  12      16      1-     Current date block
  28      16      1-     Current random number block
  44       ?      2-     Current financial data.
   ?                     Total length of the save file.
======  ======  =======  ======================================================


File header
-----------
The file header consists of 3 parts. Current version number is 2.

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
 112
======  ======  =======  ======================================================

Version history
~~~~~~~~~~~~~~~

- 1 (20140419) Initial version.


.. vim: spell
