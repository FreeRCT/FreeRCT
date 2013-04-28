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
RCDGEN=../rcdgen/src/rcdgen

.PHONY: build clean rcdgen

build: $(RCDFILES)

$(GAMEDATA): rcdgen freerct.txt
	$(RCDGEN) freerct.txt

$(PATHDATA): rcdgen paths.txt
	$(RCDGEN) paths.txt

$(SHOPDATA): rcdgen shops.txt
	$(RCDGEN) shops.txt

$(GUIDATA): rcdgen gui.txt gui_default_lang.txt gui_nl_NL_lang.txt
	$(RCDGEN) gui.txt

#$(TRACKDATA): basic_tracks.xml structdef.xml
#	$(PYTHON) convert.py -d structdef.xml -g basic_tracks.xml

data_format.html: data_format.rst
	rst2html data_format.rst data_format.html

rcdgen:
	$(MAKE) -C ../rcdgen/src rcdgen

clean:
	for i in $(RCDFILES);\
	do if test -f $$i;\
	   then rm $$i;\
	   fi;\
	done
