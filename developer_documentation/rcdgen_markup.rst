:Author: The FreeRCT Team
:Version: 2023-10-02

.. contents::
   :depth: 3

#####################################
FreeRCT Markup Language Specification
#####################################

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
This file documents the file format used by FreeRCT's *RCDgen* program to generate RCD files. See "data_format.rst" for more information about RCD files.

RCDgen accepts input files in the **FreeRCT Preprocessed Markup Language (FPP)**.

**FreeRCT Markup Language (FML)** files and **FreeRCT Markup Snippet (FMS)** files
can be passed to the C preprocessor to generate FPP files for RCDgen.

The three file formats differ only in that FML and FMS files may additionally contain `preprocessor directives`_ for the C preprocessor.

Writing FML and FMS files to autogenerate the FPP files is optional. It is possible to skip this step and write FPP files directly.


Recommended File System Layout
==============================
The following section is a recommendation on how to layout the files. It is permitted to ignore it.

An FML file is a top-level file. It must not be included in any other file.
It should generate exactly one RCD file by providing exactly one top-level value list which is a `file`_ node.
Except for the filename extension, the name of the generated file should be identical to the FML file's name.

An FMS file is not preprocessed by itself. It is always included from another FMS or FML file.
Each FMS file is included in only one other file.

Large FML or FMS files should be split into multiple files placed in a directory tree.
Similar or related files are grouped by placing them in the same subdirectory.
Every directory contains a file called ``main.fms`` which includes all other files in the directory
as well as the ``main.fms`` of every subdirectory.

Every FMS file should define at most one unit. Large units should be split across multiple FMS files.


Basic File Format Definition
============================

Value Lists
~~~~~~~~~~~
An FML file consists of nested *value lists*. A value list describes one node; nodes can be either blocks_ in an RCD file or `intermediary nodes`.
A value list consists of:

- The name of the node.
- An opening brace ``{``.
- Any number of *values*.
- A closing brace ``}``.


Values
~~~~~~
Recognized types of values are:

- Decimal *integral constants*, e.g. ``5`` or ``-128``.
- *String literals* enclosed by double quotes, e.g. ``"Hello World!"``.
- `Predefined symbols`_.
- A *bitset* of integers, defined by the keyword ``bitset`` followed by parentheses enclosing a comma-separated list of integers.
- A value list. Some `intermediary nodes`_ can expand to multiple values.

Boolean values are represented as integers, where 0 means false/off/no and any other value means true/on/yes.

Any value that is not a value list must be terminated by a semicolon (``;``).


Named Values
~~~~~~~~~~~~
Values can optionally be **named**  by prepending ``NAME:``, where ``NAME`` is a non-empty sequence of
ASCII uppercase and lowercase letters, digits, and the characters ``_#``.

Some nodes may contain multiple values with the same name.

Name Matrices
.............
`Intermediary nodes`_ may expand to a matrix of values, in which case ``NAME`` may be a matrix of names.

A matrix of names is enclosed by parentheses (``()``).
The rows of the name matrix are separated by commas (``,``).
The lines of the name matrix are separated by pipes (``|``).

Instead of using pipes and commas, the name matrix can also be generated from a sequential name template.
A sequential name template is a name containing one optional ``hor`` sequence and one optional ``vert`` sequence.
A ``hor`` or ``vert`` sequence consists of:

- An opening brace ``{``.
- The keyword ``hor`` or ``vert``.
- An opening parenthesis ``(``.
- A decimal integer denoting the inclusive sequence start.
- The keyword ``..`` (two periods).
- A decimal integer denoting the inclusive sequence end. This value must not be less than the sequence start.
- A closing parenthesis ``)``.
- A closing brace ``}``.

A sequential name template expands to a matrix of all possible combinations of names
where the sequences are substituted with each integer in the inclusive range.
The ``hor`` sequence generates the columns of the matrix, and the ``vert`` sequence the rows of the matrix.

For example, the template ``(se_{vert(0..2)}_{hor(0..3)})`` expands to the name matrix

.. code-block::

   (se_0_0, se_0_1, se_0_2, se_0_3
   |se_1_0, se_1_1, se_1_2, se_1_3
   |se_2_0, se_2_1, se_2_2, se_2_3)

The name at column ``c`` and row ``r`` of the name matrix is assigned the value
at column ``c`` and row ``r`` of the values matrix generated by the node.
Row and column indices are zero-based. A single name is equivalent to a matrix with a single element.

It is an error if the values matrix does not contain enough rows or columns to assign a value to every name.
If the values matrix contains more rows or columns than the names matrix, the extra values are unnamed.


Whitespace
~~~~~~~~~~
All tokens may be surrounded with any amount of whitespaces, tabs, carriage returns, and newlines.


Comments
~~~~~~~~
**Comments** may be used to make the code easier to read.

A *block comment* starts with the token ``/*`` and stretches until the first occurrence of the token ``*/``.

A *line comment* starts with the token ``//`` and stretches until the first newline character.

All text inside the comment including the starting and closing token will be ignored by RCDgen. Comments can start anywhere except inside a string literal.


Preprocessor Directives
~~~~~~~~~~~~~~~~~~~~~~~
FML and FMS files may contain any directive understood by the C Preprocessor. FPP files may not contain such directives other than `line directives`_.
A directive starts with a ``#`` character that must be the very first character of the line (not preceded by whitespace).
The entire line will be treated as a directive. For more detailed information on directives, consult the C Preprocessor's manual.

Includes
........
The directive ``#include "filename.fms"`` can be used to include files within other files.
The preprocessor replaces the directive with the entire content of the given file.
The filename is interpreted relative to the file which contains the directive.

Line Directives
...............
Line directives have the format ``# line file flags`` where ``line`` is a decimal integer,
``file`` a double-quoted string, and ``flags`` a whitespace-separated list of zero or more decimal integers.

The line directive sets the scanner's current line number and file name for error and status messages to the given line and file.
The ``flags`` are ignored.
This directive does not affect the file's semantics in any way, it only changes the formatting of the scanner's debug messages.

This directive should not be used directly; it is inserted automatically by the preprocessor.


Translations
============
Translatable texts are provided in YAML files, which are parsed by RCDgen together with an FPP file.
Each YAML file contains strings for one language. Strings are structured in *bundles*, which can be referenced by their name from a `strings`_ node.


Predefined Symbols
==================
RCDgen predefines a number of symbols which may be used as values.
Which symbols are available depends on the block type of the value list in which the symbol appears.

Surfaces (SURF_ Blocks)
~~~~~~~~~~~~~~~~~~~~~~~
- reserved
- the_green
- short_grass
- medium_grass
- long_grass
- semi_transparent
- sand
- cursor
- cursor_edge

Foundations (FUND_ Blocks)
~~~~~~~~~~~~~~~~~~~~~~~~~~
- reserved
- ground
- wood
- brick

Paths (PATH_ Blocks)
~~~~~~~~~~~~~~~~~~~~
- wood
- tiled
- asphalt
- concrete
- queue

Platforms (PLAT_ Blocks)
~~~~~~~~~~~~~~~~~~~~~~~~
- wood

Supports (SUPP_ Blocks)
~~~~~~~~~~~~~~~~~~~~~~~
- wood

Person Animations (ANIM_ and ANSP_ Blocks)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- guest
- handyman
- mechanic
- guard
- entertainer
- walk_ne
- walk_se
- walk_sw
- walk_nw
- mechanic_repair_ne
- mechanic_repair_se
- mechanic_repair_sw
- mechanic_repair_nw
- handyman_water_ne
- handyman_water_se
- handyman_water_sw
- handyman_water_nw
- handyman_sweep_ne
- handyman_sweep_se
- handyman_sweep_sw
- handyman_sweep_nw
- handyman_empty_ne
- handyman_empty_se
- handyman_empty_sw
- handyman_empty_nw
- guest_bench_ne
- guest_bench_se
- guest_bench_sw
- guest_bench_nw

Widgets in GBOR_ Blocks
~~~~~~~~~~~~~~~~~~~~~~~
- left_tabbar
- pressed_tab_tabbar
- tab_tabbar
- right_tabbar
- tabbar_panel
- titlebar
- button
- pressed_button
- panel

Widgets in GCHK_ Blocks
~~~~~~~~~~~~~~~~~~~~~~~
- check_box
- radio_button

Person Types (`person_graphics`_ Nodes)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- guest
- handyman
- mechanic
- guard
- entertainer

Colours (`recolour`_ Nodes)
~~~~~~~~~~~~~~~~~~~~~~~~~~~
- grey
- green_brown
- orange_brown
- yellow
- dark_red
- dark_green
- light_green
- green
- pink_brown
- dark_purple
- blue
- jade_green
- purple
- red
- orange
- sea_green
- pink
- brown

Shops (SHOP_ Blocks)
~~~~~~~~~~~~~~~~~~~~
- ne_entrance
- se_entrance
- sw_entrance
- nw_entrance
- drink
- ice_cream
- non_salt_food
- salt_food
- umbrella
- balloon 
- map
- souvenir
- money
- toilet
- first_aid

Track Voxels (`track_voxel`_ Nodes)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- north
- south
- west
- east
- nesw
- senw
- swne
- nwse

Connections (`connection`_ Nodes)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- ne
- se
- sw
- nw

Track Pieces (`track_piece`_ Nodes)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- none
- left
- right

Coaster Platforms (CSPL_ Blocks)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- wood

Fences (FENC_ Blocks)
~~~~~~~~~~~~~~~~~~~~~
- border
- wood
- conifer
- bricks


Intermediary Nodes
==================
These nodes are used to group common types of data.

Each node has mandatory and optional attributes. An attribute is a named value in the value list that represents the node.
In the following list, all attributes are mandatory unless otherwise noted.

The presence of named values that do not correspond to any known attribute generates a warning. Such values are ignored.

file
~~~~
The *file* node is the top-level node of an FML file. Unlike for other nodes,
the ``file`` keyword must be followed by a quoted string in parentheses specifying
the name of the RCD file that will be generated from this node.

The content of the file node are any number of unnamed blocks_,
which will be written to the RCD file in the order of their appearance.

The first value of a file node must be an INFO_ block.

strings
~~~~~~~
A reference to a bundle of strings from the language YAML files.

Attribute:

- ``key`` - The name of the strings bundle.

sprite
~~~~~~
Represents a single sprite. The sprite is loaded from a PNG file on the disk and clipped to the specified rectangle.

The clipping rectangle must fully lie inside the image's bounds.

It is valid to specify no named values in this node, in which case the sprite is empty. Otherwise, all attributes are mandatory.

Attributes:

- ``file`` - The PNG file path, relative to the directory in which RCDgen will be invoked.
- ``recolour`` - *Optional*. The PNG file path to the recolouring mask, relative to the directory in which RCDgen will be invoked.
- ``x_base`` - X coordinate of the upper left corner of the clipping rectangle.
- ``y_base`` - Y coordinate of the upper left corner of the clipping rectangle.
- ``width`` - Width of the clipping rectangle.
- ``height`` - Height of the clipping rectangle.
- ``x_offset`` - X offset of the clipped sprite's rendering position.
- ``y_offset`` - Y offset of the clipped sprite's rendering position.
- ``crop`` - *Optional, default 1*. Whether to crop this image by removing empty space along the edges.
- ``mask`` - *Optional*. The bitmask_ to apply to this sprite.

spritefiles
~~~~~~~~~~~
This node represents a sequence of sprite files, each of which generates a single sprite.

The attributes are the same as for the sprite_ node.

The ``file`` string may contain a template sequence similar to name templates. The template sequence consists of:

- An opening brace ``{``.
- The keyword ``seq``.
- An opening parenthesis ``(``.
- A decimal integer denoting the inclusive sequence start.
- The keyword ``..`` (two periods).
- A decimal integer denoting the inclusive sequence end. This value must not be less than the sequence start.
- A comma ``,``.
- A decimal integer denoting the number of characters in the template expansion. Each value of the sequence will be padded with leading zeros to this minimum length.
- A closing parenthesis ``)``.
- A closing brace ``}``.

If a ``recolour`` mask is set, it must expand to the same number of sprite files as the ``file`` attribute.

This node generates a matrix of sprite_ nodes. The matrix has a single row, and as many columns as generated by the sequence.

sheet
~~~~~
This node represents a group of sprites clipped from a single spritesheet.

All clipping rectangles must fully lie inside the sheet image's bounds.

The attributes are the same as for the sprite_ node, with the following additional attributes:

- ``x_step`` - Number of pixels to step in X direction from one sprite to the next.
- ``y_step`` - Number of pixels to step in Y direction from one sprite to the next.
- ``x_count`` - *Optional*. Number of sprites in the sheet in X direction. If not set, this is determined automatically from the width of the sheet and the step size.
- ``y_count`` - *Optional*. Number of sprites in the sheet in Y direction. If not set, this is determined automatically from the height of the sheet and the step size.

This node generates a matrix of sprite_ nodes. The matrix has ``y_count`` rows and ``x_count`` columns.

bitmask
~~~~~~~
The bit mask of a sprite.

Attributes:

- ``x_pos`` - X position of the mask.
- ``y_pos`` - Y position of the mask.
- ``type`` - Name of the mask. Currently only ``"voxel64"`` masks are supported.

recolour
~~~~~~~~
Specifies the recolouring information for a unit's sprites.

Attributes:

- ``original`` - The source colour.
- ``replace`` - The bitset of colours that may be used for recolouring.

person_graphics
~~~~~~~~~~~~~~~
Represents basic information about a person's graphics. Despite the name, the actual sprites are referenced from separate nodes.

Attributes:

- ``person_type`` - Type of this person.
- Optionally, up to three unnamed recolour_ nodes specifying the person's recolouring information.

frame_data
~~~~~~~~~~
Represents a person's animation's per-frame changes.

Attributes:

- ``change_x`` - Number of X pixels the person moves per animation frame.
- ``change_y`` - Number of Y pixels the person moves per animation frame.
- ``duration`` - Duration of the frame in milliseconds.

track_piece
~~~~~~~~~~~
Represents a track piece of a tracked ride. Corresponds roughly to a *TRCK* block.

Attributes:

- ``internal_name`` - Unique internal name of the track piece.
- ``track_flags`` - Bitset of track flags (see the RCD documentation for the TRCK block).
- ``banking`` - Direction in which the piece is banked.
- ``slope`` - Slope steepness of the piece.
- ``bend`` - Bend direction of the piece.
- ``cost`` - Cost of the piece.
- ``entry`` - Entry connection_ of the piece.
- ``exit`` - Exit connection_ of the piece.
- ``exit_dx`` - Exit offset in X direction.
- ``exit_dy`` - Exit offset in Y direction.
- ``exit_dz`` - Exit offset in Z direction.
- ``speed`` - *Optional*. If set, the minimum speed to which a car passing this track piece will be accelerated.
- ``car_xpos`` - Curve describing the car's X position along the track.
- ``car_ypos`` - Curve describing the car's Y position along the track.
- ``car_zpos`` - Curve describing the car's Z position along the track.
- ``car_roll`` - Curve describing the car's roll along the track.
- ``car_pitch`` - *Optional*. Curve describing the car's pitch along the track.
- ``car_yaw`` - *Optional*. Curve describing the car's yaw along the track.
- Any number of unnamed track_voxel_ nodes describing the piece's voxels.

A *curve* can be either an integer constant or a splines_ node.

track_voxel
~~~~~~~~~~~
Describes a single voxel in a track piece.

Attributes:

- ``flags`` - Bitset of flags (see the RCD documentation).
- ``dx`` - Relative X position of the voxel.
- ``dy`` - Relative Y position of the voxel.
- ``dz`` - Relative Z position of the voxel.
- ``bg`` - *Optional*. Background graphics FSET_ block.
- ``fg`` - *Optional*. Foreground graphics FSET_ block.

splines
~~~~~~~
A sequence of any number of unnamed cubic_ nodes describing a curve.

cubic
~~~~~
A cubic bezier spline curve.

Attributes:

- ``steps`` - Number of iterations in the curve. Should be at least 100.
- ``a, b, c, d`` - The parameters of the cubic equation.

connection
~~~~~~~~~~
The connection between two track pieces.

Attributes:

- ``name`` - The name of the connection. Two connections fit together if and only if their names are identical. Arbitrary names may be used; they will be converted to integer constants in the RCD file.
- ``direction`` - Direction of the connection.


Blocks
======
Each block represents a block in the generated RCD file.

See "data_format.rst" for more information about the semantics of the block types.

All notes that apply to `intermediary nodes`_ also apply to blocks.

ANIM
~~~~
Attributes:

- ``person_type`` - The person type.
- ``anim_type`` - The type of the animation.

Frame data attributes
.....................
A `frame_data`_ value named ``frame_data`` is required.
There can be *either* one such value for each frame in the animation, in which case the *n*-th value will be used for the *n*-th frame;
*or* there can be exactly one such value and an additional key ``nr_frames``, in which case the single value will be reused for every frame.

ANSP
~~~~
Attributes:

- ``tile_width`` - Zoom scale of the sprites.
- ``person_type`` - The person type.
- ``anim_type`` - The type of the animation.

Sprites
.......
The sprites must be specified in exactly one of three possible ways.

- One possibility is to add one unnamed sprite_ node for every frame.
- The second option is to use an unnamed spritefiles_ node.
- The third option is to use an unnamed sheet_ node.

In the first case, the number of frames in the animation is equal to the number of sprites;
in the second and third case, it must be specified with an attribute ``nr_frames``.

BDIR
~~~~
Attributes:

- ``tile_width`` - Zoom scale of the sprites.
- ``ne, se, sw, nw`` - Sprites for all four orientations.

CARS
~~~~
Attributes:

- ``length`` - Length of the car in 1/65,536 of a voxel.
- ``inter_length`` - Spacing between cars in 1/65,536 of a voxel.
- ``num_passengers`` - Number of passengers the car can hold.
- ``num_entrances`` - Number of entrances in the car.
- Optionally, up to three unnamed recolour_ nodes specifying the car's recolouring information.

Scales
......
The scales of the car's images may be specified either with a single attribute ``tile_width``, or with the attributes:

- ``scales`` - Number W of zoom scales in the set.
- ``tile_width_W`` - Each zoom scale's tile width.

Sprites
.......

If the scales are specified with a single ``tile_width`` parameter, the sprites are named:

- ``car_pPrRyY`` - The sprites for all orientations of the car, for values of *P* (pitch), *R* (roll), and *Y* (yaw) from 0 to 15 each.
- ``guest_sheet`` - The animation's guest overlay sheet_.

Otherwise, they are named ``car_pPrRyYwW`` and ``guest_sheet_W`` for each tile width ``W``.

CSPL
~~~~
Attributes:

- ``type`` - Type of the platform.
- ``bg`` - Background graphics FSET_ block.
- ``fg`` - Foreground graphics FSET_ block.

FENC
~~~~
Attributes:

- ``width`` - Zoom scale of the sprites.
- ``type`` - Type of the fence.
- ``ne_hor, se_hor, sw_hor, nw_hor`` - Horizontal fence graphics of the four edges.
- ``ne_n, se_e, sw_s, nw_w, ne_e, se_s, sw_w, nw_n`` - Fence graphics of the four edges with one side raised.

FGTR
~~~~
Attributes:

- ``internal_name`` - Internal name of the ride.
- ``build_cost`` - Ride construction cost.
- ``ride_width_x`` - Number of voxels occupied by this ride in X direction.
- ``ride_width_y`` - Number of voxels occupied by this ride in Y direction.
- ``height_X_Y`` - Number of voxels occupied by this ride in Z direction, for each value of X and Y from 0 to the ride's X/Y width minus 1.
- ``category`` - ``"gentle"`` or ``"thrill"``.
- ``reliability_max`` - Initial maximum reliability.
- ``reliability_decrease_daily`` - Daily reliability decrease.
- ``reliability_decrease_monthly`` - Monthly reliability decrease.
- ``cost_ownership`` - Monthly base cost of owning a ride of this type.
- ``cost_opened`` - Additional monthly base cost of owning an open ride of this type.
- ``entrance_fee`` - Default entrance fee in cents.
- ``guests_per_batch`` - Maximum number of guests per guest batch.
- ``number_of_batches`` - Maximum number of guest batches who can use the ride at the same time.
- ``idle_duration`` - Default duration how long the ride is idle between working cycles, in milliseconds.
- ``working_duration`` - Duration of one working cycle.
- ``working_cycles_min`` - Minimum number of working cycles.
- ``working_cycles_max`` - Maximum number of working cycles.
- ``working_cycles_default`` - Default number of working cycles.
- ``intensity_base`` - Base ride intensity rating.
- ``nausea_base`` - Base ride nausea rating.
- ``excitement_base`` - Base ride excitement rating.
- ``excitement_increase_cycle`` - Excitement rating increase per working cycle.
- ``excitement_increase_scenery`` - Excitement rating increase per nearby scenery item.
- ``animation_idle`` - FSET_ containing the ride's idle images.
- ``animation_starting`` - TIMA_ containing the ride's start-up animation.
- ``animation_working`` - TIMA_ containing the ride's main working animation.
- ``animation_stopping`` - TIMA_ containing the ride's spin-down animation.
- ``texts`` - strings_ node containing the ride's texts.
- Optionally, up to three unnamed recolour_ nodes specifying the ride's recolouring information.

The working duration per cycle must be at least as large as the sum of the durations of the starting, working, and stopping animations.

For rides with more than one guest batch, the starting, working, and stopping animations must be empty.

FSET
~~~~
Attributes:

- ``width_x, width_y`` - Number of voxels in X and Y direction occupied by the animation.

Scales
......
The scales of the images in the set may be specified either with a single attribute ``tile_width``, or with the attributes:

- ``scales`` - Number Z of zoom scales in the set.
- ``tile_width_Z`` - Each zoom scale's tile width.

Sprites
.......
The FSET contains one sprite for each of the (X×Y) voxels for each of the four orientations.

If the scales are specified with a single ``tile_width`` parameter,
the sprite for voxel (X,Y) at orientation O is named ``O_Y_X`` (where O is ``ne, se, sw, nw``).
Otherwise, the sprite is named ``O_Y_X_W`` for each tile width ``W``.

If the optional boolean switch ``unrotated_views_only`` is set, only north-east sprites are used for all orientations;
sprites for the other orientations may be omitted.

If the optional key ``empty_voxels`` is set to a sprite, all sprites are optional, and this sprite will be used for any missing sprites.

FTKW
~~~~
Attributes:

- ``file`` - The track design file to include.

FUND
~~~~
Attributes:

- ``tile_width, z_height`` - Zoom scale of the sprites.
- ``found_type`` - Type of the foundation.
- ``se_e0`` - Southeast foundation with visible east wall sprite.
- ``se_0s`` - Southeast foundation with visible south wall sprite.
- ``se_es`` - Southeast foundation with visible east and south walls sprite.
- ``sw_s0`` - Southwest foundation with visible south wall sprite.
- ``sw_0w`` - Southwest foundation with visible west wall sprite.
- ``sw_sw`` - Southwest foundation with visible south and west walls sprite.

GBOR
~~~~
Attributes:

- ``widget_type`` - The widget type.
- ``border_top`` - The top edge border width.
- ``border_left`` - The left edge border width.
- ``border_right`` - The right edge border width.
- ``border_bottom`` - The bottom edge border width.
- ``min_width`` - The minimal border width.
- ``min_height`` - The minimal border height.
- ``h_stepsize`` - Horizontal stepsize of the border.
- ``v_stepsize`` - Vertical stepsize of the border.
- ``top_left`` - Top-left sprite.
- ``top_middle`` - Top-middle sprite.
- ``top_right`` - Top-right sprite.
- ``middle_left`` - Left sprite.
- ``middle_middle`` - Middle sprite.
- ``middle_right`` - Right sprite.
- ``bottom_left`` - Bottom-left sprite.
- ``bottom_middle`` - Bottom-middle sprite.
- ``bottom_right`` - Bottom-right sprite.

GCHK
~~~~
Attributes:

- ``widget_type`` - The widget type.
- ``empty`` - Empty sprite.
- ``filled`` - Filled sprite.
- ``empty_pressed`` - Empty pressed sprite.
- ``filled_pressed`` - Filled pressed sprite.
- ``shaded_empty`` - Empty shaded sprite.
- ``shaded_filled`` - Filled shaded sprite.

GSCL
~~~~
Attributes:

- ``widget_type`` - The widget type.
- ``min_length`` - Minimum length.
- ``step_back`` - Background step size.
- ``min_bar_length`` - Minimal length bar.
- ``bar_step`` - Bar step size.
- ``left_button`` - Left/up button sprite.
- ``right_button`` - Right/down button sprite.
- ``left_pressed`` - Pressed left/up button sprite.
- ``right_pressed`` - Pressed right/down button sprite.
- ``left_bottom`` - Left/top bar bottom background.
- ``middle_bottom`` - Middle bar bottom background.
- ``right_bottom`` - Right/down bar bottom background.
- ``left_top`` - Left/top bar top.
- ``middle_top`` - Middle bar top.
- ``right_top`` - Right/down bar top.
- ``left_top_pressed`` - Pressed left/top bar top.
- ``middle_top_pressed`` - Pressed middle bar top.
- ``right_top_pressed`` - Pressed right/down bar top.

GSLI
~~~~
Attributes:

- ``widget_type`` - The widget type.
- ``min_length`` - Minimum length.
- ``step_size`` - Step size.
- ``width`` - Button width.
- ``left`` - Left sprite.
- ``middle`` - Middle sprite.
- ``right`` - Right sprite.
- ``slider`` - Slider button sprite.

GSLP
~~~~
Attributes:

- ``vert_down`` - Vertical downward slope trackpiece sprite.
- ``steep_down`` - Steep downward slope trackpiece sprite.
- ``gentle_down`` - Gentle downward slope trackpiece sprite.
- ``level`` - No slope trackpiece sprite.
- ``gentle_up`` - Gentle upward slope trackpiece sprite.
- ``steep_up`` - Steep upward slope trackpiece sprite.
- ``vert_up`` - Vertical upward slope trackpiece sprite.
- ``wide_left`` - Wide left bend trackpiece sprite.
- ``normal_left`` - Normal left bend trackpiece sprite.
- ``tight_left`` - Tight left bend trackpiece sprite.
- ``no_bend`` - Straight ahead trackpiece sprite.
- ``tight_right`` - Tight right bend trackpiece sprite.
- ``normal_right`` - Normal right bend trackpiece sprite.
- ``wide_right`` - Wide right bend trackpiece sprite.
- ``bank_left`` - Left banked curve trackpiece sprite.
- ``bank_right`` - Right banked curve trackpiece sprite.
- ``no_banking`` - Unbanked curve trackpiece sprite.
- ``triangle_right`` - Right arrow triangle sprite.
- ``triangle_left`` - Left arrow triangle sprite.
- ``triangle_up`` - Upward arrow triangle sprite.
- ``triangle_bottom`` - Downward arrow triangle sprite.
- ``has_platform`` - Trackpiece with platform sprite.
- ``no_platform`` - Trackpiece without platform sprite.
- ``has_power`` - Trackpiece with power sprite sprite.
- ``no_power`` - Trackpiece without power sprite sprite.
- ``disabled`` - Sprite to overlay over disabled buttons.
- ``compass_n, compass_e, compass_s, compass_w`` - Compass sprites for all four orientations.
- ``bulldozer`` - Bulldozer sprite.
- ``message_goto`` - Message Go To Location sprite.
- ``message_park`` - Message Park Management sprite.
- ``message_guest`` - Message Guest sprite.
- ``message_ride`` - Message Ride sprite.
- ``message_ride_type`` - Message Ride Type sprite.
- ``loadsave_err`` - Error sprite for the loadsave window.
- ``loadsave_warn`` - Warning sprite for the loadsave window.
- ``loadsave_ok`` - OK sprite for the loadsave window.
- ``toolbar_main`` - Toolbar main menu sprite.
- ``toolbar_speed`` - Toolbar speed menu sprite.
- ``toolbar_path`` - Toolbar path building sprite.
- ``toolbar_ride`` - Toolbar ride selection sprite.
- ``toolbar_fence`` - Toolbar fence building sprite.
- ``toolbar_scenery`` - Toolbar scenery placement sprite.
- ``toolbar_terrain`` - Toolbar landscaping sprite.
- ``toolbar_staff`` - Toolbar staff management sprite.
- ``toolbar_inbox`` - Toolbar inbox sprite.
- ``toolbar_finances`` - Toolbar finances management sprite.
- ``toolbar_objects`` - Toolbar path objects sprite.
- ``toolbar_view`` - Toolbar view menu sprite.
- ``toolbar_park`` - Toolbar park management sprite.
- ``speed_0`` - 0× speed icon.
- ``speed_1`` - 1× speed icon.
- ``speed_2`` - 2× speed icon.
- ``speed_4`` - 4× speed icon.
- ``speed_8`` - 8× speed icon.
- ``sunny", "light_cloud", "thick_cloud", "rain", "thunder`` - Sprites for all weather conditions.
- ``light_rog_red", "light_rog_orange", "light_rog_green", "light_rog_none`` - Sprites for red/orange/green indicators.
- ``light_rg_red", "light_rg_green", "light_rg_none`` - Sprites for red/green indicators.
- ``pos_2d`` - Flat +rotation positive direction sprite.
- ``neg_2d`` - Flat rotation negative direction sprite.
- ``pos_3d`` - Diametric rotation positive direction sprite.
- ``neg_3d`` - Diametric rotation negative direction sprite sprite.
- ``close_button`` - Window close button sprite.
- ``terraform_dot`` - Terraform dot sprite.
- ``texts`` - strings_ node containing the GUI's texts.

INFO
~~~~
Attributes:

- ``name`` - Human-readable name of the RCD file.
- ``description`` - *Optional*. Human-readable description of the RCD file.
- ``uri`` - Unique identifier for the RCD file.
- ``website`` - *Optional*. Link to the RCD file's website.

MENU
~~~~
Attributes:

- ``splash_duration`` - Splash screen duration in milliseconds.
- ``splash`` - Splash screen sprite.
- ``logo`` - FreeRCT logo sprite.
- ``new_game`` - New Game button sprite.
- ``load_game`` - Load Game button sprite.
- ``launch_editor`` - Scenario Editor button sprite.
- ``settings`` - Settings button sprite.
- ``quit`` - Quit button sprite.

PATH
~~~~
Attributes:

- ``tile_width, z_height`` - Zoom scale of the sprites.
- ``path_type`` - Path type.
- ``empty`` - Unconnected path sprite.
- ``ne`` - Northeast connected path sprite.
- ``se`` - Southeast connected path sprite.
- ``ne_se`` - Northeast and southeast connected path sprite.
- ``ne_se_e`` - Northeast, southeast, and east connected path sprite.
- ``sw`` - Southwest connected path sprite.
- ``ne_sw`` - Northeast and southwest connected path sprite.
- ``se_sw`` - Southeast and southwest connected path sprite.
- ``se_sw_s`` - Southeast, southwest, and south connected path sprite.
- ``ne_se_sw`` - Northeast, southeast, and southwest connected path sprite.
- ``ne_se_sw_e`` - Northeast, southeast, southwest, and east connected path sprite.
- ``ne_se_sw_s`` - Northeast, southeast, southwest, and south connected path sprite.
- ``ne_se_sw_e_s`` - Northeast, southeast, southwest, east, and south connected path sprite.
- ``nw`` - Northwest connected path sprite.
- ``ne_nw`` - Northeast and northwest connected path sprite.
- ``ne_nw_n`` - Northeast, northwest, and north connected path sprite.
- ``nw_se`` - Northwest and southeast connected path sprite.
- ``ne_nw_se`` - Northeast, northwest, and southeast connected path sprite.
- ``ne_nw_se_n`` - Northeast, northwest, southeast, and north connected path sprite.
- ``ne_nw_se_e`` - Northeast, northwest, southeast, and east connected path sprite.
- ``ne_nw_se_n_e`` - Northeast, northwest, southeast, north, and east connected path sprite.
- ``nw_sw`` - Northwest and southwest connected path sprite.
- ``nw_sw_w`` - Northwest, southwest, and west connected path sprite.
- ``ne_nw_sw`` - Northwest, northwest, and southwest connected path sprite.
- ``ne_nw_sw_n`` - Northwest, northwest, southwest, and north connected path sprite.
- ``ne_nw_sw_w`` - Northwest, northwest, southwest, and west connected path sprite.
- ``ne_nw_sw_n_w`` - Northwest, northwest, southwest, north, and west connected path sprite.
- ``nw_se_sw`` - Northwest, southeast, and southwest connected path sprite.
- ``nw_se_sw_s`` - Northwest, southeast, southwest, and south connected path sprite.
- ``nw_se_sw_w`` - Northwest, southeast, southwest, and west connected path sprite.
- ``nw_se_sw_s_w`` - Northwest, southeast, southwest, south, and west connected path sprite.
- ``ne_nw_se_sw`` - Northeast, northwest, southeast, and southwest connected path sprite.
- ``ne_nw_se_sw_n`` - Northeast, northwest, southeast, southwest, and north connected path sprite.
- ``ne_nw_se_sw_e`` - Northeast, northwest, southeast, southwest, and east connected path sprite.
- ``ne_nw_se_sw_n_e`` - Northeast, northwest, southeast, southwest, north, and east connected path sprite.
- ``ne_nw_se_sw_s`` - Northeast, northwest, southeast, southwest, and south connected path sprite.
- ``ne_nw_se_sw_n_s`` - Northeast, northwest, southeast, southwest, north, and south connected path sprite.
- ``ne_nw_se_sw_e_s`` - Northeast, northwest, southeast, southwest, east, and south connected path sprite.
- ``ne_nw_se_sw_n_e_s`` - Northeast, northwest, southeast, southwest, north, east, and south connected path sprite.
- ``ne_nw_se_sw_w`` - Northeast, northwest, southeast, southwest, and west connected path sprite.
- ``ne_nw_se_sw_n_w`` - Northeast, northwest, southeast, southwest, north, and west connected path sprite.
- ``ne_nw_se_sw_e_w`` - Northeast, northwest, southeast, southwest, east, and west connected path sprite.
- ``ne_nw_se_sw_n_e_w`` - Northeast, northwest, southeast, southwest, north, east, and west connected path sprite.
- ``ne_nw_se_sw_s_w`` - Northeast, northwest, southeast, southwest, south, and west connected path sprite.
- ``ne_nw_se_sw_n_s_w`` - Northeast, northwest, southeast, southwest, north, south, and west connected path sprite.
- ``ne_nw_se_sw_e_s_w`` - Northeast, northwest, southeast, southwest, east, south, and west connected path sprite.
- ``ne_nw_se_sw_n_e_s_w`` - Northeast, northwest, southeast, southwest, north, east, south, and west connected path sprite.
- ``ramp_ne`` - Path sloping to the northeast sprite.
- ``ramp_nw`` - Path sloping to the northwest sprite.
- ``ramp_se`` - Path sloping to the southeast sprite.
- ``ramp_sw`` - Path sloping to the southwest sprite.

PDEC
~~~~
Attributes:

- ``tile_width`` - Zoom scale of the sprites.
- ``lamp_post_ne, lamp_post_se, lamp_post_sw, lamp_post_nw`` - Lamp post sprites for all four orientations.
- ``demolished_post_ne, demolished_post_se, demolished_post_sw, demolished_post_nw`` - Demolished lamp post sprites for all four orientations.
- ``litter_bin_ne, litter_bin_se, litter_bin_sw, litter_bin_nw`` - Litter bin sprites for all four orientations.
- ``overflow_bin_ne, overflow_bin_se, overflow_bin_sw, overflow_bin_nw`` - Overflowing litter bin sprites for all four orientations.
- ``demolished_bin_ne, demolished_bin_se, demolished_bin_sw, demolished_bin_nw`` - Demolished litter bin sprites for all four orientations.
- ``bench_ne, bench_se, bench_sw, bench_nw`` - Bench sprites for all four orientations.
- ``demolished_bench_ne, demolished_bench_se, demolished_bench_sw, demolished_bench_nw`` - Demolished bench sprites for all four orientations.
- ``litter_flat, litter_ne, litter_se, litter_sw, litter_nw`` - Litter sprites for flat and ramped paths. Each key may be present up to 4 times.
- ``vomit_flat, vomit_ne, vomit_se, vomit_sw, vomit_nw`` - Vomit sprites for flat and ramped paths. Each key may be present up to 4 times.

PLAT
~~~~
Attributes:

- ``tile_width, z_height`` - Zoom scale of the sprites.
- ``platform_type`` - Type of the platform.
- ``ns`` - North-south platform sprite.
- ``ew`` - East-west platform sprite.
- ``ramp_ne`` - Northeast sloping platform sprite.
- ``ramp_se`` - Southeast sloping platform sprite.
- ``ramp_sw`` - Southwest sloping platform sprite.
- ``ramp_nw`` - Southeast sloping platform sprite.
- ``right_ramp_ne`` - Right northeast sloping platform sprite.
- ``right_ramp_se`` - Right southeast sloping platform sprite.
- ``right_ramp_sw`` - Right southwest sloping platform sprite.
- ``right_ramp_nw`` - Right northwest sloping platform sprite.
- ``left_ramp_ne`` - Left northeast sloping platform sprite.
- ``left_ramp_se`` - Left southeast sloping platform sprite.
- ``left_ramp_sw`` - Left southwest sloping platform sprite.
- ``left_ramp_nw`` - Left northwest sloping platform sprite.

PRSG
~~~~
Attributes:

- ``person_graphics`` - The person_graphics_ node.

RCST
~~~~
Attributes:

- ``internal_name`` - Internal name of the ride.
- ``coaster_type`` - Type of the coaster.
- ``platform_type`` - Type of the coaster's platforms.
- ``max_number_trains`` - Maximum allowed number of trains.
- ``max_number_cars`` - Maximum allowed number of cars per train.
- ``reliability_max`` - Initial maximum reliability.
- ``reliability_decrease_daily`` - Daily reliability decrease.
- ``reliability_decrease_monthly`` - Monthly reliability decrease.
- ``texts`` - strings_ node containing the ride's texts.
- Any number of unnamed `track_piece`_ nodes.

RIEE
~~~~
Attributes:

- ``internal_name`` - Internal name of the entrance/exit type.
- ``type`` - ``"entrance"`` or ``"exit"``.
- ``bg`` - Background graphics FSET_ block.
- ``fg`` - Foreground graphics FSET_ block.
- ``texts`` - strings_ node containing the object's texts.

SCNY
~~~~
Attributes:

- ``internal_name`` - Internal name of the item.
- ``width_x`` - Number of voxels occupied by this item in X direction.
- ``width_y`` - Number of voxels occupied by this item in Y direction.
- ``category`` - Numeric scenery category ID of the item (see the RCD documentation for the SCNY block).
- ``buy_cost`` - The amount of money it costs to buy this item, in cents.
- ``remove_cost`` - The amount of money it costs to remove this item, in cents. May be negative if removing it returns money.
- ``watering_interval`` - How many milliseconds after watering the item falls dry. 0 means never.
- ``symmetric`` - *Optional*. If set, only north-east views of the animations will be used.
- ``preview_ne, preview_se,preview_sw, preview_nw`` - The previews of this item for all orientations (or only for north-east if symmetric).
- ``main_animation`` - The main TIMA_ animation.
- ``height_X_Y`` - Number of voxels occupied by this item in Z direction, for each value of X and Y from 0 to the item's X/Y width minus 1.
- ``texts`` - strings_ node containing the item's texts.

If the watering interval is not zero, the following attributes are required:

- ``min_watering_interval`` - The minimum time in milliseconds that must pass between repeatedly watering the item.
- ``dry_animation`` - The TIMA_ animation to display while the item is dry.
- ``return_cost_dry`` - The ``remove_cost`` while the item is dry.

SHOP
~~~~
Attributes:

- ``internal_name`` - Internal name of the shop.
- ``build_cost`` - Shop construction cost.
- ``height`` - Height of the shop in voxels.
- ``flags`` - Bitset of the shop's entrance directions.
- ``cost_item1`` - Cost of the first sold item in cents.
- ``cost_item2`` - Cost of the second sold item in cents.
- ``type_item1`` - Type of the first sold item.
- ``type_item2`` - Type of the second sold item.
- ``cost_ownership`` - Monthly base cost of owning a shop of this type.
- ``cost_opened`` - Additional monthly base cost of owning an open shop of this type.
- ``images`` - The FSET_ containing the shop's images.
- ``texts`` - strings_ node containing the shop's texts.
- Optionally, up to three unnamed recolour_ nodes specifying the shop's recolouring information.

SUPP
~~~~
Attributes:

- ``tile_width, z_height`` - Zoom scale of the sprites.
- ``support_type`` - Supports type.
- ``s_ns`` - Single-height for flat terrain north-south support sprite.
- ``s_ew`` - Single-height for flat terrain east-west support sprite.
- ``d_ns`` - Double-height for flat terrain north-south support sprite.
- ``d_ew`` - Double-height for flat terrain east-west support sprite.
- ``p_ns`` - Double height for paths north-south support sprite.
- ``p_ew`` - Double height for paths east-west support sprite.
- ``n#n`` - Single-height north support sprite.
- ``n#e`` - Single-height east support sprite.
- ``n#ne`` - Single-height north and east support sprite.
- ``n#s`` - Single-height south support sprite.
- ``n#ns`` - Single-height north and south support sprite.
- ``n#es`` - Single-height east and south support sprite.
- ``n#nes`` - Single-height north, east, and south support sprite.
- ``n#w`` - Single-height west support sprite.
- ``n#nw`` - Single-height north and west support sprite.
- ``n#ew`` - Single-height east and west support sprite.
- ``n#new`` - Single-height north, east, and west support sprite.
- ``n#sw`` - Single-height south and west support sprite.
- ``n#nsw`` - Single-height north, south, and west support sprite.
- ``n#esw`` - Single-height east, south, and west support sprite.
- ``n#N`` - Steep north slope support sprite.
- ``n#E`` - Steep east slope support sprite.
- ``n#S`` - Steep south slope support sprite.
- ``n#W`` - Steep west slope support sprite.

SURF
~~~~
Attributes:

- ``tile_width, z_height`` - Zoom scale of the sprites.
- ``surf_type`` - Type of the surface.
- ``n#`` - Flat surface sprite.
- ``n#n`` - Raised north corner surface sprite.
- ``n#e`` - Raised east corner surface sprite.
- ``n#ne`` - Raised north and east corners surface sprite.
- ``n#s`` - Raised south corner surface sprite.
- ``n#ns`` - Raised south and north corners surface sprite.
- ``n#es`` - Raised east and south corners surface sprite.
- ``n#nes`` - Raised north, east, and south corners surface sprite.
- ``n#w`` - Raised west corner surface sprite.
- ``n#nw`` - Raised north and west surface sprite.
- ``n#ew`` - Raised east and west surface sprite.
- ``n#new`` - Raised north, east, and west surface sprite.
- ``n#sw`` - Raised south and west surface sprite.
- ``n#nsw`` - Raised north, south, and west surface sprite.
- ``n#esw`` - Raised east, south, and west surface sprite.
- ``n#Nb`` - Steep northern slope bottom surface sprite.
- ``n#Eb`` - Steep eastern slope bottom surface sprite.
- ``n#Sb`` - Steep southern slope bottom surface sprite.
- ``n#Wb`` - Steep western slope bottom surface sprite.
- ``n#Nt`` - Steep northern slope top surface sprite.
- ``n#Et`` - Steep eastern slope top surface sprite.
- ``n#St`` - Steep southern slope top surface sprite.
- ``n#Wt`` - Steep western slope top surface sprite.

TCOR
~~~~
The attributes are the same as for the SURF_ block.

TIMA
~~~~
Attributes:

- ``frames`` - Number of frames in the animation.

Duration
........
The duration can be specified *either* with the key ``fps``, in which case all frames have the same duration;
*or* with keys ``duration_N`` (where *N* ranges from 0 to ``(frames - 1)``) to set each frame's duration in milliseconds.

Frames
......
The frames can be specified *either* with a single sheet_ node named ``sheet``,
in which case all frames are generated automatically from the sheet;
*or* by providing FSET_ blocks named ``frame_N`` (where *N* ranges from 0 to ``(frames - 1)``).

TSEL
~~~~
The attributes are the same as for the SURF_ block.
