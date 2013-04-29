Very quick how-to
=================

Sorry, only instructions for Linux, as that's the only platform I know about.

Check out
---------

The source code is in Subversion, so you need a svn client to get it.

The command to do this is

::
        $ svn checkout http://freerct.googlecode.com/svn/trunk/   freerct-read-only

(see also the 'source' tab of the project)


Building the program
--------------------

Almost everything is written in C++, which means you need *g++* to compile it. In
addition, you need the development packages of *SDL* and *SDL-ttf* (true-type
font). you also need *libpng* for making the RCD data files (that contain the
graphics and other data read by the program). Last but not least, you need the
standard development stuff, like *make*. Most Linuces have these packages in
their repository.

All the paths are all still hard-coded in the Makefile. (No pretty ./configure yet).

Building is as simple as

::

        $ cd freerct-read-only # Go into the downloaded source directory.
        $ make                 # Let make do the heavy work.

- The src directory contains the source code of the FreeRCT program itself.
- The rcdgen directory contains the source code of the rcdgen program, that
  builds RCD files from source (which are read by freerct).
- The rcd directory contains the source files of the RCD data files, except
  the graphics.
- The sprites_src directory contains all the graphics of the game.

The ``make`` command above will also generate the rcdgen program, and build the RCD
files. This should create a 'freerct' program in the directory.

At the end, make checks for the presence of the ``freerct.cfg`` file. The first
time, that will fail. See below how to fix that.


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
 $ cd src
 $ ./freerct


which should open a window containing a piece of greenly coloured flat world,
and a toolbar near the left top (see also the pictures in the blog).

In ``/doc/manual.rst`` you can find a description of most of the current
functionality. In case you get stuck, pressing 'q' quits the program.















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

