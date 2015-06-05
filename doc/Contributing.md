# Introduction #

The program is not finished yet, so we like your help. Current aim is to make the program "playable". To make this happen faster, we need developers.

# Code development #

Anything that does not work or exist needs fixing. You need to know C++, and make patches using the [code style](CodeStyle.md) of the project, and provide Doxygen comments with it.

The code should build out of the box at a Linux system. For Windows, the build system first needs to be extended so we can build it at both platforms.

If you need graphics that do not exist yet, just make some stub graphics. It does not matter how ugly they are.

Submissions take the form of patches in unified diff format.


# Graphics development #

We need LOTS of graphics. Most of the graphics are created using the [Blender program](http://www.blender.org). There are textures and templates in our repository that you should use, so everything gets a uniform look and a standardized format so it can be automagically processed without much effort.

Note that the GPL v2 license of the project requires you to make all your source files publicly available, so others can build on your work. Some source files may not allow such publication, please check the license conditions beforehand.

## RCD files ##
For future contributions with graphics and item properties, an alternative route exists (in theory so far, nobody has tried it yet). On startup, the program reads RCD files to load its graphics and item properties. The point of this is that everybody can create such a file, and thus make his own extensions.

Currently the **rcdgen** converter program exists (as in, you can download its source, and build it). There is also documentation what you can express in it, and the source files of the used graphics from the project data are available.

# Where should you work on #

In short, _anything that does not exist yet_. Since that is a bit vague, below is a little list

  * **Security** (needs benches, and/or lights, security guards walking around, guests that spot guards)
  * **Handy men** (needs growing grass, rides that break down, a dispatch system calling handy men for repair and maintenance, handy men walking around, and mowing the grass)
  * **Litter** (needs litter at paths, litter bins, handy men collecting litter)
  * **Park border** (needs park gate, option to buy new land)
  * **Guests finding their way through the park** (needs path finding, guests understanding what's available and what they want, some sort of advantage for owning a park map)
  * **User finding his way** (UI to display the park in small scale, windows listing rides, statistics, problem areas)
  * **Popularity** (needs a measure of how good the park is, feedback coupling to number of guests arriving (but up to some point so it doesn't spiral out of control), advertising)
  * **Load and save of games** (discussion: https://groups.google.com/forum/#!topic/freerctgame/h1iIJ-DLJWI )
  * **Scenarios**

All these topics are quite broad. The trick to make actual progress is not to try to do it all in one step. Instead, start with the smallest possible thing you can imagine, and realize it. It won't implement the entire topic, but you're one step closer. Repeat until you run out of things to implement.


# Way of working #

  * You may (but don't have to) announce you are going to make something.
  * The project uses the GPL v2 license. Anything you submit gets that license. If you don't like that, please don't submit anything.
  * A [discussion group](https://groups.google.com/forum/?fromgroups#!forum/freerctgame) exists, where you can discuss things.
  * An issue in the issue tracker is the center of (mostly technical) communication about the subject.
    * When you have something concrete, find an existing issue or create a new one, and put the information and/or data in it.
    * We will review it, and give feedback. It may be a while before that happens though, we are all quite busy.
    * When it is of good enough quality, it is accepted and added to the project. Again, this may take a while.