# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: doc viewdoc build run rcd clean all

all: rcd build

build:
	$(MAKE) -C src

rcd:
	$(MAKE) -C src/rcdgen
	$(MAKE) -C graphics/rcd

clean:
	$(MAKE) -C src clean
	$(MAKE) -C graphics/rcd clean
	$(MAKE) -C src/rcdgen clean

run:
	$(MAKE) -C src run

doc:
	doxygen Doxyfile
	@if test -s doxygen_warnings.txt;\
	then echo "Warnings generated, see doxygen_warnings.txt.";\
	fi

viewdoc: doc
	firefox doc/html/index.html
