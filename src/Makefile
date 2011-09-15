# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: run clean

CXX=g++
CXXFLAGS=-Wall -g
INCLUDES=-I/usr/include/SDL
LIBS=-lSDL

CXXFILES=voxel_map.cpp main.cpp video.cpp
OBJFILES=$(CXXFILES:.cpp=.o)
TARGET=stackedtiles

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

run: $(TARGET)
	./$(TARGET)

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

