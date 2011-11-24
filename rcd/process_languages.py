#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
r"""
Process translations.

Load the text files, and produce table files. Also generate an index for the C++ compiler.

Format of the input file:
 - '#....'                Comment starts with '#' in the first column.
 - 'NAME     :text'       All colons should start in the same column.
 - '          more text'  Continuation line of the previous NAME.

Format of an output language file:
 - Header:
   4 bytes "RCDS" (RCD Strings)
   4 bytes 1 (version number header).
 - Text Strings:
   4 bytes "LTXT" (Language text).
   4 bytes 2 (version number LTXT block).
   4 bytes length (minus the first 12 bytes).
   2 bytes number of texts.
   8 bytes name of the language (\0 terminated).
   ? bytes all texts, each one \0 terminated.
"""
import os, sys, argparse, random, string
from rcdlib import output

TABLE_START = 100  # Start of string table.


class Language(object):
    """
    Class for holding the source language.

    @ivar texts: Mapping of names to strings.
    @type texts: C{dict} of C{str} to C{str}

    @ivar name: Name of the language.
    @type name: C{str}
    """
    def __init__(self, name):
        self.texts = {}
        self.name = name

    def store(self, name, text, uniq=True):
        """
        Store a translation.

        @param name: Name of the text (that is, the identifier).
        @type  name: C{str}

        @param text: Text of the translation.
        @type  text: C{str}

        @param uniq: Entry should be unique.
        @type  uniq: C{bool}
        """
        if uniq and name in self.texts:
            sys.stderr.write('process_languages, error: Name %r is not unique' % (name,))
            sys.exit(1)

        if not is_identifier(name):
            sys.stderr.write('process_languages, error: %r is not a valid name (only uppercase and _ are allowed)' % name)
            sys.exit(1)

        self.texts[name] = text

    def dump_stats(self):
        print "Language: %s" % self.name
        print "%d texts" % len(self.texts)

    def load(self, fname):
        """
        Load a file.
        """
        self.texts = {}

        fp = open(fname, 'r')
        colon = None
        name = None
        text = None
        for idx, line in enumerate(fp):
            line = line.rstrip()
            if line == '' or line[0] == '#':
                continue

            if line[0] == ' ':
                # Continuation line
                if colon is None or text is None:
                    sys.stderr.write('process_languages, error: Need a name:text line first.\n')
                    sys.exit(1)

                if not line.startswith(' ' * (colon + 1)):
                    sys.stderr.write('process_languages, error: Incorrect number of spaces.\n')
                    sys.exit(1)

                text = text + ' ' + line[colon + 1:]
                continue

            # 'name   :text'  line

            i = line.find(':')
            if i <= 0:
                sys.stderr.write('process_languages, warning: Line %d ignored\n' % (idx + 1,))
                continue

            if colon is None:
                colon = i
            else:
                if colon != i:
                    sys.stderr.write('process_languages, error: Colon is at the'
                                     'wrong column (expected %d, found %d)\n' % (colon, i))
                    sys.exit(1)

            if name is not None and text is not None:
                self.store(name, text, True)

            name = line[:colon].rstrip()
            text = line[colon + 1:]


        if name is not None and text is not None:
            self.store(name, text, True)

        fp.close()

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


class TextDefinition(object):
    """
    Class containing the definition of the texts.

    @ivar defs: List of pairs (name, default-text). Index value is the
                string number (+ STRING_START).
    @type defs: C{list} of C{tuple} (C{str}, C{str})
    """
    def __init__(self, lang):
        """
        Initializer.

        @param lang: Default language.
        @type  lang: L{Language}
        """
        self.defs = sorted(lang.texts.iteritems())

    def write_include(self, fname):
        """
        Write an include file with the string table definitions.
        """
        fp = open(fname, 'w')
        fp.write("// String table for FreeRCT.\n")
        fp.write("// Generated by 'process_languages', do not edit.\n")
        fp.write("\n")
        fp.write("#ifndef STRING_TABLE_H\n")
        fp.write("#define STRING_TABLE_H\n")
        fp.write("\n")

        fp.write("#define STRING_TABLE_START  %d\n" % (TABLE_START,))
        fp.write("#define STRING_TABLE_LENGTH %d\n" % (len(self.defs),))
        fp.write("\n")

        for idx, (name, text) in enumerate(self.defs):
            assert is_identifier(name)
            fp.write("#define %s %d\n" % (name, idx + TABLE_START))

        fp.write("\n")
        fp.write("#endif\n");
        fp.close()

    def write_language(self, lang, fname):
        """
        Write a language file.
        """
        # Compute lang-name (always 8 bytes long)
        langname = lang.name[:7]
        assert langname != ''
        while len(langname) < 8:
            langname = langname + chr(0)

        # Compute strings
        strings = []
        for name, default_text in self.defs:
            strings.append(lang.texts.get(name, default_text) + chr(0))

        length = 2 + 8 + sum(len(s) for s in strings)

        # Write output
        out = output.FileOutput()
        out.set_file(fname)

        # Header
        out.store_text("RCDS") # Header name
        out.uint32(1)          # Header version number

        # Block
        out.store_text("LTXT") # Block name
        out.uint32(2)          # Block version number
        out.uint32(length)     # Length of the block data

        # Block data
        out.uint16(len(self.defs)) # Number of strings
        assert len(langname) == 8 and langname[-1] == chr(0)
        out.store_text(langname)   # Output language name

        for s in strings:
            out.store_text(s)

        out.close()


parser = argparse.ArgumentParser(description='Process translations.')
parser.add_argument('--directory')
parser.add_argument('--output-include')
parser.add_argument('--output-language')
args = parser.parse_args()

english = args.directory
if english is None: english = "."
english = os.path.join(english, 'english.txt')


if not os.path.isfile(english):
    sys.stderr.write("Cannot find the language files (%r is not available)\n", english)
    sys.exit(1)

l = Language('en_uk')
l.load(english)
#l.dump_stats()

td = TextDefinition(l)

if args.output_include is not None:
    td.write_include(args.output_include)

if args.output_language is not None:
    td.write_language(l, args.output_language)
