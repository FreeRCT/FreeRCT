:Author: The FreeRCT team
:Version: 2021-04-03

.. contents::
   :depth: 4

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

.. note:: The file format changed significantly between the file header versions 9 and 10.
          This document covers only the format from header version 10 onwards.
          Documentation for older format versions can be found in the file "savegame_history.rst".

The file starts with a file header to identify the file as being a save game.
After the file header come data patterns of game elements that are stored.

All data in the file is organized in **patterns**. Each pattern starts by
specifying a version number followed by the pattern's data layout as
detailed below. A pattern can contain other nested patterns. A pattern
which is not nested in any other pattern is called a **block**.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       ?     10-     File header
   ?       ?     10-     Current date block.
   ?       ?     10-     Current basic world block.
   ?       ?     10-     Current financial data.
   ?       ?     10-     Current weather block.
   ?       ?     10-     Rides block.
   ?       ?     10-     Scenery block.
   ?       ?     10-     Guests block.
   ?       ?     10-     Staff block.
   ?       ?     10-     Inbox block.
   ?       ?     10-     Current random number block.
   ?                     Total length of the save file.
======  ======  =======  ======================================================


File header
-----------
The file header consists of 3 parts. Current version number is 10.

Header Layout
~~~~~~~~~~~~~

======  ======  ======================================================
Offset  Length  Description
======  ======  ======================================================
   0       4    "FCTS".
   4       4    Version number of the file.
   8       4    "STCF"
  12            Total size.
======  ======  ======================================================

Version history
...............

- (older versions are documented in the file "savegame_history.rst").
- 10 (20210402) Refactored handling of versions.


Nested patterns
---------------
Miscellaneous data layouts which are reused in multiple places.

Voxel object
~~~~~~~~~~~~
Basic information for a moveable object.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       4      1-     Merged x coordinate.
   8       4      1-     Merged y coordinate.
  12       4      1-     Merged z coordinate.
  16                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Person
~~~~~~
Basic information for a person.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     Voxel object data.
   ?       1      1-     Person type.
   ?       2      1-     Offset with respect to center of path/tile.
   ?       ?      1-     Name characters.
   ?       ?      1-     Recolour information.
   ?       2      1-     Current walk information (animation), in compressed format.
   ?       2      1-     Current displayed frame of the animation.
   ?       2      1-     Remaining displayed time of the current frame.
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Guest
~~~~~
A single guest.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   ?       ?      1-     Person data.
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
   ?       4      2-     Preferred ride intensity.
   ?       4      2-     Minimum ride intensity.
   ?       4      2-     Maximum ride intensity.
   ?       4      2-     Maximum ride nausea.
   ?       4      2-     Minimum ride excitement.
                         Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210402) Added ride rating preferences.


Ride Instance
~~~~~~~~~~~~~
Basic information for a single ride instance.

======  ======  =======  ===========================================================
Offset  Length  Version  Description
======  ======  =======  ===========================================================
   0       4      1-     Version number.
   4       ?      1-     Ride name characters.
   ?       2      1-     Ride state and flags.
   ?       2      1-     Ride entrance type ID.
   ?       2      1-     Ride exit type ID.
   ?       ?      1-     Ride recolour information.
   ?       ?      1-     Entrance recolour information.
   ?       ?      1-     Exit recolour information.
   ?       ?      1-     Every sold item's price (8 byte each).
   ?       ?      1-     Every sold item's count (8 byte each).
   ?       8      1-     Total profit of the ride.
   ?       8      1-     Total profit of selling items.
   ?       2      1-     Current reliability.
   ?       2      1-     Current maximum reliability.
   ?       4      1-     Ride maintenance interval.
   ?       4      1-     Time since last maintenance.
   ?       1      1-     1 if the ride is broken; otherwise 0.
   ?       1      1-     1 if a mechanic has been requested; otherwise 0.
   ?       4      1-     Time since the message about a long queue was last sent.
   ?       4      1-     Excitement rating.
   ?       4      1-     Intensity rating.
   ?       4      1-     Nausea rating.
                         Total size.
======  ======  =======  ===========================================================

Version history
...............

- 1 (20210402) Initial version.


Display Coaster Car
~~~~~~~~~~~~~~~~~~~
One piece of a coaster car.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     Voxel object data.
   ?       1      1-     Current car pitch.
   ?       1      1-     Current car roll.
   ?       1      1-     Current car yaw.
                         Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster Car
~~~~~~~~~~~
One car of a coaster train.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     The front Display Coaster Car.
   ?       ?      1-     The back Display Coaster Car.
   ?       4      1-     Number of guests in the car.
   ?      4*?     1-     The ID of every guest in the car.
                         Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster Train
~~~~~~~~~~~~~
One train of a coaster.

======  ======  =======  ========================================================
Offset  Length  Version  Description
======  ======  =======  ========================================================
   0       4      1-     Version number.
   4       ?      1-     The data of every car in the train.
                         The number of cars is stored in the coaster instance.
   ?       4      1-     The train's position along the track.
   ?       4      1-     The current speed.
   ?       1      1-     The train's current station policy.
   ?       4      1-     The number of milliseconds left to wait in the station.
                         Total size.
======  ======  =======  ========================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster stations
~~~~~~~~~~~~~~~~
One station of a coaster.

======  ======  ========================  ============================================================
Offset  Length  Coaster Instance Version  Description
======  ======  ========================  ============================================================
   0       2      1-                      X coordinate of the entrance.
   2       2      1-                      Y coordinate of the entrance.
   4       2      1-                      Z coordinate of the entrance.
   6       2      1-                      X coordinate of the exit.
   8       2      1-                      Y coordinate of the exit.
  10       2      1-                      Z coordinate of the exit.
  12       1      1-                      Station direction.
  13       4      1-                      Station length.
  17       4      1-                      Station start position.
  21       4      1-                      Number of voxels occupied by the station.
  25       ?      1-                      For each voxel: The x, y, and z coordinate (2 bytes each).
   ?                                      Total size.
======  ======  ========================  ============================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster intensity statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
A single coaster intensity statistics data point.

======  ======  ========================  ======================================================
Offset  Length  Coaster Instance Version  Description
======  ======  ========================  ======================================================
   0       4      1-                      Position along the track.
   4       1      1-                      Whether this data point is valid (1 or 0).
   5       4      1-                      Data point precision.
   9       4      1-                      Average train speed.
  13       4      1-                      Average vertical G force.
  17       4      1-                      Average horizontal G force.
  21                                      Total size.
======  ======  ========================  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster
~~~~~~~
A coaster instance.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     Ride instance data.
   0       4      1-     Number of positioned track pieces.
   ?       4      1-     Total length of the roller coaster (in 1/256 pixels).
   ?       2      1-     Number of placed track pieces.
   ?       ?      1-     Contents of "number" placed track pieces.
   ?       4      1-     Number of trains in this coaster.
   ?       4      1-     Number of cars in a single train.
   ?       ?      1-     Data of each train
   ?       4      1-     Maximum idle duration in milliseconds.
   ?       4      1-     Minimum idle duration in milliseconds.
   ?       4      1-     Number of stations.
   ?       ?      1-     Each station's data.
   ?       4      1-     Number of intensity statistics data points.
   ?       ?      1-     Each intensity statistics data point.
                         Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Fixed Ride Instance
~~~~~~~~~~~~~~~~~~~
Basic information for a single fixed ride instance.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     Ride instance data.
   ?       1      1-     Ride orientation.
   ?       2      1-     X coordinate of the ride base position.
   ?       2      1-     Y coordinate of the ride base position.
   ?       2      1-     Z coordinate of the ride base position.
   ?       2      1-     Number of working cycles.
   ?       4      1-     Minimum idle duration.
   ?       4      1-     Maximum idle duration.
   ?       4      1-     Time left in the current working phase.
   ?       1      1-     1 if the ride is working; otherwise 0.
   ?       ?      1-     Onride guests data.
                         Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Onride guests
~~~~~~~~~~~~~
Holds data about all the guests on a ride.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       2      1-     The size of a batch.
   6       2      1-     The number of batches.
   8       ?      1-     Every batch's onride guests batch data.
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Onride guests batch
~~~~~~~~~~~~~~~~~~~
Holds data about one batch of guests on a ride.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       1      1-     The batch's state.
   5       4      1-     The remaining running time.
   9       2      1-     The batch's entry information.
  11       ?      1-     The onride guests data for each guest.
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Onride guests data
~~~~~~~~~~~~~~~~~~
Holds data about one individual guest on a ride.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       4      1-     The guest's ID.
   8       1      1-     The guest's entry information.
   9                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Shop Instance
~~~~~~~~~~~~~
Information for a single shop instance.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     The fixes ride instance data.
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Gentle/Thrill Ride Instance
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Information for a single instance of a gentle or thrill ride.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       ?      1-     The fixes ride instance data.
   ?       2      1-     The entrance's x coordinate.
   ?       2      1-     The entrance's y coordinate.
   ?       2      1-     The entrance's z coordinate.
   ?       2      1-     The exit's x coordinate.
   ?       2      1-     The exit's y coordinate.
   ?       2      1-     The exit's z coordinate.
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Scenery Instance
~~~~~~~~~~~~~~~~
Information for a single scenery item instance.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       2      1-     X base coordinate.
   6       2      1-     Y base coordinate.
   8       2      1-     Z base coordinate.
  10       1      1-     The item's orientation.
  11       4      1-     The time in the animation.
  15       4      1-     The time since the item was last watered.
  19                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Message
~~~~~~~
Information for a single message in the player's inbox.

======  ======  =======  ============================================================================================================
Offset  Length  Version  Description
======  ======  =======  ============================================================================================================
   0       4      1-     Version number.
   4       2      1-     The message string ID.
   6       4      1-     The first data item.
  10       4      1-     The second data item.
  14       4      1-     The timestamp in compressed date format (see the Date block for information on this format).
  18                     Total size.
======  ======  =======  ============================================================================================================

Version history
...............

- 1 (20210402) Initial version.


Recolouring
~~~~~~~~~~~
Represents a recolouring specification.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       1      1-     The palette index of the first colour.
   5       1      1-     The palette index of the second colour.
   6       1      1-     The palette index of the third colour.
   7       1      1-     The palette index of the fourth colour.
   8                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Voxel
~~~~~
Represents a single voxel.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     Version number.
   4       4      1-     Ground type.
   8       1      1-     Instance type.
   9      0/2     1-     Instance data (skipped for voxels without a ride instance).
   ?       2      1-     Fence bits.
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Voxel Stack
~~~~~~~~~~~
Represents a voxel stack.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "VSTK".
   4       4      1-     Version number.
   8       2      1-     Base height.
  10       2      1-     Stack height.
  12       1      1-     Owner type.
  13       ?      1-     Every voxel's data pattern.
   ?       4      1-     "KTSV"
   ?                     Total size.
======  ======  =======  ======================================================

Version history
...............

- (older versions are documented in the file "savegame_history.rst").
- 3 (20150428) Fences near the lowest corner of a steep slope moved from top voxel to base voxel.


Blocks
------
Top-level data layouts which are present once in every savegame file.

Date
~~~~
Stores the current date of the game.

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
...............

- 1 (20140410) Initial version.


Random
~~~~~~
Stores the current random seed.

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
...............

- 1 (20140410) Initial version.


Weather
~~~~~~~
Stores the current weather.

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
...............

- 1 (20150505) Initial version.


Guests
~~~~~~
Stores all guests in the game.

======  ======  =======  ==============================================================
Offset  Length  Version  Description
======  ======  =======  ==============================================================
   0       4      1-     "GSTS".
   4       4      1-     Version number of the guests block.
   8       2      1-     Start voxel x coordinate.
  10       2      1-     Start voxel y coordinate.
  12       2      1-     Frame counter.
  14       2      1-     Next guest (index) to animate.
  16       4      1-     Lowest 'free' index for next new guest.
  20       4      1-     Number of active guests.
  24       ?      1-     Contents of "number" active guests. Each guest is stored as
                         his unique ID (2 bytes) followed by the Guest data pattern.
   ?       4      1-     "STSG"
======  ======  =======  ==============================================================

Version history
...............

- 1 (20150823) Initial version.


Staff
~~~~~
Stores all the staff in the game.

======  ======  =======  ==============================================================
Offset  Length  Version  Description
======  ======  =======  ==============================================================
   0       4      1-     "STAF".
   4       4      1-     Version number of the staff block.
   8       4      1-     Number of pending mechanic requests.
  12       ?      1-     Every mechanic requests's ride ID (2 bytes each).
   ?       4      1-     "FATS"
======  ======  =======  ==============================================================

Version history
...............

- 1 (20210402) Initial version.


Finances
~~~~~~~~
Stores the historic information about income and payments,
as well as the current loan and amount of available cash.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "FINA".
   4       4      1-     Version number of the financial block.
   8       1      1-     Number of available history sections.
   9       1      1-     Index into the current financial data bock.
  10       8      1-     Current cash.
  18     ?*112           'number available' history sections, see below.
  12       4      1-     "ANIF".
  16                     Total size.
======  ======  =======  ======================================================

A history section is structured as follows:

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
...............

- 1 (20210402) Initial version.


Inbox
~~~~~
Stores all messages in the player's inbox.

======  ======  =======  ==============================================================
Offset  Length  Version  Description
======  ======  =======  ==============================================================
   0       4      1-     "INBX".
   4       4      1-     Version number of the staff block.
   8       4      1-     Number of messages.
  12       ?      1-     Every message's data pattern.
   ?       4      1-     "XBNI"
======  ======  =======  ==============================================================

Version history
...............

- 1 (20210402) Initial version.


World
~~~~~
The basic world block contains voxel information about ground, foundations, and
small rides (paths etc). Voxel data of full rides and voxel objects are not
stored here, they are part of the full rides or persons.

======  ======  =======  ====================================================================
Offset  Length  Version  Description
======  ======  =======  ====================================================================
   0       4      1-     "WRLD".
   4       4      1-     Version number of the basic world block.
   8       2      1-     Length of the world in X direction.
  10       2      1-     Length of the world in Y direction.
  10       2      2-     Number of border fence override rules.
  12       ?      2-     Every border fence override rule. One rule consists of the x and
                         y coordinates (2 bytes each) followed by the tile edge (1 byte).
   ?       4      1-     "DLRW"
   ?       ?      1-     Voxel stack patterns.
======  ======  =======  ====================================================================

The voxel stack blocks store each voxel stack of the world, starting at
coordinate ``(0, 0)`` and ending at ``(max_x, max_y)``. The ``y`` coordinate
runs fastest.

Version history
...............

- (older versions are documented in the file "savegame_history.rst").
- 2 (20210402) Added border fence override rules.


Scenery
~~~~~~~
All placed scenery instances in the world.

======  ======  =======  =========================================================================
Offset  Length  Version  Description
======  ======  =======  =========================================================================
   0       4      1-     "SCNY".
   4       4      1-     Version number of the staff block.
   8       4      1-     Number of scenery items.
  12       ?      1-     Every item's type index followed by its scenery instance data pattern.
   ?       4      1-     "YNCS"
======  ======  =======  =========================================================================

Version history
...............

- 1 (20210402) Initial version.


Rides
~~~~~
Stores all rides.

======  ======  =======  ======================================================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================================================
   0       4      1-     "RIDS".
   4       4      1-     Version number of the rides block.
   8       2      1-     Number of rides.
  10       ?      1-     Every ride's content, consisting of the ride type kind (1 byte), the ride type name
                         characters, and the data pattern of the ride instance's most derived class.
   ?       4      1-     "SDIR"
======  ======  =======  ======================================================================================

Version history
...............

- 1 (20210402) Initial version.


.. vim: spell
