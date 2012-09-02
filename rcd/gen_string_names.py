#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
"""
Load the GUI string names, and generate C++ code for accessing them as
STR_.... constants.
"""
import argparse, os, sys, string
from rcdlib import datatypes

def is_identifier(text):
    """
    Upper-case and _ are allowed only.
    """
    if len(text) < 4: return False # Arbitrary lower limit on the length
    if text[0] not in string.ascii_uppercase: return False
    if text[-1] not in string.ascii_uppercase: return False
    for t in text:
        if t != '_' and t not in string.ascii_uppercase: return False
    return True

def write_header(words, fname, base, prefix):
    fp = open(fname, 'w')
    fp.write('// GUI string table for FreeRCT\n')
    fp.write('// Automagically generated, do not edit\n')
    fp.write('\n')
    fp.write('#ifndef ' + prefix + 'STRING_TABLE_H\n')
    fp.write('#define ' + prefix + 'STRING_TABLE_H\n')
    fp.write('\n')

    fp.write('/** Strings table */\n')
    fp.write('enum ' + prefix.capitalize() + 'Strings {\n')

    for w in words:
        name = prefix + w.encode('ascii')
        if base is not None:
            fp.write('\t%s = %s,\n' % (name, base))
            base = None
        else:
            fp.write('\t%s,\n' % name)
    fp.write('\n')
    fp.write('\t' + prefix + 'STRING_TABLE_END,\n');
    fp.write('};\n')
    fp.write('\n')
    fp.write('#endif\n')
    fp.close()

def write_code(words, fname):
    fp = open(fname, 'w')
    fp.write('// GUI string names for FreeRCT\n')
    fp.write('// Automagically generated, do not edit\n')
    fp.write('\n')
    fp.write('/** String names array */\n')
    fp.write('const char *_' + prefix.lower() + 'strings_table[] = {\n')
    for w in words:
        name = w.encode('ascii')
        fp.write('\t"%s",\n' % name)
    fp.write('\tNULL,\n')
    fp.write('};\n')
    fp.write('\n')
    fp.close()

parser = argparse.ArgumentParser(description='Process translations.')
parser.add_argument('string_names', metavar='FILE', help='file containing string names')
parser.add_argument('--header',     metavar='FILE', help='Output an enum with the string names in the code')
parser.add_argument('--code',       metavar='FILE', help='Output C++ code with an array of string names')
parser.add_argument('--base',       metavar='BASE', help='Start-offset of the table')
parser.add_argument('--prefix',     metavar='PREFIX', help='Prefix of the string names in the header')
args = parser.parse_args()

base = args.base
if base is None: base = '0'
prefix = args.prefix
if prefix is None: prefix = 'GUI_'

if not os.path.isfile(args.string_names):
    sys.stderr.write("Cannot find the string-names files (%r is not available)\n", args.string_names)
    sys.exit(1)

words = datatypes.read_string_names(args.string_names)
words = [w for w in words if is_identifier(w)]

if args.header is not None: write_header(words, args.header, base, prefix)
if args.code is not None: write_code(words, args.code)

sys.exit(0)
