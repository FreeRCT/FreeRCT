# 32bpp sprites #

We decided to switch to 32bpp images as it removes a conversion step in the image generation process, and we want to have such sprites anyway.

This page defines what images exist for a sprite and how to render them. Further details can be found in the [Data format spec](data_format.rst).

# Rendering the sprites #

The directory structure defines what kind of sprites to expect in a given directory.
For example `rides/ride_shops/ice_cream` contains sprites for an ice cream shop.

The images themselves are 32bpp PNG files named `<size>_<number>.png`, where `<size>` is the size of a tile (64, 128, or 256 pixels), and `<number>` is a four-digit increasing number, starting with `0001`.
All images have the same size, and have the origin at exactly the same point (for each size). This makes it easy to perform automatic (post-)processing of these images.

## Recolouring ##

The images cannot usually be drawn directly, since in the game parts of rides and similar can be recoloured by the user. Usually two or three parts can be given a new colour. Where to find these parts in the images is given in the corresponding `<size>p_<number>.png` 8bpp PNG file. The index of each pixel in the latter image defines which recolour part each pixel belongs to.

Part `0` denotes that the pixel should not be recolored. For each of the other parts, a recolour RGB table (see below) is assigned with 256 entries. Recolouring is performed by querying the (R, G, B) colour values of the 32bpp pixel, and computing the brightness of it with `max(R, G, B)`. The brightness is the index in the recolour table.
The following recolour tables exist:

* COL_RANGE_GREY
* COL_RANGE_GREEN_BROWN
* COL_RANGE_ORANGE_BROWN
* COL_RANGE_YELLOW
* COL_RANGE_DARK_RED
* COL_RANGE_DARK_GREEN
* COL_RANGE_LIGHT_GREEN
* COL_RANGE_GREEN
* COL_RANGE_PINK_BROWN
* COL_RANGE_DARK_PURPLE
* COL_RANGE_BLUE
* COL_RANGE_DARK_JADE_GREEN
* COL_RANGE_PURPLE
* COL_RANGE_RED
* COL_RANGE_ORANGE
* COL_RANGE_SEA_GREEN
* COL_RANGE_PINK
* COL_RANGE_BROWN

The opacity of the recoloured pixel is copied from the original 32bpp pixel.

## Overall brightness ##

After recolouring comes overall brightness adjustment. On sunny days, everything looks brightly coloured, while on dark days, the colours are also less bright.
The game has 11 steps in brightness, from -5 to +5. Brightness adjustment is done by computing the RGB color values by a factor between 0.75 and 1.25.
Technically it changes both brightness and contrast, but this is considered acceptable.

## Transparency ##

Last but not least, the opacity needs to be handled. Opacity `0` (fully transparent) means that the new pixel is completely invisible, thus the background should stay fully visible. Opacity `255` means the new pixel must fully overwrite whatever is at the background. The general formula is `(FC * A + BC * (255 - A)) / 255` where `FC` is the value of a RGB colour value of the new pixel, `BC` is the already present RGB colour value of the background, and `A` is opacity of the new pixel.

Obviously, special cases for `A = 0` and `A = 255` are useful for rendering quickly.
