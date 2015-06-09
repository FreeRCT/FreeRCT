The main interface is a sprite-based dimetric world with an overlaid window system. The dimetric world is a standard isometric-like game world. Clicks are registered on a sprite-by-sprite basis; if a user clicks on a guest on a path on a terrain tile then it is the guest which is identified as the target of the clicking. The windowing system is a standard layered window system.

Interaction with the world is semi-modal. There are two key ways of interacting with the world.

## Normal mode ##

  * Left click in the isometric world 'selects' the object which was clicked on. 'Selects' refers to opening the window associated with that object, e.g. the relevant guest window, ride window, etc.
  * Right click in the isometric world has no action.
  * Click and drag in the isometric world moves the world view.
  * Left click in a window acts as normal.
  * Click and drag on a window title bar moves the window.
  * Hovering the mouse over a window opens a tooltip (if available).

## Construction mode ##
Construction is a modal process. Only one type of construction can be occuring at any time, e.g. building/modifying one track ride, placing one shop, changing paths, altering terrain.
  * All interactions are the same as normal mode, except: right click either removes the selected object (for scenery objects) or selects the object for modification. E.g. if a track ride was being modified then right clicking on a path would switch the construction mode to modifying the paths.
  * Switching to a construction mode automatically opens the associated window; e.g. if editing terrain then right clicking on a path then the construction mode switches to modifying paths and the path modifying window is opened.

For tablet/touchscreen compatibility left click and drag (not just right click and drag) should move the world view. Press and hold in the isometric world should act like a right click. Press and hold on a window should act like a mouse hover.

## Key windows ##
### Colours ###
Although each widget has a colour, a window should have a single colour as much as possible to give a clean look.

The colour of a window depends on its function:

| **Type of window** | **Colour** |
|:-------------------|:-----------|
| Construction       | Brown      |
| Warning            | Red        |
| Landscaping        | Green      |
| Ride/guest info    | Dark red   |

### Ride/Shop window ###
Note the park window is a specialised version of this window accessed by clicking on the ride entrance.

  * Home tab features a slightly zoomed out view of the ride/shop and the basic controls; a button to modify the attraction (i.e. switch to construction mode), open/close/test the ride, rename the ride, centre the world view on the ride.
  * Economics tab summarises the ride usage and money statistics; guest numbers, running costs, profits, etc. The user can set ride pricing (and pricing of any purchaseable addons, eg. on-ride photo).
  * Ride properties tab summarises the ride property statistics; excitingness, intensity and nausea ratings. It also includes controls for altering the ride running mode (e.g. train lengths, ride lengths, etc.)
  * Apperance tab. Specify the ride colour scheme and any graphical variants.

The guest window (tabbed interface)
  * Home tab features view of the guest and basic controls; a button to pick up the person, rename the person, centre the world view on the persion.
  * Statistics tab summarises the number of rides ridden, objects carried, etc.
  * Properties tab summerises the guest preferences and their current status; happiness, energy etc.
  * Thoughts tab includes the recent thoughts of the guest regarding the park. This is the key feedback mechanism; e.g. "The carousel is very boring", "The rollercoaster is too expensive", "I'm hungry", "Where is the exit?", etc.

### Staff window ###
  * **to do**

### List windows ###
  * These are listed summaries of various park objects, e.g. rides/shops, guests, staff.
  * Sortable lists based on various object statistics. e.g. running status of ride/shop, happiness of guests

### Construction windows ###
  * Windows for the various construction processes. e.g. track ride modification, simple ride/shop placement, path construction, landscaping, scenery placement.

## Guests ##
How guests interact with the park defines gameplay, sets the challenges of the game and controls the game difficulty.

Each guest has several properties which defined their behaviour; character traits, preferences, etc. They also have several properties; happiness, hunger, anger, etc. The parks scenery, rides, food, etc. Affect the guests properties depending on their preferences. In turn the guests properties affect their behaviour; willingness to spend money, ride rides, vandalise scenery, etc.

### Preferences ###
  * Ride intensity
  * Nausea tolerance
  * Willingness to spend money
  * (Dietary preferences (salt, sweet, etc.)?)

### Properties ###
  * Happiness (from happy to angry)
  * Energy (from energetic to tired)
  * Nausea
  * Amount of money to spend
  * Hunger
  * Thirst
  * Toilet

### Park impacts ###
Guests choose to ride rides based on their ride intensity preference, ride excitingness and ride nausea rating. They also consider how much the ride costs with respect to their willingness to spend money.
  * Increase happiness (modulated by ride intensity and guest preference and ride excitement)
  * Increase nausea (modulated by ride nausea rating)

Scenery around paths and rides has two effects:
  * Improves happiness
  * Modulates ride excitement

Food and drink items can be bought and consumed, see ParkGuestItems.
  * Happiness (modulated by dietary preference)
  * Hunger level (food reduces)
  * Thirst level (drinks reduce, salty food increases)
  * (Nausea level (food increases)?)
  * Toilet (increases)

  * Getting lost increases anger
  * Walking decreases energy (tired guest walk slower)
  * Litter and vandalism decrease happiness

Various guest properties impact each other, e.g.:
  * Desire for food and drink is affected by guest nausea

Behaviour of the guests is impacted by their properties as well:
  * Likelihood of being sick increases with nausea
  * Willingness to spend money affected by happiness/anger
  * Willingness to vandalise depends on anger

### Items ###
Guests can buy, carry and use/consume several types of item. These are:
  * Food (Chips, pizza, candy floss/cotton candy, burgers etc. These items reduce guest hunger, increase energy (and increase nausea?) as they are eaten.)
  * Drinks (Fizzy drinks, water, etc. These reduce thirst (and increase energy?) as they are consumed.)
  * Balloons (No functional effect)
  * On-ride photos (No functional effect)
  * Umbrella (Used when raining. Umbrellas reduce loss of happiness during rainy weather and negate the aversion to queues during rain. 'Outdoor' rides, which are less popular during rain, are not affected by umbrellas.)
  * Park maps (These reduce the chance of guests getting lost and help guests find rides which match their preferences.)
  * Litter (Generated when some items are used up, must be disposed of in a bin or it is eventually dropped)

### Graphics ###
Most guests properties do not have visible effects on the guest sprites. The exceptions are:
  * High nausea: Face sprite changed to a green one.
  * High anger: Face sprite changed to a red one
  * Low energy: Body sprite changed to a slouched one with slower movement.
  * High energy: guest movement sped up with a corresponding increase with animation frame rate.

Sprites for the different items will be overlaid onto the 'guest carrying something' sprites. For simplicity only one item will be shown at a time, eg. if a guest with a balloon and an umbrella buys a drink then just the drinks can would be shown. Once finished with the drink the balloon would be shown. If, at any point, it started raining then the umbrella would be shown.

Some item types require specialised animations:
  * Food & drink; sipping/biting animation
  * Map; looking at map animation

## Staff ##
There are four types of staff:
  * Handyman
  * Engineer
  * Security guard
  * Entertainer

### Handymen ###
Roles:
  * Scrubbing paths to remove vomit and litter (messy paths reduce guest happiness)
  * Maintaining scenery objects which require maintenance (e.g. flower beds require watering)
  * Emptying bins (to prevent overflowing)
  * Maintaining terrain which requires maintenance (e.g. mowing grass)

### Security Guards ###
Role:
  * Patrolling paths (when a security guard is nearby an angry guest is less likely to vandalise)

### Entertainers ###
Role:
  * Patrolling paths and dancing/entertaining guests (increases happiness)

### Engineers ###
Roles:
  * Called to rides at the specified maintenance interval (regular servicing reduces loss of reliability over time)
  * Called to rides in the case of a breakdown (rides cannot run while broken down, some breakdowns may cause accidents)
  * (Fixing vandalised path decorations and scenery?)

All staff (except handymen while mowing grass) stay on the paths and ignore queues. Patrol areas can be defined per staff member; in this case the staff member does not leave the specified area and will attempt to return to it if removed.

All staff (like guests) can be picked up and placed in a new location on the map.

## Scenery ##
The primary roles of scenery are to improve guest happiness and to increase the excitingness of rides.

Scenery types:
  * Single tile objects
  * Multiple tile objects
  * Quarter tile objects
  * Fences
  * Path decorations

These can be subcategorised by type:
  * Objects
  * Trees and flowers

Some scenery items have specific functionality, for example the over-path sign which is placed on paths. This can be used to close that section of path to guests. Some scenery has specific requirements, for example flower beds which have to be watered by a handyman else the plants will gradually die.

Different terrain and support types are a related feature. All land tiles can be given one of several ground types, rocks, grass, sand, etc., and foundation type, bare, wood, rock, etc.  Similarly different path types are available. Some ground types have special maintenance requirements, like grass which requires mowing by a handyman to stay in a format pleasing to guests.

Some path decorations have key functions:
  * Benches - guests can sit and gradually restore energy. Guests also prefer to sit while eating.
  * Bins - some guest items produced litter when consumed, eg. drinks, food. Guests would dispose of litter in bins (which had to be emptied by handymen) or would litter the path.

Scenery objects have no clickable properties, right click removes it when in construction modes.

## Weather ##
In addition the world features weather. Weather can be sunny, cloudy, light or heavy rain or stormy. Weather is season dependent (the time in the park runs March to October and back to March seamlessly) and impactes guest behaviour. Outdoor rides are less popular in bad, especially wet, weather. Good weather boosted guest happiness and makes guests seek water rides.
