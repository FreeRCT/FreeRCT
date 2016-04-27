"""
This file is part of FreeRCT.
FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
"""

from PIL import Image
import os
import argparse

#tile widths to expect for renders
tilew = (64, 128, 256)

#parser for command line arguments
parser = argparse.ArgumentParser(description = "Merge sprites in a directory into a sprite sheet.")
parser.add_argument("fpath", help = "directory to work in")
args = parser.parse_args()

def mask(fpath):
    fpath = fpath.rstrip(os.sep)
    filearr = [f for f in os.listdir(fpath) if os.path.isfile(os.path.join(fpath, f))]
    for i in filearr:
        if i.endswith(".png"):
            for j in tilew:
                name = os.path.splitext(i)[0]
                print(name)
                if name.endswith(str(j)):
                    for k in range(3):
                        if k == 0:
                            string = ""
                        else:
                            string = str(k)
                        final = Image.open(fpath + os.sep + name + ".png")
                        maskSource = Image.open(fpath + os.sep + "mask" + os.sep + str(j) + "px" + string + ".png")
                        width, height = final.size
                        mask = Image.new('RGBA', (width,height), (255, 0, 0, 0))
                        mheight = maskSource.size[1]
                        for l in range(int(height / mheight)):
                            mask.paste(maskSource, (0, mheight * l))
                        mask_p = mask.load()
                        final_p = final.load()
                        for y in range(height):
                            for x in range(width):
                                if mask_p[x,y] == 0 or mask_p[x,y] == (0,0,0,255):
                                    final_p[x,y] = (255, 0, 0, 0)
                        final.save(fpath + os.sep + name + "_masked" + string + ".png", "PNG")

mask(**vars(args))
