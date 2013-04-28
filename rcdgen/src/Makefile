# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: clean

INCLUDES=
LIBS=-lpng
CXXFLAGS=-Wall -g
CXX=g++
BISON=bison
FLEX=flex
DEPEND=deps
TARGET=rcdgen
CXXFILES=ast.cpp check_data.cpp getoptdata.cpp image.cpp fileio.cpp main.cpp \
	nodes.cpp parser.cpp scanner.cpp scanner_funcs.cpp
OBJFILES=$(CXXFILES:.cpp=.o)

$(TARGET): $(DEPEND) $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $(INCLUDES) $<

$(DEPEND): $(CXXFILES)
	for f in $(CXXFILES);\
	do $(CXX) -MM $(CXXFLAGS) $(INCLUDES) $$f;\
	done > $(DEPEND)

clean:
	for i in $(OBJFILES) $(DEPEND) $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done

tokens.h parser.cpp: parser.y
	$(BISON) --defines=tokens.h --output=parser.cpp parser.y

scanner.cpp scanner.h: scanner.l
	$(FLEX) --outfile=scanner.cpp scanner.l

ifndef NODEPS
include $(DEPEND)
endif
