# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

PYTHON=python

.PHONY: build

build: borderhighlight_8bpp64.rcd cornerhighlight_8bpp64.rcd \
	foundation_8bpp64.rcd groundtile_8bpp64.rcd path_8bpp64.rcd

borderhighlight_8bpp64.rcd: borderhighlight_8bpp64.py ../sprites/borderhighlight8bpp64_masked.png
	$(PYTHON) borderhighlight_8bpp64.py

foundation_8bpp64.rcd: foundation_8bpp64.py ../sprites/foundationtemplate8bpp64_masked.png
	$(PYTHON) foundation_8bpp64.py

groundtile_8bpp64.rcd: groundtile_8bpp64.py ../sprites/groundtiletemplatetemplate8bpp64_masked.png
	$(PYTHON) groundtile_8bpp64.py

cornerhighlight_8bpp64.rcd: cornerhighlight_8bpp64.py ../sprites/cornerhighlight8bpp64_masked.png
	$(PYTHON) cornerhighlight_8bpp64.py --output cornerhighlight_8bpp64.rcd \
		../sprites/cornerhighlight8bpp64_masked.png

path_8bpp64.rcd: path_8bpp64.py ../sprites/pathtemplate8bpp64.png
	$(PYTHON) path_8bpp64.py --output path_8bpp64.rcd \
		../sprites/pathtemplate8bpp64.png
