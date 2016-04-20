"""
This file is part of FreeRCT.
FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
"""

from PIL import Image
import os
import math
import argparse

#tile widths to expect for renders
tilew = (64, 128, 256)

#length of string for tile index number
indexleng = 5

#parser for command line arguements
parser = argparse.ArgumentParser(description = "Merge sprites in a directory into a sprite sheet.")
parser.add_argument("fpath", help = "directory to work in")
parser.add_argument("w", help = "columns in sprite sheet (default: 4)", type = int, nargs="?", default = "4")
parser.add_argument("h", help = "rows in sprite sheet (default: 8)", type = int, nargs="?", default = "8")
parser.add_argument("autoh", help = "whether or not to auto-count the rows in the sprite sheet (default: True)", type = bool, nargs="?", default = "True")
parser.add_argument("tiw", help = "X scale factor (default: 1)", type = int, nargs="?", default = "1")
parser.add_argument("tih", help = "Y scale factor (default: 1)", type = int, nargs="?", default = "1")
args = parser.parse_args()

#parameters: directory, columns, rows, auto-count, x size factor, y size factor
def mergeRenders(fpath, w, h, autoh, tiw, tih):
    #additional directory variables
    fpath = fpath.rstrip(os.sep)
    path = os.path.abspath(os.path.join(fpath, os.pardir)) + os.sep
    name = os.path.basename(fpath)

    if autoh == True:
        #count for auto height
        for i in tilew:
            j = 1
            string = str(j).zfill(indexleng)
            while os.path.exists(path + name + os.sep + str(i) + "_" + string + ".png") == True:
                j += 1
                string = str(j).zfill(indexleng)
        h = math.floor((j - 1) / w)

    #loop through tile widths
    for i in tilew:
        #calculate output image size and make output image
        iw = i * tiw * w
        ih = i * tih * h
        final = Image.new('RGBA', (iw,ih), (255, 0, 0, 0))
        j = 1
        string = str(j).zfill(indexleng)
        while os.path.exists(path + name + os.sep + str(i) + "_" + string + ".png") == True:
            img = Image.open(path + name + os.sep + str(i) + "_" + string + ".png")
            final.paste(img, (((j - 1) % w) * i * tiw, math.floor((j - 1) / w) * i * tih))
            j += 1
            string = str(j).zfill(indexleng)
        final.save(path + name + str(i) + ".png", "PNG")

#run main function
mergeRenders(**vars(args))
