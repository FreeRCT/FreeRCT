Very quick how-to
=================

Sorry, only instructions for Linux, as that's the only platform I know about.

Building RCD files
------------------
The RCD files contain the data/graphics of the program.

To build them, you need Python 2.7 and the Python-imaging library (PIL). Many
linuces have them in their package manager.

Building is as simple as

::

  $ cd freerct-read-only # Go into the downloaded source directory.
  $ cd rcd               # Go into the 'rcd' directory.
  $ make                 # Let make do the heavy work.
  $ cd ..                # Go back to the root


Building the program
--------------------
The program is written in C++, which means you need g++ to compile it. In
addition, you need the development packages of SDL and SDL-ttf (true-type
font). It uses Python again to generate a table/strings.h file in the process.

The paths where it looks for SDL are hard-coded in the Makefile. (No pretty
./configure yet).

::

  $ cd src  # Go into the source code directory.
  $ make    # Build it.


This should create a 'freerct' program in the directory.
Building language files

The language file contain the texts displayed by the program. Currently there
is only one language file, but one day we will have more. You can build it in
the 'src' directory as well

::

  $ make lang


(which generates a ../rcd/english.lng file)

Config file
-----------
Finally, you need a 'freerct.cfg' INI format file next to the 'freerct'
program, containing the path to the font file you want to use. It looks like

::

  [font]
  medium-size = 12
  medium-path = /usr/share/fonts/gnu-free/FreeSans.ttf

This means the medium sized font is 12 points high, and its source font
definition file is at the indicated path. Make sure you use a path that
actually exists.

The actual file is not that critical, as long as it contains the ASCII
characters, in the font-size you mention in the file.


Running the program
-------------------
Now run the program

::

  $ ./freerct


which should open a window containing a piece of greenly coloured flat world,
and a toolbar near the left top (see also the pictures in the blog).

Mouse wheel raises and lower terrain, left/right cursor rotates the world, and
right-drag moves the world. Clicking at 'Paths' opens the path tool. Click at a
world tile to start a new path, click an arrow for the build direction, and
'level' for a level path (and 'up'/'down' for a path leading up or down). 'Buy'
buys you a path tile. 'Remove' unbuys the path, and 'long' allows building a
long path in one go.

Finally, 'q' quits the program.


Guests
------
Guests are not visible yet, but they need a path tile at an edge (and not in
the corners) to enter the world.

Hopefully in the near future you can see them walking around.

