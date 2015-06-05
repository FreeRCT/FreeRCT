FreeRCT will be a modular game, a design which supports the modular nature of the gameplay.

Scenarios:
  * The core of the game is scenarios; which consist of a landscape partly populated with paths, scenery, rides, etc.
  * Scenarios can be downloaded or randomly generated.
  * A scenario is the basic gameplay unit. Each scenario (with the exception of a sandbox mode) has a set of goals and a time limit. Completion of a scenario may unlock the next in a series of scenarios.
  * A scenario features certain park attractions, scenery, path types, etc. Some may be available at the start of the scenario while others may have to be researched.

Game content
  * All scenarios require some fundamental game content components. These will be provided through the basic installation. This includes the basic terrain (grass and water), foundation (plain earth), path (concrete and benches and bins), scenery (eg. a tree bush, fence), guests, key staff, etc.
  * All other rides, shops, scenery components will represent modular downloadable content which can be downloaded if requested by a particular scenario.
  * Modularisation of game content minimises initial install size and eases community contribution of game content. Downloadable content (through an in game system) ensures easy access to content.


Game components:
  * Track rides consist of the graphics for the track components, the metadata encoding track component shapes and some other key data encoding properties like; crash on negative vertical G force, swing out on corners, inverted cars, constantly powered/free moving, etc. Graphics for the ride cars and passengers are also included. (track rides also lend themselves to submodules; alternative cars, new track components etc.)
  * Simple rides consist of the graphics for the ride and some simple scripts as to how they should be displayed. Key ride properties, excitingness, nausea, etc., are also included.
  * Shops consist of the graphics for the shop, details of what item(s) the shop sells and the behavior of those items. It also provides sprites to represent the guest carrying that item.
  * Scenery simply consists of the graphics and and a class which provides information about the scenery styling for grouping with other similar scenery items.