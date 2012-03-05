#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
from PIL import Image
from rcdlib import gui, blocks

normalized_keys = {'left'          : 'leftup',
                   'left-pressed'  : 'leftup-pressed',
                   'right'         : 'rightdown',
                   'right-pressed' : 'rightdown-pressed',
                   'left-under'    : 'leftup-under',
                   'middle-under'  : 'middle-under',
                   'right-under'   : 'rightdown-under',
                   'left-select'   : 'leftup-select',
                   'middle-select' : 'middle-select',
                   'right-select'  : 'rightdown-select',
                   'down'          : 'rightdown',
                   'down-pressed'  : 'rightdown-pressed',
                   'up'            : 'leftup',
                   'up-pressed'    : 'leftup-pressed',
                   'up-under'      : 'leftup-under',
                   'middle-under'  : 'middle-under',
                   'down-under'    : 'rightdown-under',
                   'up-select'     : 'leftup-select',
                   'middle-select' : 'middle-select',
                   'down-select'   : 'rightdown-select', }

def normalize_scroll(bar):
    """
    Normalize the name of the scrollbar data.
    """
    norm_bar = {}
    for k, v in bar.iteritems():
        norm_bar[normalized_keys[k]] = v
    return norm_bar


checkbox = {'empty'         : ( 0, 0,  15, 15),
            'pressed'       : (17, 0,  32, 15),
            'filled'        : (34, 0,  49, 15), # rtl ?
            'pressed-filled': (51, 0,  66, 15),
            'shaded'        : (68, 0,  83, 15),
            'shaded-filled' : (85, 0, 100, 15) }
radio = {'empty'         : ( 0, 17,  15, 32),
         'pressed'       : (17, 17,  32, 32),
         'filled'        : (34, 17,  49, 32),
         'pressed-filled': (51, 17,  66, 32),
         'shaded'        : (68, 17,  83, 32),
         'shaded-filled' : (85, 17, 100, 32) }

# Skipped
dropdown = {'left'   : (102, 0, 109, 15),
            'middle' : (111, 0, 111, 15),
            'button' : (113, 0, 129, 15) } # rtl ?

# Skipped
dropdown_shaded = {'left'   : (131, 0, 138, 15),
                   'middle' : (140, 0, 140, 15),
                   'button' : (142, 0, 158, 15) } # rtl ?

# Skipped
dropdown_pressed = {'left'   : (102, 17, 109, 32),
                    'middle' : (111, 17, 111, 32),
                    'button' : (113, 17, 129, 32) } # rtl ?

# x-start, y-start, top-left, bottom-right
titlebar = {'top-left'     : ( 0, 34,  7, 41),
            'top-middle'   : ( 9, 34,  9, 41),
            'top-right'    : (11, 34, 18, 41),
            'left'         : ( 0, 43,  7, 43),
            'middle'       : ( 9, 43,  9, 43),
            'right'        : (11, 43, 18, 43),
            'bottom-left'  : ( 0, 45,  7, 52),
            'bottom-middle': ( 9, 45,  9, 52),
            'bottom-right' : (11, 45, 18, 52),
            'top-border'   : 2,
            'left-border'  : 2,
            'bottom-border': 2,
            'right-border' : 2 }

button_up = {'top-left'     : (20, 34, 27, 41),
             'top-middle'   : (29, 34, 29, 41),
             'top-right'    : (31, 34, 38, 41),
             'left'         : (20, 43, 27, 43),
             'middle'       : (29, 43, 29, 43),
             'right'        : (31, 43, 38, 43),
             'bottom-left'  : (20, 45, 27, 52),
             'bottom-middle': (29, 45, 29, 52),
             'bottom-right' : (31, 45, 38, 52),
             'top-border'   : 1,
             'left-border'  : 1,
             'bottom-border': 1,
             'right-border' : 1 }

button_down = {'top-left'     : (40, 34, 47, 41),
               'top-middle'   : (49, 34, 49, 41),
               'top-right'    : (51, 34, 58, 41),
               'left'         : (40, 43, 47, 43),
               'middle'       : (49, 43, 49, 43),
               'right'        : (51, 43, 58, 43),
               'bottom-left'  : (40, 45, 47, 52),
               'bottom-middle': (49, 45, 49, 52),
               'bottom-right' : (51, 45, 58, 52),
               'top-border'   : 1,
               'left-border'  : 1,
               'bottom-border': 1,
               'right-border' : 1 }

frame = {'top-left'     : (60, 34, 67, 41),
         'top-middle'   : (69, 34, 69, 41),
         'top-right'    : (71, 34, 78, 41),
         'left'         : (60, 43, 67, 43),
         'middle'       : (69, 43, 69, 43),
         'right'        : (71, 43, 78, 43),
         'bottom-left'  : (60, 45, 67, 52),
         'bottom-middle': (69, 45, 69, 52),
         'bottom-right' : (71, 45, 78, 52),
         'top-border'   : 2,
         'left-border'  : 2,
         'bottom-border': 2,
         'right-border' : 2 }

inset_frame = {'top-left'     : (160, 34, 167, 41),
               'top-middle'   : (169, 34, 169, 41),
               'top-right'    : (171, 34, 178, 41),
               'left'         : (160, 43, 167, 43),
               'middle'       : (169, 43, 169, 43),
               'right'        : (171, 43, 178, 43),
               'bottom-left'  : (160, 45, 167, 52),
               'bottom-middle': (169, 45, 169, 52),
               'bottom-right' : (171, 45, 178, 52),
               'top-border'   : 1,
               'left-border'  : 1,
               'bottom-border': 1,
               'right-border' : 1 }

rounded_button_up = {'top-left'     : (60, 34, 67, 41),
                     'top-middle'   : (69, 34, 69, 41),
                     'top-right'    : (71, 34, 78, 41),
                     'left'         : (60, 43, 67, 43),
                     'middle'       : (69, 43, 69, 43),
                     'right'        : (71, 43, 78, 43),
                     'bottom-left'  : (60, 45, 67, 52),
                     'bottom-middle': (69, 45, 69, 52),
                     'bottom-right' : (71, 45, 78, 52),
                     'top-border'   : 3,
                     'left-border'  : 3,
                     'bottom-border': 3,
                     'right-border' : 3 }

rounded_button_down = {'top-left'     : (100, 34, 107, 41),
                       'top-middle'   : (109, 34, 109, 41),
                       'top-right'    : (111, 34, 118, 41),
                       'left'         : (100, 43, 107, 43),
                       'middle'       : (109, 43, 109, 43),
                       'right'        : (111, 43, 118, 43),
                       'bottom-left'  : (100, 45, 107, 52),
                       'bottom-middle': (109, 45, 109, 52),
                       'bottom-right' : (111, 45, 118, 52),
                       'top-border'   : 3,
                       'left-border'  : 3,
                       'bottom-border': 3,
                       'right-border' : 3 }

# Skipped for now
deep_button_up      = {'top-left'     : (120, 34, 127, 41),
                       'top-middle'   : (129, 34, 129, 41),
                       'top-right'    : (131, 34, 138, 41),
                       'left'         : (120, 43, 127, 43),
                       'middle'       : (129, 43, 129, 43),
                       'right'        : (131, 43, 138, 43),
                       'bottom-left'  : (120, 45, 127, 52),
                       'bottom-middle': (129, 45, 129, 52),
                       'bottom-right' : (131, 45, 138, 52),
                       'top-border'   : 3,
                       'left-border'  : 3,
                       'bottom-border': 3,
                       'right-border' : 3 }

# Skipped for now
deep_button_down    = {'top-left'     : (140, 34, 147, 41),
                       'top-middle'   : (149, 34, 149, 41),
                       'top-right'    : (131, 34, 138, 41),
                       'left'         : (140, 43, 147, 43),
                       'middle'       : (149, 43, 149, 43),
                       'right'        : (131, 43, 138, 43),
                       'bottom-left'  : (140, 45, 147, 52),
                       'bottom-middle': (149, 45, 149, 52),
                       'bottom-right' : (131, 45, 138, 52),
                       'top-border'   : 3,
                       'left-border'  : 3,
                       'bottom-border': 3,
                       'right-border' : 3 }

slider_underground = {'left'   : ( 0, 54,  7, 69),
                      'middle' : ( 9, 54,  9, 69),
                      'right'  : (11, 54, 18, 69)}
sliderbar_up   = (20, 54, 35, 69)
sliderbar_down = (37, 54, 52, 69)

hor_scrollbar        = {'left'               : (  1, 92,  16, 107),
                        'left-pressed'       : ( 69, 92,  84, 107),
                        'right'              : ( 35, 92,  50, 107),
                        'right-pressed'      : ( 86, 92, 101, 107),
                        'left-under'         : (103, 92, 110, 107),
                        'middle-under'       : (112, 92, 113, 107),
                        'right-under'        : (115, 92, 122, 107),
                        'left-select'        : (145, 92, 152, 107),
                        'middle-select'      : (154, 92, 156, 107),
                        'right-select'       : (158, 92, 164, 107) }

shaded_hor_scrollbar = {'left'               : ( 18, 92,  33, 107),
                        'left-pressed'       : None,
                        'right'              : ( 52, 92,  67, 107),
                        'right-pressed'      : None,
                        'left-under'         : (124, 92, 131, 107),
                        'middle-under'       : (133, 92, 134, 107),
                        'right-under'        : (136, 92, 143, 107),
                        'left-select'        : (145, 92, 152, 107),  # Should be shaded
                        'middle-select'      : (154, 92, 156, 107),  # Should be shaded
                        'right-select'       : (158, 92, 164, 107) } # Should be shaded

ver_scrollbar =        {'down'               : (  1, 71,  16, 86),
                        'down-pressed'       : ( 69, 71,  84, 86),
                        'up'                 : ( 35, 71,  50, 86),
                        'up-pressed'         : ( 86, 71, 101, 86),
                        'up-under'           : (103, 71, 118, 78),
                        'middle-under'       : (103, 80, 118, 81),
                        'down-under'         : (103, 83, 118, 90),
                        'up-select'          : (137, 71, 152, 78),
                        'middle-select'      : (137, 80, 152, 81),
                        'down-select'        : (137, 83, 152, 90) }

shaded_ver_scrollbar = {'down'               : ( 18, 71,  33, 86),
                        'down-pressed'       : None,
                        'up'                 : ( 52, 71,  67, 86),
                        'up-pressed'         : None,
                        'up-under'           : (120, 71, 135, 78),
                        'middle-under'       : (120, 80, 135, 81),
                        'down-under'         : (120, 83, 135, 90),
                        'up-select'          : (137, 71, 152, 78),  # Should be shaded
                        'middle-select'      : (137, 80, 152, 81),  # Should be shaded
                        'down-select'        : (137, 83, 152, 90) } # Should be shaded


fname = '../sprites_src/gui/GUIEles.png'
dest_fname = 'gui.rcd'

im = Image.open(fname)
assert im.mode == 'P' # 'P' means 8bpp indexed


file_data = blocks.RCD()

gui.add_checkbox(im, file_data, gui.CHECKBOX, checkbox)
gui.add_checkbox(im, file_data, gui.RADIO_BUTTON, radio)

gui.add_decoration(im, file_data, gui.TITLEBAR, titlebar)
gui.add_decoration(im, file_data, gui.BUTTON, button_up) # First use of button_up
gui.add_decoration(im, file_data, gui.PRESSED_BUTTON, button_down)
gui.add_decoration(im, file_data, gui.FRAME, frame)
gui.add_decoration(im, file_data, gui.PANEL, button_up) # Second use of button_up
gui.add_decoration(im, file_data, gui.INSET_FRAME, inset_frame)
gui.add_decoration(im, file_data, gui.ROUNDED_BUTTON, rounded_button_up)
gui.add_decoration(im, file_data, gui.PRESSED_ROUNDED_BUTTON, rounded_button_down)

bar = normalize_scroll(hor_scrollbar)
gui.add_scrollbar(im, file_data, gui.HOR_NORMAL_SCROLLBAR, bar)
bar = normalize_scroll(shaded_hor_scrollbar)
gui.add_scrollbar(im, file_data, gui.HOR_SHADED_SCROLLBAR, bar)
bar = normalize_scroll(ver_scrollbar)
gui.add_scrollbar(im, file_data, gui.VERT_NORMAL_SCROLLBAR, bar)
bar = normalize_scroll(shaded_ver_scrollbar)
gui.add_scrollbar(im, file_data, gui.VERT_SHADED_SCROLLBAR, bar)

file_data.to_file(dest_fname)
