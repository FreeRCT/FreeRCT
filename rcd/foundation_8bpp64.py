#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import foundations

fname = '../sprites/foundationtemplate8bpp64_masked.png'

images = foundations.split_image(fname, -32, -33, 64, 64)
foundations.write_foundationRCD(images, 64, 16, foundations.GROUND, 'foundation_8bpp64.rcd')

