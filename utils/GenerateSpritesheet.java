/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

import java.awt.image.*;
import java.io.*;
import javax.imageio.ImageIO;

/**
 * This program provides an easy way to generate spritesheets for ride animations from individual (blender) images.
 * Let's assume you've made an animation with `f` frames for a ride with a size of `x` × `y` tiles.
 * The base images, all of which are of size `w` × `h`, are located in `src_dir`.
 * The images are called "PREFIX0000.png" through "PREFIX9999.png", where `PREFIX` may be different for each of the four rotations
 * (let's call the prefixes `se`,`ne`,`nw`,`sw` in this order). The number of the
 * animation's starting frame is `idOff_<p>` for each of the four prefixes <p>.
 * To generate the spritesheet and save it as `output_file`, run
 *    java GenerateSpritesheet src_dir output_file idOff_se se idOff_ne ne idOff_nw nw idOff_sw sw f x y w h true
 * The last, optional parameter can be set to `false` to disable intelligent calculation of frame indices
 * (useful when (ab)using this tool to generate spritesheets for a different purpose than the one stated above).
 * Before running the program the first time you need to run
 *    javac GenerateSpritesheet.java
 */

public class GenerateSpritesheet {
	public static void main(String[] args) throws Exception {
		System.out.println("Usage: java GenerateSpritesheet src_dir output_file idOff_se se idOff_ne ne idOff_nw nw idOff_sw sw nr_frames x y w h [cleverindex=true]");
		int arg_index = 0;
		final String src = args[arg_index++];
		final String result = args[arg_index++];
		final String[] prefixes = new String[4];
		final int[] idOff = new int[4];
		for (int i = 0; i < prefixes.length; ++i) {
			idOff[i] = Integer.valueOf(args[arg_index++]);
			prefixes[i] = args[arg_index++];
		}
		final int frames = Integer.valueOf(args[arg_index++]);
		final int sprites_x = Integer.valueOf(args[arg_index++]);
		final int sprites_y = Integer.valueOf(args[arg_index++]);
		final int sprite_w = Integer.valueOf(args[arg_index++]);
		final int sprite_h = Integer.valueOf(args[arg_index++]);
		final boolean cleverindex = (arg_index >= args.length) || Boolean.parseBoolean(args[arg_index]);

		BufferedImage out = new BufferedImage(frames * sprite_w * sprites_x, sprite_h * sprites_y * 4, BufferedImage.TYPE_INT_ARGB);
		for (int f = 0; f < frames; ++f)
		for (int v = 0; v < 4; ++v)
		for (int x = 0; x < sprites_x; ++x)
		for (int y = 0; y < sprites_y; ++y)
		{
			int tileIndex = Integer.MIN_VALUE;
			if (cleverindex) {
				switch (v) {
					case 0:
					case 1:
						tileIndex = x * sprites_y + sprites_y - y - 1;
						break;
					case 2:
					case 3:
						tileIndex = y + sprites_y * (sprites_x - x - 1);
						break;
				}
			} else {
				tileIndex = y * sprites_x + x;
			}
			final int imageID = idOff[v] + f + frames * tileIndex;
			String filename = "" + imageID;
			while (filename.length() < 4) filename = "0" + filename;
			filename = prefixes[v] + filename + ".png";
			System.out.println(String.format("[x=%d y=%d v=%d f=%d idx=%d] Loading %s %s", x, y, v, f, tileIndex, src, filename));
			BufferedImage in = ImageIO.read(new File(src, filename));
			if (in.getWidth() != sprite_w || in.getHeight() != sprite_h) {
				System.out.println("Wrong height");
				System.exit(1);
			}
			final int xpos = f * sprite_w * sprites_x + x * sprite_w;
			final int ypos = v * sprite_h * sprites_y + y * sprite_h;
			for (int i = 0; i < sprite_w; ++i)
			for (int j = 0; j < sprite_h; ++j)
			out.setRGB(xpos + i, ypos + j, in.getRGB(i, j));
		}
		ImageIO.write(out, "png", new File(result));
		System.out.println("Success!");
	}
}
