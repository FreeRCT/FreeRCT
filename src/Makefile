# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: run clean rcd all

OBJDIR=objects
RCDDIR=../graphics/rcd
RCDGEN=rcdgen/rcdgen

CXX=g++
CXXFLAGS=-Wall -g
INCLUDES=-I$(OBJDIR) -I/usr/include/SDL
LIBS=-lSDL -lSDL_ttf

CXXFILES=bottom_toolbar_gui.cpp coaster.cpp coaster_gui.cpp config_reader.cpp dates.cpp \
	error_gui.cpp fileio.cpp finances.cpp finances_gui.cpp \
	gamelevel.cpp getoptdata.cpp gui_graphics.cpp language.cpp main.cpp map.cpp math_func.cpp \
	palette.cpp path_build.cpp path.cpp path_gui.cpp \
	people.cpp person.cpp person_gui.cpp \
	random.cpp ride_gui.cpp ride_type.cpp \
	select_mode.cpp setting_gui.cpp shop_gui.cpp sprite_store.cpp string_func.cpp \
	terraform.cpp terraform_gui.cpp tile_func.cpp toolbar_gui.cpp track_piece.cpp \
	video.cpp viewport.cpp \
	widget.cpp window.cpp

OBJFILES=$(addprefix $(OBJDIR)/,$(CXXFILES:.cpp=.o))
DEPEND=$(OBJDIR)/depend-freerct.txt
TARGET=freerct

all: rcd $(TARGET)
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

run: all
	./$(TARGET)

gdbrun: all
	gdb $(TARGET)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

STRINGDIR=$(OBJDIR)/table
STRINGFILES=$(STRINGDIR)/gui_strings.h $(STRINGDIR)/shops_strings.h $(STRINGDIR)/coasters_strings.h

$(DEPEND): $(STRINGFILES) $(CXXFILES)
	for f in $(CXXFILES);\
	do $(CXX) -MM $(CXXFLAGS) $(INCLUDES) $$f | sed -e "s@^[^ ]*\\.o:@$(OBJDIR)/&@";\
	done > $(DEPEND)

$(OBJDIR)/table/gui_strings.h: $(RCDGEN)
	$(RCDGEN) --header $(OBJDIR)/table/gui_strings.h \
			--code $(OBJDIR)/table/gui_strings.cpp \
			--prefix GUI --base STR_GUI_START

$(OBJDIR)/table/shops_strings.h: $(RCDGEN)
	$(RCDGEN) --header $(OBJDIR)/table/shops_strings.h \
			--code $(OBJDIR)/table/shops_strings.cpp \
			--prefix SHOPS --base STR_GENERIC_SHOP_START

$(OBJDIR)/table/coasters_strings.h: $(RCDGEN)
	$(RCDGEN) --header $(OBJDIR)/table/coasters_strings.h \
			--code $(OBJDIR)/table/coasters_strings.cpp \
			--prefix COASTERS --base STR_GENERIC_COASTER_START

GENSTRINGFILES=$(OBJDIR)/table/coasters_strings.cpp $(OBJDIR)/table/shops_strings.cpp \
	$(OBJDIR)/table/gui_strings.cpp
clean:
	for i in $(OBJFILES) $(STRINGFILES) $(DEPEND) $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done

ifndef NODEPS
-include $(DEPEND)
endif
