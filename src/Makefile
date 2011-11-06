# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: run clean rcd

CXX=g++
CXXFLAGS=-Wall -g
INCLUDES=-I/usr/include/SDL
LIBS=-lSDL -lSDL_ttf

CXXFILES=fileio.cpp main.cpp map.cpp palette.cpp path.cpp \
	sprite_store.cpp string_func.cpp toolbar_gui.cpp video.cpp \
	viewport.cpp widget.cpp window.cpp

OBJFILES=$(CXXFILES:.cpp=.o)
TARGET=freerct

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

run: $(TARGET)
	./$(TARGET)

rcd:
	$(MAKE) -C ../rcd

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) $(INCLUDES) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,' > $@

clean:
	for i in $(OBJFILES) $(CXXFILES:.cpp=.d) $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done

ifndef NODEPS
include $(CXXFILES:.cpp=.d)
endif

