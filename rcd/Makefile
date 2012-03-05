# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

RCDFILES=borderhighlight_8bpp64.rcd buildarrows_8bpp64.rcd cornerhighlight_8bpp64.rcd \
	cursor_groundtile_8bpp64.rcd \
	gui.rcd foundation_8bpp64.rcd groundtile_8bpp64.rcd path_8bpp64.rcd

PYTHON=python

.PHONY: build clean doc

#build: $(RCDFILES)
build: gui.rcd
	$(PYTHON) convert.py -d structdef.xml -g freerct.xml


clean:
	for i in $(RCDFILES);\
	do if test -f $$i;\
	   then rm $$i;\
	   fi;\
	done

# Build documentation (run it several times to get the referenced pages correct)
doc:
	pdflatex data_format.tex
	pdflatex data_format.tex
	pdflatex data_format.tex

gui.rcd: guis.py ../sprites_src/gui/GUIEles.png
	$(PYTHON) guis.py
