# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: run clean rcd lang all

OBJDIR=objects
RCDDIR=../rcd

PYTHON=python
GEN_NAMES_PY=$(RCDDIR)/gen_string_names.py

CXX=g++
CXXFLAGS=-Wall -g
INCLUDES=-I$(OBJDIR) -I/usr/include/SDL
LIBS=-lSDL -lSDL_ttf

CXXFILES=config_reader.cpp dates.cpp \
	fileio.cpp language.cpp main.cpp map.cpp math_func.cpp \
	palette.cpp path.cpp path_build.cpp path_gui.cpp \
	people.cpp person.cpp \
	random.cpp ride_type.cpp \
	sprite_store.cpp string_func.cpp \
	tile_func.cpp toolbar_gui.cpp video.cpp \
	viewport.cpp widget.cpp window.cpp

OBJFILES=$(addprefix $(OBJDIR)/,$(CXXFILES:.cpp=.o))
DEPEND=$(OBJDIR)/depend.txt
TARGET=freerct

all: lang rcd $(TARGET)
	@if test ! -f freerct.cfg;\
	then echo;\
	     echo "Failed to find font settings. Did you make a 'freerct.cfg' file next to the 'freerct' program?";\
	     echo "Example content (you need to change the path and/or the size to something that exists at your system):";\
	     echo "[font]";\
	     echo "medium-size = 12";\
	     echo "medium-path = /usr/share/fonts/gnu-free/FreeSans.ttf";\
	     exit 1;\
	fi


$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

run: lang $(TARGET)
	./$(TARGET)

lang: ../rcd/english.lang

../rcd/english.lang: ../rcd/process_languages.py lang/english.txt
	$(PYTHON) ../rcd/process_languages.py --directory lang --output-language ../rcd/english.lang

rcd:
	$(MAKE) -C $(RCDDIR)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

$(DEPEND): $(OBJDIR)/table/strings.h $(OBJDIR)/table/shops_strings.h $(CXXFILES)
	for f in $(CXXFILES);\
	do $(CXX) -MM $(CXXFLAGS) $(INCLUDES) $$f;\
	done > $(DEPEND)

$(OBJDIR)/table/strings.h: lang/english.txt ../rcd/process_languages.py
	$(PYTHON) ../rcd/process_languages.py --directory lang --output-include $@

$(OBJDIR)/table/shops_strings.h: $(RCDDIR)/shops_strings.txt
	$(PYTHON) $(GEN_NAMES_PY) --header $(OBJDIR)/table/shops_strings.h \
			--code $(OBJDIR)/table/shops_strings.cpp \
			--prefix=SHOPS_ --base=STR_GENERIC_SHOP_START \
			$(RCDDIR)/shops_strings.txt

clean:
	for i in $(OBJFILES) $(DEPEND) $(OBJDIR)/table/strings.h $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done

ifndef NODEPS
include $(DEPEND)
endif

