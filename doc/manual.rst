FreeRCT manual
==============

While the game is still in development (and not much of a game currently),
this is its manual. Its main purpose is to explain the user interface.

When the program is started, you are presented the main game display.
Here you can do the following things:

- Drag the world by holding the right mouse button and moving the mouse.
- Rotate left/right with the cursor left/right buttons at the keyboard.
- Quit the program by entering 'q' at the keyboard, or select ``Quit`` from
  the top toolbar, and confirm.


Terraforming
------------

You can modify the landscape by using the ``Terraform`` window available from
the ``Landscaping`` button at the top of the window.

With the ``+`` and ``-`` buttons at the right side of the window you can
enlarge and decrease the size of the affected area. The area is displayed in
the center of the window, and also projected at the main display when you
move the mouse into it.

Actual landscaping is performed by using the mouse wheel. It moves the
displayed area. For areas bigger than a single tile, you can move entire
tiles only. That can be done in two different ways, selectable at the bottom
of the window. The ``Level terraforming area`` selection will move the
highest corners in the area down, or the lowest corners up. The other
selection, ``Move terraform area as-is`` moves the *entire* area up or down.
When the area is a single tile or a dot, you can select the entire tile or
just one corner. A single tile area limits landscaping changes to that tile.
A 'dot' area has no limits. It changes the entire world, until the change
hits the edge of the world, or a vertical area.


Path building
-------------

A path can be build by opening the path gui. Click 'Paths' in the toolbar. A
window appears with the following buttons:

- Slope buttons 'Down', 'Level', 'Up' to indicate in which vertical
  direction to build the path. (Should be replaced with graphics.)
- Four direction arrows to indicate the horizontal direction of the path.
- 'Forward' and 'Backward' to move the selected tile in the selected
  horizontal direction.
- 'Long' to build a long path in one go.
  Place the tile cursor, then click 'Long'. A path will be constructed to the
  position of the mouse. Click to fixate the position. Then click 'Buy' to
  buy the new path.
- 'Buy' to build a path next to the currently selected tile, in the indicated
  directions (both horizontal and vertical). The currently selected tile will
  move to the newly bought path.
- 'Remove' to sell the currently selected path. Selection will move one tile
  back.

Dragging the world with the right mouse button, and rotating the world
continues to work to allow moving around.


Guests
------

Guests follow paths, starting from a path tile at the edge of the world. They
will buy food from your shops if they need it. When they become unhappy, they
will disappear.


Building a shop
---------------

The ``Buy ride`` button at the top opens the window to buy new rides.
Currently, the only ride you can buy is a shop. It is automatically selected.
With the rotation buttons at the bottom right, you can rotate the shop. The
``select`` button selects the shop so you can place it in the world. It will
try to connect to a road if possible. Move and rotate the world and position
the shop with the mouse until it is at the point where you want it. Then
left-click the mouse to add the shop.


Managing a shop
---------------

By clicking at a shop, you can open its window. It shows some statistics so
you can see how well it is doing. At the bottom is a selection to open or
close the shop.


Finances
--------

The ``Finances`` button at the top opens the finances window, showing your
overall financial records.


vim: tw=77
