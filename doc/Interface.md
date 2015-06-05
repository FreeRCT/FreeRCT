The main interface is a sprite-based dimetric world with an overlaid window system. The dimetric world is a standard isometric-like game world. Clicks are registered on a sprite-by-sprite basis; if a user clicks on a guest on a path on a terrain tile then it is the guest which is identified as the target of the clicking. The windowing system is a standard layered window system.

Interaction with the world is semi-modal. There are two key ways of interacting with the world.
Normal mode:

  * Left click in the isometric world 'selects' the object which was clicked on. 'Selects' refers to opening the window associated with that object, e.g. the relevant guest window, ride window, etc.
  * Right click in the isometric world has no action.
  * Click and drag in the isometric world moves the world view.
  * Left click in a window acts as normal.
  * Click and drag on a window title bar moves the window.
  * Hovering the mouse over a window opens a tooltip (if available).

Construction mode:
Construction is a modal process. Only one type of construction can be occuring at any time, e.g. building/modifying one track ride, placing one shop, changing paths, altering terrain.
  * All interactions are the same as normal mode, except: right click either removes the selected object (for scenery objects) or selects the object for modification. E.g. if a track ride was being modified then right clicking on a path would switch the construction mode to modifying the paths.
  * Switching to a construction mode automatically opens the associated window; e.g. if editing terrain then right clicking on a path then the construction mode switches to modifying paths and the path modifying window is opened.

For tablet/touchscreen compatibility left click and drag (not just right click and drag) should move the world view. Press and hold in the isometric world should act like a right click. Press and hold on a window should act like a mouse hover.

Key windows:
The ride/shop window (tabbed interface). Note the park window is a specialised version of this window accessed by clicking on the ride entrance.
  * Home tab features a slightly zoomed out view of the ride/shop and the basic controls; a button to modify the attraction (i.e. switch to construction mode), open/close/test the ride, rename the ride, centre the world view on the ride.
  * Economics tab summarises the ride usage and money statistics; guest numbers, running costs, profits, etc. The user can set ride pricing (and pricing of any purchaseable addons, eg. on-ride photo).
  * Ride properties tab summarises the ride property statistics; excitingness, intensity and nausea ratings. It also includes controls for altering the ride running mode (e.g. train lengths, ride lengths, etc.)
  * Apperance tab. Specify the ride colour scheme and any graphical variants.

The guest window (tabbed interface)
  * Home tab features view of the guest and basic controls; a button to pick up the person, rename the person, centre the world view on the persion.
  * Statistics tab summarises the number of rides ridden, objects carried, etc.
  * Properties tab summerises the guest preferences and their current status; happiness, energy etc.
  * Thoughts tab includes the recent thoughts of the guest regarding the park. This is the key feedback mechanism; e.g. "The carousel is very boring", "The rollercoaster is too expensive", "I'm hungry", "Where is the exit?", etc.

The staff window
  * **to do**

List windows
  * These are listed summaries of various park objects, e.g. rides/shops, guests, staff.
  * Sortable lists based on various object statistics. e.g. running status of ride/shop, happiness of guests

Construction windows
  * Windows for the various construction processes. e.g. track ride modification, simple ride/shop placement, path construction, landscaping, scenery placement.