:Author: The FreeRCT team
:Version: 2021-04-26

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

All data in the file is organized in **patterns**. Every pattern starts with a
magic string (4 bytes) and a version number (4 bytes), followed by the pattern's
data layout as detailed below. The pattern is terminated by the magic string in
reverse.

A pattern can contain other nested patterns. A pattern which is not
nested in any other pattern is called a **block**.

By convention, the magic strings of blocks consist of four uppercase characters, and
the magic strings of all other patterns consist of four lowercase characters.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       ?     10-     `File header`_.
   ?       ?     10-     Current date_ block.
   ?       ?     10-     Current basic world_ block.
   ?       ?     10-     Current finances_ block.
   ?       ?     10-     Current weather_ block.
   ?       ?     12-     `Game observer`_ block.
   ?       ?     10-     Rides_ block.
   ?       ?     10-     Scenery_ block.
   ?       ?     10-     Guests_ block.
   ?       ?     10-     Staff_ block.
   ?       ?     10-     Inbox_ block.
   ?       ?     10-     Current random_ number block.
======  ======  =======  ======================================================


File header
-----------
The file header contains basic information that should be accessible before loading the savegame.
Current version number is 12.

Header Layout
~~~~~~~~~~~~~

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "FCTS".
   4       4      1-     Version number of the file.
   8       8      11-    UNIX timestamp when the savegame was created.
  16       ?      11-    Version string with which the savegame was created.
           ?     11-11   Name of the scenario.
   ?       ?      12-    Scenario_ data.
   ?       4      1-     "STCF"
======  ======  =======  ======================================================

Version history
...............

- (older versions are documented in the file "savegame_history.rst").
- 10 (20210402) Refactored handling of versions.
- 11 (20220717) Added scenario data and savefile information.
- 12 (20220820) Added game observer data and extracted scenario data.


Nested patterns
---------------
Miscellaneous data layouts which are reused in multiple places.

Scenario
~~~~~~~~
Stores the scenario parameters.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "SCNO".
   4       4      1-     Version number.
   8       ?      1-     Name of the scenario.
   ?       ?      1-     The scenario's `objective container`_.
   ?       2      1-     Lowest guest spawn probability.
   ?       2      1-     Highest guest spawn probability.
   ?       4      1-     Maximum guest count.
   ?       4      1-     Initial amount of money.
   ?       4      1-     Initial loan.
   ?       4      1-     Maximum loan.
   ?       2      1-     Interest rate.
   ?       4      1-     "ONCS"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


Abstract objective
~~~~~~~~~~~~~~~~~~
Stores data common to all objective types.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "OJAO".
   4       4      1-     Version number.
   8       1      1-     Whether the objective is currently fulfilled (1 or 0).
   9       4      1-     The objective's drop policy in days.
  13       4      1-     The objective's current drop policy counter.
  17       4      1-     "OAJO"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


Objective container
~~~~~~~~~~~~~~~~~~~
An objective that groups one or more other objectives.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "OJCN".
   4       4      1-     Version number.
   8      21      1-     The `abstract objective`_ data.
  29       1      1-     The timeout policy.
  30       4      1-     The timeout date in compressed date format.
  34       4      1-     Number of sub-objectives.
  38       ?      1-     For each sub-objective, the objective's type (1 byte) followed
                         by the objective pattern for this type of objective.
   ?       4      1-     "NCJO"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


Empty objective
~~~~~~~~~~~~~~~
An empty objective.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "OJ00".
   4       4      1-     Version number.
   8      21      1-     The `abstract objective`_ data.
  29       4      1-     "00JO"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


Guests objective
~~~~~~~~~~~~~~~~
An objective to achieve a certain number of guests.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "OJGU".
   4       4      1-     Version number.
   8      21      1-     The `abstract objective`_ data.
  29       4      1-     The guest count to achieve.
  33       4      1-     "UGJO"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


Rating objective
~~~~~~~~~~~~~~~~
An objective to achieve a certain park rating.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "OJRT".
   4       4      1-     Version number.
   8      21      1-     The `abstract objective`_ data.
  29       2      1-     The rating to achieve.
  31       4      1-     "TRJO"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


Finance history
~~~~~~~~~~~~~~~
A single section in the finance manager history.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "fina".
   4       4      1-     Version number.
   8       8      1-     Construction costs of rides.
  16       8      1-     Running cost of rides.
  24       8      1-     Land purchase costs.
  32       8      1-     Landscaping costs.
  40       8      1-     Income from entrance tickets.
  48       8      1-     Income from ride tickets.
  56       8      1-     Income from non-food shop sales.
  64       8      1-     Stock costs from non-food shops.
  72       8      1-     Income from food shop sales.
  80       8      1-     Stock costs from food shops.
  88       8      1-     Wages of staff payments.
  96       8      1-     Marketing costs.
 104       8      1-     Research costs.
 112       8      1-     Loan interest.
 116       4      1-     "anif".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Voxel object
~~~~~~~~~~~~
Basic information for a moveable object.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "vxoj".
   4       4      1-     Version number.
   8       4      1-     Merged x coordinate.
  12       4      1-     Merged y coordinate.
  16       4      1-     Merged z coordinate.
  20       4      1-     "joxv".
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
   0       4      1-     "prsn".
   4       4      1-     Version number.
   8       ?      1-     `Voxel object`_ data.
   ?       1      1-     Person type.
   ?       2      1-     Offset with respect to center of path/tile.
   ?       ?      1-     Name characters.
   ?       2      2-     Ride index.
   ?       ?      1-     Recolouring_ information.
   ?       2      1-     Current walk information (animation), in compressed format.
   ?       2      1-     Current displayed frame of the animation.
   ?       2      1-     Remaining displayed time of the current frame.
   ?       2      3-     The person's current status.
   ?       4      1-     "nsrp".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210426) Moved ride index of guests and mechanics to Person.
- 3 (20210509) Moved status of staff members to Person.


Guest
~~~~~
A single guest.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "gues".
   4       4      1-     Version number.
   ?       ?      1-     Person_ data.
   ?       1      1-     Current activity.
   ?       2      1-     Current happiness.
   ?       2      1-     Sum of happiness for calculations once guest goes home.
   ?       8      1-     Cash on hand.
   ?       8      1-     Cash spent.
   ?       2      1-2    Ride index.
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
   ?       4      1-     "seug".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210402) Added ride rating preferences.
- 3 (20210426) Moved ride index to Person data.


Staff member
~~~~~~~~~~~~
A single staff member.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "stfm".
   4       4      1-     Version number.
   ?       ?      1-     Person_ data.
           2      1-1    The person's current status.
   ?       4      1-     "mfts".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210426) Initial version.
- 2 (20210509) Moved status of staff members to Person.


Mechanic
~~~~~~~~
A single mechanic.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "mchc".
   4       4      1-     Version number.
   ?       ?      1-1    Person_ data.
   ?       ?      2-     `Staff Member`_ data.
   ?       2      1-1    Ride index.
   ?       4      1-     "chcm".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210423) Initial version.
- 2 (20210426) Added Staff Member pattern, and moved ride index to Person data.


Handyman
~~~~~~~~
A single handyman.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "hndy".
   4       4      1-     Version number.
   ?       ?      1-     `Staff Member`_ data.
   ?       1      1-     Current activity.
   ?       4      1-     "ydnh".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210426) Initial version.


Guard
~~~~~
A single security guard.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "gard".
   4       4      1-     Version number.
   ?       ?      1-     `Staff Member`_ data.
   ?       4      1-     "drag".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210426) Initial version.


Entertainer
~~~~~~~~~~~
A single entertainer.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "etai".
   4       4      1-     Version number.
   ?       ?      1-     `Staff Member`_ data.
   ?       4      1-     "iate".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210426) Initial version.


Ride Instance
~~~~~~~~~~~~~
Basic information for a single ride instance.

======  ======  =======  ===========================================================
Offset  Length  Version  Description
======  ======  =======  ===========================================================
   0       4      1-     "ride".
   4       4      1-     Version number.
   8       ?      1-     Ride name characters.
   ?       2      1-     Ride state and flags.
           2      1-1    Ride entrance type ID.
           2      1-1    Ride exit type ID.
   ?       2      2-     Ride entrance type internal name.
   ?       2      2-     Ride exit type internal name.
   ?       ?      1-     Ride recolouring_ information.
   ?       ?      1-     Entrance recolouring_ information.
   ?       ?      1-     Exit recolouring_ information.
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
   ?       4      1-     "edir".
======  ======  =======  ===========================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20220829) Use internal name for entrances and exits.


Display Coaster Car
~~~~~~~~~~~~~~~~~~~
One piece of a coaster car.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "dpcc".
   4       4      1-     Version number.
   8       ?      1-     `Voxel object`_ data.
   ?       1      1-     Current car pitch.
   ?       1      1-     Current car roll.
   ?       1      1-     Current car yaw.
   ?       4      1-     "ccpd".
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
   0       4      1-     "cstc".
   4       4      1-     Version number.
   8       ?      1-     The front Display Coaster Car.
   ?       ?      1-     The back Display Coaster Car.
   ?       4      1-     Number of guests in the car.
   ?      4*?     1-     The ID of every guest in the car.
   ?       4      1-     "ctsc".
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
   0       4      1-     "cstt".
   4       4      1-     Version number.
   8       ?      1-     The data of every car in the train.
                         The number of cars is stored in the coaster instance.
   ?       4      1-     The train's position along the track.
   ?       4      1-     The current speed.
   ?       1      1-     The train's current station policy.
   ?       4      1-     The number of milliseconds left to wait in the station.
   ?       4      1-     "ttsc".
======  ======  =======  ========================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster station
~~~~~~~~~~~~~~~
One station of a coaster.

This data layout is not a pattern in itself, but rather embedded in the Coaster Instance pattern.
It uses the Coaster Instance pattern's version number.

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
======  ======  ========================  ============================================================

Version history
...............

- 1 (20210402) Initial version.


Coaster intensity statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
A single coaster intensity statistics data point.

This data layout is not a pattern in itself, but rather embedded in the Coaster Instance pattern.
It uses the Coaster Instance pattern's version number.

======  ======  ========================  ======================================================
Offset  Length  Coaster Instance Version  Description
======  ======  ========================  ======================================================
   0       4      1-                      Position along the track.
   4       1      1-                      Whether this data point is valid (1 or 0).
   5       4      1-                      Data point precision.
   9       4      1-                      Average train speed.
  13       4      1-                      Average vertical G force.
  17       4      1-                      Average horizontal G force.
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
   0       4      1-     "csti".
   4       4      1-     Version number.
   8       ?      1-     `Ride instance`_ data.
   ?       4      1-     Number of positioned track pieces.
   ?       4      1-     Total length of the roller coaster (in 1/256 pixels).
   ?       2      1-     Number of placed track pieces.
   ?       ?      1-     Contents of "number" placed track pieces.
   ?       4      1-     Number of trains in this coaster.
   ?       4      1-     Number of cars in a single train.
   ?       ?      1-     Data of each train
   ?       4      1-     Maximum idle duration in milliseconds.
   ?       4      1-     Minimum idle duration in milliseconds.
   ?       4      1-     Number of stations.
   ?       ?      1-     Each station's `coaster station`_ data.
   ?       4      1-     Number of intensity statistics data points.
   ?       ?      1-     Each `coaster intensity statistics`_ data point.
   ?       4      1-     "itsc".
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
   0       4      1-     "fxri".
   4       4      1-     Version number.
   8       ?      1-     `Ride instance`_ data.
   ?       1      1-     Ride orientation.
   ?       2      1-     X coordinate of the ride base position.
   ?       2      1-     Y coordinate of the ride base position.
   ?       2      1-     Z coordinate of the ride base position.
   ?       2      1-     Number of working cycles.
   ?       4      1-     Minimum idle duration.
   ?       4      1-     Maximum idle duration.
   ?       4      1-     Time left in the current working phase.
   ?       1      1-     1 if the ride is working; otherwise 0.
   ?       ?      1-     `Onride guests`_ data.
   ?       4      1-     "irxf".
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
   0       4      1-     "onrg".
   4       4      1-     Version number.
   8       2      1-     The size of a batch.
  10       2      1-     The number of batches.
  12       ?      1-     Every batch's `onride guests batch`_ data.
   ?       4      1-     "grno".
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
   0       4      1-     "gstb".
   4       4      1-     Version number.
   8       1      1-     The batch's state.
   9       4      1-     The remaining running time.
  13       2      1-     The batch's entry information.
  15       ?      1-     The `onride guests data`_ for each guest.
   ?       4      1-     "btsg".
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
   0       4      1-     "gstd".
   4       4      1-     Version number.
   8       4      1-     The guest's ID.
  12       1      1-     The guest's entry information.
  13       4      1-     "dtsg".
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
   0       4      1-     "shop".
   4       4      1-     Version number.
   8       ?      1-     The `fixed ride instance`_ data.
   ?       4      1-     "pohs".
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
   0       4      1-     "gtri".
   4       4      1-     Version number.
   8       ?      1-     The `fixed ride instance`_ data.
   ?       2      1-     The entrance's x coordinate.
   ?       2      1-     The entrance's y coordinate.
   ?       2      1-     The entrance's z coordinate.
   ?       2      1-     The exit's x coordinate.
   ?       2      1-     The exit's y coordinate.
   ?       2      1-     The exit's z coordinate.
   ?       4      1-     "irtg".
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
   0       4      1-     "scni".
   4       4      1-     Version number.
   8       2      1-     X base coordinate.
  10       2      1-     Y base coordinate.
  12       2      1-     Z base coordinate.
  14       1      1-     The item's orientation.
  15       4      1-     The time in the animation.
  19       4      1-     The time since the item was last watered.
  23       4      1-     "incs".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Path Object
~~~~~~~~~~~
Information for a path object instance. This covers user-placed objects such as benches and lamps as well as litter and vomit.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "pobj".
   4       4      1-     Version number.
   8       2      1-     In-voxel X offset.
  10       2      1-     In-voxel Y offset.
  12       2      1-     In-voxel Z offset.
  14       1      1-     State bitset.
  15      16      1-     Data attributes.
  31       4      1-     "jbop".
======  ======  =======  ======================================================

The object's type index and voxel coordinate are saved by the parent pattern.

Version history
...............

- 1 (20210429) Initial version.


Message
~~~~~~~
Information for a single message in the player's inbox.

======  ======  =======  ============================================================================================================
Offset  Length  Version  Description
======  ======  =======  ============================================================================================================
   0       4      1-     "mssg".
   4       4      1-     Version number.
   8       2      1-     The message string ID.
  10       4      1-     The first data item.
  14       4      1-     The second data item.
  18       4      1-     The timestamp in compressed date format (see the Date block for information on this format).
  22       4      1-     "gssm".
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
   0       4      1-     "rcol".
   4       4      1-     Version number.
   8       1      1-     The palette index of the first colour.
   9       1      1-     The palette index of the second colour.
  10       1      1-     The palette index of the third colour.
  11       1      1-     The palette index of the fourth colour.
  12       4      1-     "locr".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.


Voxel
~~~~~
Represents a single voxel.

======  ======  =======  ===============================================================
Offset  Length  Version  Description
======  ======  =======  ===============================================================
   0       4      1-     "voxl".
   4       4      1-     Version number.
   8       4      1-     Ground type.
  12       1      1-     Instance type.
  13      0/2     1-     Instance data (skipped for voxels without a ride instance).
   ?       2      1-     Fence bits.
   ?       4      1-     "lxov".
======  ======  =======  ===============================================================

Version history
...............

- 1 (20210402) Initial version.


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
======  ======  =======  ======================================================

Version history
...............

- 1 (20150505) Initial version.


Game observer
~~~~~~~~~~~~~
Stores game observer related data.

======  ======  =======  =====================================================================
Offset  Length  Version  Description
======  ======  =======  =====================================================================
   0       4      1-     "GOBS".
   4       4      1-     Version number of the game observer block.
   8       1      1-     Won-lost state (0 for running, 1 for won, 2 for lost).
   9       1      1-     Whether the park is open (1 or 0).
  10       ?      1-     Park name.
   ?       4      1-     Park entrance fee.
   ?       2      1-     Current park rating.
   ?       4      1-     Current number of guests.
   ?       4      1-     Highest-ever number of guests.
   ?       4      1-     Number `R` of data points in the park rating history.
   ?     N*2      1-     Every data point in the park rating history (most recent first).
   ?       4      1-     Number `G` of data points in the guest count history.
   ?     G*4      1-     Every data point in the guest count history (most recent first).
   ?       4      1-     "SBOG"
======  ======  =======  =====================================================================

Version history
...............

- 1 (20220820) Initial version.


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
  20       2      2-     State of the hunger    complaint counter.
  22       2      2-     State of the thirst    complaint counter.
  24       2      2-     State of the waste     complaint counter.
  26       2      2-     State of the litter    complaint counter.
  28       2      2-     State of the vandalism complaint counter.
  30       4      2-     Time since the last hunger    complaint notification.
  34       4      2-     Time since the last thirst    complaint notification.
  38       4      2-     Time since the last waste     complaint notification.
  42       4      2-     Time since the last litter    complaint notification.
  46       4      2-     Time since the last vandalism complaint notification.
  50       4      1-     Number of active guests.
  54       ?      1-     Contents of "number" active guests. Each guest is stored as
                         his unique ID (2 bytes) followed by the `Guest`_ data pattern.
   ?       4      1-     "STSG"
======  ======  =======  ==============================================================

Version history
...............

- 1 (20150823) Initial version.
- 2 (20210429) Added guest complaint counters.


Staff
~~~~~
Stores all the staff in the game.

======  ======  =======  ==============================================================
Offset  Length  Version  Description
======  ======  =======  ==============================================================
   0       4      1-     "STAF".
   4       4      1-     Version number of the staff block.
   8       2      3-     Last unique staff member ID.
  10       4      1-     Number of pending mechanic requests.
  14       ?      1-     Every mechanic requests's ride ID (2 bytes each).
   ?       4      2-     Number of mechanics.
   ?       ?      2-     The data of every Mechanic_.
   ?       4      3-     Number of handymen.
   ?       ?      3-     The data of every Handyman_.
   ?       4      3-     Number of guards.
   ?       ?      3-     The data of every Guard_.
   ?       4      3-     Number of entertainers.
   ?       ?      3-     The data of every Entertainer_.
   ?       4      1-     "FATS"
======  ======  =======  ==============================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210423) Added mechanics.
- 3 (20210426) Added staff IDs, guards, entertainers, and handymen.


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
  18       8      2-     Current loan.
  26       ?      1-     Each `finance history`_ section's data pattern.
   ?       4      1-     "ANIF".
======  ======  =======  ======================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210917) Added loans.


Inbox
~~~~~
Stores all messages in the player's inbox.

======  ======  =======  ==============================================================
Offset  Length  Version  Description
======  ======  =======  ==============================================================
   0       4      1-     "INBX".
   4       4      1-     Version number of the staff block.
   8       4      1-     Number of messages.
  12       ?      1-     Every `message`_'s data pattern.
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
   ?       ?      1-     `Voxel stack`_ patterns.
======  ======  =======  ====================================================================

The voxel stack blocks store each voxel stack of the world, starting at
coordinate ``(0, 0)`` and ending at ``(max_x, max_y)``. The ``y`` coordinate
runs fastest.

Version history
...............

- (older versions are documented in the file "savegame_history.rst").
- 2 (20210402) Added border fence override rules.


Voxel Stack
~~~~~~~~~~~
Represents a voxel stack.

This pattern is different from all other blocks in that
it is typically present multiple times in the file.

======  ======  =======  ======================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================
   0       4      1-     "VSTK".
   4       4      1-     Version number.
   8       2      1-     Base height.
  10       2      1-     Stack height.
  12       1      1-     Owner type.
  13       ?      1-     Every `voxel`_'s data pattern.
   ?       4      1-     "KTSV"
======  ======  =======  ======================================================

Version history
...............

- (older versions are documented in the file "savegame_history.rst").
- 3 (20150428) Fences near the lowest corner of a steep slope moved from top voxel to base voxel.


Scenery
~~~~~~~
All placed scenery instances and path objects in the world.

======  ======  =======  ========================================================================================================
Offset  Length  Version  Description
======  ======  =======  ========================================================================================================
   0       4      1-     "SCNY".
   4       4      1-     Version number of the staff block.
   8       4      1-     Number of scenery items.
           ?      1-2    Every item's type index (2 bytes) followed by its `scenery instance`_ data pattern.
  12       ?      3-     Every item's internal name followed by its `scenery instance`_ data pattern.
   ?       4      2-     Number of user-placed path objects.
   ?       ?      2-     Every user-placed path object's data, consisting of the voxel coordinate
                         (3× 2 bytes), the item's type index (1 byte), and its `path object`_ data pattern.
   ?       4      2-     Number of litter and vomit objects.
   ?       ?      2-     Every litter and vomit object's data, consisting of the voxel coordinate
                         (3× 2 bytes), the item's type index (1 byte), and its `path object`_ data pattern.
   ?       4      1-     "YNCS"
======  ======  =======  ========================================================================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210429) Added path objects.
- 3 (20220829) Use internal name.


Rides
~~~~~
Stores all rides.

======  ======  =======  ======================================================================================
Offset  Length  Version  Description
======  ======  =======  ======================================================================================
   0       4      1-     "RIDS".
   4       4      1-     Version number of the rides block.
   8       2      1-     Number of rides.
           ?      1-1    Every ride's content, consisting of the ride type kind (1 byte), the ride type name
                         characters, and the data pattern of the `ride instance`_'s most derived class.
           ?      2-2    Every ride's content, consisting of the ride type kind (1 byte), the ride type
                         index (2 bytes), and the data pattern of the `ride instance`_'s most derived class.
           ?      3-3    Every ride's content, consisting of the unique ride instance index (2 bytes),
                         the ride type kind (1 byte), the ride type index (2 bytes),
                         and the data pattern of the `ride instance`_'s most derived class.
  10       ?      4-     Every ride's content, consisting of the unique ride instance index (2 bytes),
                         the ride type kind (1 byte), the ride type's internal name,
                         and the data pattern of the `ride instance`_'s most derived class.
   ?       4      1-     "SDIR"
======  ======  =======  ======================================================================================

Version history
...............

- 1 (20210402) Initial version.
- 2 (20210819) Replace ride type name with ride type index.
- 3 (20210827) Assign every ride instance a unique index.
- 4 (20220829) Use internal name.


.. vim: spell
