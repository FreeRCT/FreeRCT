# Introduction #

As in any open source project, many people will read and perhaps even modify the code. Quite likely this will happen long after you have left. Therefore, it is important to make the code easily understandable.

To make this happen, the project has a number of rules and guide lines for code.
  * Document every part.
  * Use algorithms that scale well without being cryptic.
  * Use the code style explained below.

## Documenting code ##

Everybody knows we should do it, but most programmers think they can live without. When you write programs of less than a thousand lines that you never give to other people, you probably can.
FreeRCT is however close to 30,000 lines (when I write this page), and growing. There are just too many details to remember all.

Of course you can read the code of the method to understand what it does, and for code that you want to change you have to do that, but what if you just want to call the function?
The latter kind of use is where documentation makes the difference. In this project, you can read just a few lines of text instead of reverse engineering the intended use from the code, and hope you didn't miss an important detail.
(Alberth: Since I switched to documenting all my code, I found that it actually speeds up my work instead of slowing down, despite the additional time needed to write the documentation.)

The project uses [Doxygen](http://www.doxygen.org/) for documenting the code. There is no public generated documentation you can browse, but it is easy to install the program locally, and build your own documentation html tree.

Comments are normal sentence, they start with uppercase letters, and end with dots.

There are three main forms of documenting:

  * A single line can be place above the element:
```
    /** Compute the initial position of a window. */
    class ComputeInitialPosition ...
```

  * Multiple lines start with a `/**` line:
```
    /**
     * Set the initial position of the top-left corner of the window.
     * @param x Initial X position.
     * @param y Initial Y position.
     */
    void Window::SetPosition(int32 x, int32 y) ...
```

> The leading white space before the second-to-end comment lines is the same as the leading white space before the `/**` line, and one extra space at the end. In this way, the block always stays together.

  * Documenting afterwards at a single line:
```
    int base_pos; ///< Base position of a new window.
```


## Algorithm choices ##

The program should be fast, inevitably lots of features will be added that all cost CPU time. To make this possible, each feature should try to minimize use of resources (memory and CPU).
At the same time, the algorithm should be easy to understand.
These constraints are conflicting, so there are no simple rules that cover all cases here.

As a general rule however, the fastest possible solution is not the right one. Highly optimized solutions are often difficult to read and understand. They also tend to fail when you compile the code at another processor architecture.
Instead aim for good scalability. The base speed is not so important. What _is_ important, is that the speed should not dramatically go down when the amount of work grows (exponential or worse).

If possible, document what algorithm you are actually using, and include a publicly available reference to it, so others can read what you implemented instead of deciphering from the code.

## Code style ##

Code style is the easiest to do, but at the same time it is the most difficult. Like all projects, this one also uses a crazy code style different from your own. It's not something that is intended, it just happens.

Here are the main rules (sometimes with a rationale), in a number of topics:

### White space ###

  * Use TABs for leading white space. Everybody wants a different indent setting. Now you can have it by adjusting your tabstops.
  * Kill all trailing white space. It's just waste of space, and causes spurious lines in patches.
  * All lines should end with an EOL (`\n`) character. In particular, the last line of the file must have that. Some editors do this on their own (for example, the vim editor), other editors do not do this, so you need to add an empty line at the end (but not more than one, see below).
  * Don't add trailing empty lines (or more than one, if your editor needs an empty line at the end to get an EOL after the last line).
  * Use a single empty line at most between code elements.

It is **really useful** to show TAB and trailing white space in your editor, or you cannot even see whether you followed the above rules.

### Header and file structure ###

It's a GPL project, so the GPL license must be added above every file in the project. The following template file might be useful:
```
/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.h File IO declarations. */

#ifndef FILEIO_H
#define FILEIO_H

#endif
```
The `$Id$` is used by [http:subversion.apache.org Subversion] to denote the revision of the file. See the `svn:keywords` property of `svn propset` for details.

Lines 3 to 8 are the condensed GPL license.

The `/** @file fileio.h File IO declarations. */` is the Doxygen file header documentation. Adjust it for your new file.

If you make a `.h` file, the `#ifndef ... #endif` lines are useful (adjust the name of the macro to the name of your new file).

For a `.cpp` file, remove those lines and start with an `#include "stdafx.h"` line instead.
Try to have as many `#include` lines in the `.cpp` files as you can (instead of in the `.h` file), to reduce the number of files implicitly included.


### Classes and structures ###

There is no clear rule about using classes versus structures. In general, `struct`s are more plain data-like, while classes are more real objects. When using `malloc` and friends, `struct`s are usually better.

Classes and structures use camelcase names, like
```
/** Base class for windows with a widget tree. */
class GuiWindow : public Window {
public:
        ....
};
```

The general order is `public`, `protected`, and `private`. Methods go before data members.

Methods also use camel case for their name. Names of formal parameters are lowercase only, with underscore characters to enhance readability. If a parameter has a 1-to-1 correspondence with a data member, use the same name.

The first creation of a virtual method is expressed by the `virtual` keyword. Derived classes and all implementations use a `/* virtual */` prefix to denote the method is virtual.

Most code is not very protective about data access, and puts all variables in the public space to make them easily readable, and avoid getter and setter clutter (There is no code other than this project that accesses the data, no need to stay compatible with older versions etc.)
Whether an external user has write access should be clear too. If it is not clear, it should be documented.


Templates and abstract (base) classes are fine, but they should have a concrete positive influence on the size or complexity of the code.
Standard containers for simple cases are avoided to reduce memory fragmentation. Use an array, or a single or double linked list instead (or whatever is the right solution).

### Enumerations ###
Enumerations use camelcase, like
```
/** Sprites for supports available for use. */
enum SupportSprites {
        SSP_FLAT_SINGLE_NS, ///< Single height for flat terrain, north and south view.
        ...

        SSP_COUNT,          ///< Number of support sprites.
};
```
Each value is completely uppercase, with underscore characters for readability. Each element ends with a comma, to make it easy to add new elements.
Many enumerations have a `_COUNT` element at the end to denote how many elements there are.

### Global variables ###

In general, they should be avoided, but there are good use cases. Try to make them `static` and `const` if possible.

They start with an underscore character, followed by lowercase letters and underscores for readability, for example
```
WindowManager _manager; ///< %Window manager.
```

Constants are in uppercase, just like enumeration values. For example:
```
static const int WORLD_X_SIZE = 128; ///< Maximal length of the X side (North-West side) of the world.
```


### Methods and function implementation ###

Method and function bodies are statements mixed with (variable) declarations. To start with the latter:

  * Local variables are lower case, with underscore characters to enhance readability, if needed.
  * Pointer and reference declarations are attached to the name rather than the type.
  * Declarations are done at or near their first use. If possible, declare iterators inside the `for` statement.
  * Variables that contain commonly used objects often have the same name, try to use the same convention.
  * Accesses to members of the class are always prefixed with `this->` to make them stand out with local variables.

Simple statements like assignment or function calls that are very long can be split over multiple lines. In that case, add **2** additional TABs before the second and further lines to show the line is a continuation. For example
```
return this->titlebar.IsLoaded() && this->button.IsLoaded()
		&& this->rounded_button.IsLoaded() && this->frame.IsLoaded()
		...
```

Statements that have a block statement like `if`, `while`, `for`, and so on, may be either written at a single line, or split over sveral lines by adding curly brackets. The opening curly bracket is at the end of the line above the block, and the closing is at the same indentation as the starting indent, like
```
for (uint i = 0; i < TBN_COUNT; i++) this->bend_select[i] = NULL;

while (true) {
	if (iter == this->animations.end()) return;
	...
}
```

The `case` statement is indented inside the `switch` statement. To increase readability, an empty line is often useful between the alternatives. If you need local variables in the `case`, add them locally:
```
switch (wid_num) {
	...

	case BTB_SPACING: {
		Point32 money = GetMoneyStringSize(LARGE_MONEY_AMOUNT);
		...
		break;
	}

	case ...:
		...
}
```
Normally, the switch is complete. Use the `NOT_REACHED();` macro to denote some cases will never happen. If a non-empty alternative falls through to the case below, use `/* FALL-THROUGH */` to denote this at the end of the former case.


  * There is a space between the keyword (`if`, `while`, etc) and the opening parenthesis.
  * Integer and pointer comparisons are explicit. That is, do `if (p != NULL)` or `while (i == 0)` instead of relying on the implicit conversion `if (p)` or `while (!i)`. Boolean comparisons are not explicit.
  * `free` and `delete` can handle `NULL` values, no need to check against them yourself.
  * A line containing only comment uses C-style comment, like `/* ... */`. Comments behind some code use line comment, like `a = 3; // ...`

Other expression rules

  * There are operator priorities in C++, you don't need to enforce them by adding parentheses. For uncommon or unclear cases, it may be useful to add them though.
  * Use spaces around operators like `a + b` or `a != 4 && b * 2 < 15 * f`.

Finally, as  a general rule, don't be over-generic in the implementation. Only add functionality in the functions and methods that you actually need now. If you need more tomorrow, add it tomorrow. Plans for the future sometimes don't happen, and then there is all this future functionality that nobody needs, which is only in the way.

## Acknowledgment ##

Many thanks to the [OpenTTD](http://www.openttd.org/) project for [defining the code style](http://wiki.openttd.org/Coding_style).