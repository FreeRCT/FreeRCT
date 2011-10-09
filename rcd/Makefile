# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

RCDFILES=borderhighlight_8bpp64.rcd cornerhighlight_8bpp64.rcd \
	foundation_8bpp64.rcd groundtile_8bpp64.rcd path_8bpp64.rcd

PYTHON=python

.PHONY: build clean

build: $(RCDFILES)

clean:
	for i in $(RCDFILES);\
	do if test -f $$i;\
	   then rm $$i;\
	   fi;\
	done

borderhighlight_8bpp64.rcd: borderhighlight_8bpp64.py ../sprites_src/terrain/groundtiles/borderhighlight8bpp64_masked.png
	$(PYTHON) borderhighlight_8bpp64.py --output borderhighlight_8bpp64.rcd \
		../sprites_src/terrain/groundtiles/borderhighlight8bpp64_masked.png

foundation_8bpp64.rcd: foundation_8bpp64.py ../sprites_src/terrain/foundation/template8bpp64_masked.png
	$(PYTHON) foundation_8bpp64.py --output foundation_8bpp64.rcd \
		../sprites_src/terrain/foundation/template8bpp64_masked.png

groundtile_8bpp64.rcd: groundtile_8bpp64.py ../sprites_src/terrain/groundtiles/template8bpp64_masked.png
	$(PYTHON) groundtile_8bpp64.py --output groundtile_8bpp64.rcd \
		../sprites_src/terrain/groundtiles/template8bpp64_masked.png

cornerhighlight_8bpp64.rcd: cornerhighlight_8bpp64.py ../sprites_src/terrain/groundtiles/cornerhighlight8bpp64_masked.png
	$(PYTHON) cornerhighlight_8bpp64.py --output cornerhighlight_8bpp64.rcd \
		../sprites_src/terrain/groundtiles/cornerhighlight8bpp64_masked.png

path_8bpp64.rcd: path_8bpp64.py ../sprites_src/paths/path/template8bpp64.png
	$(PYTHON) path_8bpp64.py --output path_8bpp64.rcd \
		../sprites_src/paths/path/template8bpp64.png
