# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

GAMEDATA=borderhighlight_8bpp64.rcd buildarrows_8bpp64.rcd \
	cornerhighlight_8bpp64.rcd cursor_groundtile_8bpp64.rcd foundation_8bpp64.rcd \
	groundtile_8bpp64.rcd path_8bpp64.rcd people_animation.rcd

GUIDATA=gui.rcd

RCDFILES=$(GAMEDATA) $(GUIDATA)
PYTHON=python

.PHONY: build clean

build: $(GAMEDATA) $(GUIDATA)

$(GAMEDATA): freerct.xml structdef.xml
	$(PYTHON) convert.py -d structdef.xml -g freerct.xml


clean:
	for i in $(RCDFILES);\
	do if test -f $$i;\
	   then rm $$i;\
	   fi;\
	done

gui.rcd: guis.py ../sprites_src/gui/GUIEles.png
	$(PYTHON) guis.py
