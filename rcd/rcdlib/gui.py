# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
"""
Gui sprites handling.
"""
from rcdlib import spritegrid, blocks

CHECKBOX=96
RADIO_BUTTON=112

#: Name of the 9 sprites decorating a rectangle.
decorating_sprites = ['top-left', 'top-middle', 'top-right',
                      'left', 'middle', 'right',
                      'bottom-left', 'bottom-middle', 'bottom-right']

def add_checkbox(im, rcd, wid_type, coords):
    """
    Make a checkbox or radio button block.

    @param im: Image.
    @type  im: L{PIL.Image}

    @param rcd: RCD file.
    @type  rcd: L{RCD}

    @param wid_type: Widget type.
    @type  wid_type: C{int}

    @param coords: Coordinates of the sprites in the image.
    @type  coords: C{dict} of C{str} to (x1, y1, x2, y2)
    """
    imgs = {}
    for name in ['empty', 'pressed', 'filled', 'pressed-filled',
                 'shaded', 'shaded-filled']:
        x1, y1, x2, y2 = coords[name]
        if x1 > x2: x1, x2 = x2, x1
        if y1 > y2: y1, y2 = y2, y1

        imo = spritegrid.ImageObject(im, 0, 0, x1, y1, x2 - x1 + 1, y2 - y1 + 1)

        pxl_blk = imo.make_8PXL(skip_crop = False)
        imgs[name] = rcd.add_block(pxl_blk)

    imgs['widget_num'] = wid_type
    chk = blocks.block_factory.get_block('GCHK', 1)
    chk.set_values(imgs)
    rcd.add_block(chk)

# x1/x2 are both part of the sprite, so width is 1 longer
def _width(x1, y1, x2, y2):
    if x1 > x2: return x1 - x2 + 1
    return x2 - x1 + 1
# y1/y2 are both part of the sprite, so height is 1 longer
def _height(x1, y1, x2, y2):
    if y1 > y2: return y1 - y2 + 1
    return y2 - y1 + 1

def _sumwidth_seq(imgdata, seq):
    """
    Compute total width of the sequence.
    """
    b = sum(_width(*imgdata[s]) for s in seq)
    return b

def _sumheight_seq(imgdata, seq):
    """
    Compute total height of the sequence.
    """
    b = sum(_height(*imgdata[s]) for s in seq)
    return b

def _minwidth(imgdata):
    s = min([_sumwidth_seq(imgdata, ['top-left', 'top-middle', 'top-right']),
             _sumwidth_seq(imgdata, ['left', 'middle', 'right']),
             _sumwidth_seq(imgdata, ['bottom-left', 'bottom-middle', 'bottom-right'])])
    s = s - imgdata['left-border'] - imgdata['right-border']
    assert s > 0
    return s

def _minheight(imgdata):
    s = min([_sumheight_seq(imgdata, ['top-left', 'left', 'bottom-left']),
             _sumheight_seq(imgdata, ['top-middle', 'middle', 'bottom-middle']),
             _sumheight_seq(imgdata, ['top-right', 'right', 'bottom-right'])])
    s = s - imgdata['top-border'] - imgdata['bottom-border']
    assert s > 0
    return s

def _hor_stepsize(imgdata):
    sizes = [_width(*imgdata['top-middle']),
             _width(*imgdata['middle']),
             _width(*imgdata['bottom-middle'])]
    assert len(set(sizes)) == 1  # Otherwise a smart computation is needed, a kind of smallest common multiple for many values
    return sizes[0]

def _vert_stepsize(imgdata):
    sizes = [_height(*imgdata['left']),
             _height(*imgdata['middle']),
             _height(*imgdata['right'])]
    assert len(set(sizes)) == 1  # Otherwise a smart computation is needed, a kind of smallest common multiple for many values
    return sizes[0]

TITLEBAR = 32
BUTTON = 48
PRESSED_BUTTON = 49
ROUNDED_BUTTON = 52
PRESSED_ROUNDED_BUTTON = 53
FRAME = 64
PANEL = 68
INSET_FRAME = 80

def add_decoration(im, rcd, widget_num, imgdata):
    """
    Add rectangle decoration around a widget.

    @param im: Image
    @type  im: L{PIL.Image}

    @param rcd: RCD file.
    @type  rcd: L{blocks.RCD}

    @param widget_num: Widget number of the decoration.
    @type  widget_num: C{int}

    @param imgdata: Image data. The 9 rectangle sprites as coordinates, and
                    {top/left/right/bottom}-border width setting.
    @type  imgdata: C{dict} of C{str} to various.
    """
    imgs = {}
    for name in decorating_sprites:
        x1, y1, x2, y2 = imgdata[name]
        if x1 > x2: x1, x2 = x2, x1
        if y1 > y2: y1, y2 = y2, y1

        dx, dy = 0, 0
        if name in ['top-left', 'top-middle', 'top-right']:
            dy = -imgdata['top-border']
        if name in ['bottom-left', 'bottom-middle', 'bottom-right']:
            dy = -(y2-y1)+imgdata['bottom-border']
        if name in ['top-left', 'left', 'bottom-left']:
            dx = -imgdata['left-border']
        if name in ['top-right', 'right', 'bottom-right']:
            dx = -(x2-x1)+imgdata['right-border']

        imo = spritegrid.ImageObject(im, dx, dy, x1, y1, x2 - x1 + 1, y2 - y1 + 1)

        pxl_blk = imo.make_8PXL(skip_crop = False)
        imgs[name] = rcd.add_block(pxl_blk)

    imgs['border-width-top'] = imgdata['top-border']
    imgs['border-width-left'] = imgdata['left-border']
    imgs['border-width-right'] = imgdata['right-border']
    imgs['border-width-bottom'] = imgdata['bottom-border']
    imgs['widget_num'] = widget_num
    imgs['min-width-rect'] = _minwidth(imgdata)
    imgs['min-height-rect'] = _minheight(imgdata)
    imgs['hor-stepsize'] = _hor_stepsize(imgdata)
    imgs['vert-stepsize'] = _vert_stepsize(imgdata)

    deco = blocks.block_factory.get_block('GBOR', 1)
    deco.set_values(imgs)
    rcd.add_block(deco)

HOR_NORMAL_SCROLLBAR = 160
HOR_SHADED_SCROLLBAR = 161
VERT_NORMAL_SCROLLBAR = 176
VERT_SHADED_SCROLLBAR = 177

def add_scrollbar(im, rcd, widget_num, imgdata):
    """
    Add scrollbar sprites block.

    @param im: Image
    @type  im: L{PIL.Image}

    @param rcd: RCD file.
    @type  rcd: L{blocks.RCD}

    @param widget_num: Widget number of the decoration.
    @type  widget_num: C{int}

    @param imgdata: Image data. The 9 rectangle sprites as coordinates, and
                    {top/left/right/bottom}-border width setting.
    @type  imgdata: C{dict} of C{str} to various.
    """
    names = ['leftup', 'rightdown',
             'leftup-pressed', 'rightdown-pressed',
             'leftup-under', 'middle-under', 'rightdown-under',
             'leftup-select', 'middle-select', 'rightdown-select',
             'leftup-pressed-select', 'middle-pressed-select', 'rightdown-pressed-select']

    # No pressed variants available yet, re-use the unpressed ones.
    imgdata['leftup-pressed-select'] = imgdata['leftup-select']
    imgdata['middle-pressed-select'] = imgdata['middle-select']
    imgdata['rightdown-pressed-select'] = imgdata['rightdown-select']



    imgs = {}
    imos = {}
    for name in names:
        value = imgdata[name]
        if value is None:
            imgs[name] = 0
            continue

        x1, y1, x2, y2 = imgdata[name]
        if x1 > x2: x1, x2 = x2, x1
        if y1 > y2: y1, y2 = y2, y1

        imo = spritegrid.ImageObject(im, 0, 0, x1, y1, x2 - x1 + 1, y2 - y1 + 1)
        imos[name] = imo
        pxl_blk = imo.make_8PXL(skip_crop = False)
        imgs[name] = rcd.add_block(pxl_blk)

    scroll_len = [imos['leftup'], imos['rightdown'], imos['leftup-under'], imos['rightdown-under'], imos['middle-under']]
    select_len = [imos['leftup-select'], imos['middle-select'], imos['rightdown-select']]
    scroll_step = imos['middle-under']
    select_step = imos['middle-select']

    imgs['widget_num'] = widget_num
    if widget_num == HOR_SHADED_SCROLLBAR or widget_num == HOR_NORMAL_SCROLLBAR:
        imgs['minimal-length-scrollbar'] = sum(x.xsize for x in scroll_len)
        imgs['minimal-length-select'] = sum(x.xsize for x in select_len)
        imgs['stepsize-scrollbar'] = scroll_step.xsize
        imgs['stepsize-select'] = select_step.xsize
    else:
        imgs['minimal-length-scrollbar'] = sum(x.ysize for x in scroll_len)
        imgs['minimal-length-select'] = sum(x.ysize for x in select_len)
        imgs['stepsize-scrollbar'] = scroll_step.ysize
        imgs['stepsize-select'] = select_step.ysize

    scroll = blocks.block_factory.get_block('GSCL', 1)
    scroll.set_values(imgs)
    rcd.add_block(scroll)
