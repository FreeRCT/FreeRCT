#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from rcdlib import spritegrid, path
import argparse

parser = argparse.ArgumentParser(description='Process a path sprites image.')
parser.add_argument(dest='image_file', metavar='img-file', type=str,
                   help='Image file contains the path sprites')
parser.add_argument('--output', dest='output', action='store',
                   metavar='rcd-file', help='Output RCD file')
parser.add_argument('--verbose', action='store_true', default=False,
                    help="Output size and offset of the sprites.")
args = parser.parse_args()

if args.output is None:
    if args.image_file.lower().endswidth('.png'):
        out_name = args.image_file[:-4] + '.rcd'
    else:
        out_name = args.image_file + '.rcd'
else:
    out_name = args.output

# Sprites as laid out in the source image.
std_layout = [['empty',               'empty',               'empty',               'empty'],
              ['ne',                  'se',                  'sw',                  'nw'],
              ['ne_se',               'se_sw',               'nw_sw',               'ne_nw'],
              ['ne_sw',               'nw_se',               'ne_sw',               'nw_se'],
              ['ne_se_sw',            'nw_se_sw',            'ne_nw_sw',            'ne_nw_se'],
              ['ne_nw_se_sw',         'ne_nw_se_sw',         'ne_nw_se_sw',         'ne_nw_se_sw'],
              ['ne_se_e',             'se_sw_s',             'nw_sw_w',             'ne_nw_n'],
              ['ne_se_sw_e',          'nw_se_sw_s',          'ne_nw_sw_w',          'ne_nw_se_n'],
              ['ne_se_sw_s',          'nw_se_sw_w',          'ne_nw_sw_n',          'ne_nw_se_e'],
              ['ne_se_sw_e_s',        'nw_se_sw_s_w',        'ne_nw_sw_n_w',        'ne_nw_se_n_e'],
              ['ne_nw_se_sw_n_s_w',   'ne_nw_se_sw_n_e_w',   'ne_nw_se_sw_n_e_s',   'ne_nw_se_sw_e_s_w'],
              ['ne_nw_se_sw_n_w',     'ne_nw_se_sw_n_e',     'ne_nw_se_sw_e_s',     'ne_nw_se_sw_s_w'],
              ['ne_nw_se_sw_n_s',     'ne_nw_se_sw_e_w',     'ne_nw_se_sw_n_s',     'ne_nw_se_sw_e_w'],
              ['ne_nw_se_sw_n',       'ne_nw_se_sw_e',       'ne_nw_se_sw_s',       'ne_nw_se_sw_w'],
              ['ne_nw_se_sw',         'ne_nw_se_sw',         'ne_nw_se_sw',         'ne_nw_se_sw'],
              ['ne_nw_se_sw_n_e_s_w', 'ne_nw_se_sw_n_e_s_w', 'ne_nw_se_sw_n_e_s_w', 'ne_nw_se_sw_n_e_s_w'],
              ['ramp_sw',             'ramp_nw',             'ramp_ne',             'ramp_se']]


images = spritegrid.split_spritegrid(args.image_file, -32, -33, 64, 64, std_layout)
path.write_pathRCD(images, path.CONCRETE, 64, 16, args.verbose, out_name)

