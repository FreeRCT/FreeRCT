# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

GAMEDATA=borderhighlight_8bpp64.rcd \
	cornerhighlight_8bpp64.rcd \
	groundtile_8bpp64.rcd people_animation.rcd

PATHDATA=path_8bpp64.rcd
SHOPDATA=shops.rcd
GUIDATA=gui.rcd

RCDFILES=$(GAMEDATA) $(PATHDATA) $(GUIDATA) ${SHOPDATA}
PYTHON=python

.PHONY: build clean

build: $(RCDFILES)

$(GAMEDATA): freerct.xml structdef.xml
	$(PYTHON) convert.py -d structdef.xml -g freerct.xml

$(PATHDATA): paths.xml structdef.xml
	$(PYTHON) convert.py -d structdef.xml -g paths.xml

$(SHOPDATA): shops.xml shops_strings.txt structdef.xml
	$(PYTHON) convert.py -d structdef.xml -g shops.xml

$(GUIDATA): gui.xml gui_strings.txt gui_default_lang.xml gui_nl_NL_lang.xml structdef.xml
	$(PYTHON) convert.py -d structdef.xml -g gui.xml

data_format.html: data_format.rst
	rst2html data_format.rst data_format.html

clean:
	for i in $(RCDFILES);\
	do if test -f $$i;\
	   then rm $$i;\
	   fi;\
	done
