# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: run clean rcd lang

PYTHON=python

CXX=g++
CXXFLAGS=-Wall -g
INCLUDES=-I/usr/include/SDL
LIBS=-lSDL -lSDL_ttf

CXXFILES=config_reader.cpp fileio.cpp language.cpp main.cpp map.cpp math_func.cpp \
	palette.cpp path.cpp path_gui.cpp \
	sprite_store.cpp string_func.cpp toolbar_gui.cpp video.cpp \
	viewport.cpp widget.cpp window.cpp

OBJFILES=$(CXXFILES:.cpp=.o)
TARGET=freerct

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

run: lang $(TARGET)
	./$(TARGET)

lang: ../rcd/english.lang

../rcd/english.lang: ../rcd/process_languages.py lang/english.txt
	$(PYTHON) ../rcd/process_languages.py --directory lang --output-language ../rcd/english.lang

rcd:
	$(MAKE) -C ../rcd

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.d: %.cpp table/strings.h
	$(CXX) -MM $(CXXFLAGS) $(INCLUDES) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,' > $@

table/strings.h: lang/english.txt ../rcd/process_languages.py
	if test ! -d table;\
	then mkdir table;\
	fi
	$(PYTHON) ../rcd/process_languages.py --directory lang --output-include table/strings.h

clean:
	for i in $(OBJFILES) $(CXXFILES:.cpp=.d) table/strings.h $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done

ifndef NODEPS
include $(CXXFILES:.cpp=.d)
endif

