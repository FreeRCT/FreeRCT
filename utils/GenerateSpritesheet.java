/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

import java.awt.image.*;
import java.io.*;
import java.util.*;
import javax.imageio.ImageIO;

/**
 * This program provides an easy way to generate spritesheets for ride animations from individual (blender) images.
 * Let's assume you've made an animation with `f` frames for a ride with a size of `x` × `y` tiles.
 * The base images, all of which are of size `w` × `h`, are located in `src_dir`.
 * The images are called "PREFIX0000.png" through "PREFIX9999.png", where `PREFIX` may be different for each of the four rotations
 * (let's call the prefixes `se`,`ne`,`nw`,`sw` in this order). The number of the
 * animation's starting frame is `idOff_<p>` for each of the four prefixes <p>.
 *
 * To generate the spritesheet and save it as `output_file`, run
 *    java GenerateSpritesheet --src_dir src_dir --output_file output_file --se idOff_se se --ne idOff_ne ne --nw idOff_nw nw --sw idOff_sw sw \
 *                             --frames f --x_sprites x --y_sprites y --width w --height h --indexer TIMA
 *
 * The last parameter declares which indexing algorithm to use. The indexing algorithm decides at which coordinates
 * in the resulting spritesheet each input image should be located. See below for available indexing algorithms.
 *
 * Before running the program the first time you need to run
 *    javac GenerateSpritesheet.java
 */

public class GenerateSpritesheet {
	public static enum IndexingAlgorithm {
		/** Arranges the individual images in the way expected by the TIMA and FSET node generators. */
		TIMA,
		/** Just put one sprite after the other, without considering rotations or other fancy processing. */
		PLAIN,
	}

	public static void main(String[] args) throws Exception {
		System.out.println("Usage: java GenerateSpritesheet ARGS...");
		System.out.println("Args:");
		System.out.println("  -s --src_dir DIR         Directory for input files.");
		System.out.println("  -o --output_file FILE    Output file path and name.");
		System.out.println("     --se OFF PREFIX       Offset and prefix for the south-east views.");
		System.out.println("     --sw OFF PREFIX       Offset and prefix for the south-west views.");
		System.out.println("     --nw OFF PREFIX       Offset and prefix for the north-west views.");
		System.out.println("     --ne OFF PREFIX       Offset and prefix for the north-east views.");
		System.out.println("  -f --frames N            Number of animation frames to include in the output.");
		System.out.println("  -p --padding N           Number of animation frames including skipped frames (defaults to the value for -f).");
		System.out.println("  -x --x_sprites N         Number of sprites in X direction.");
		System.out.println("  -y --y_sprites N         Number of sprites in Y direction.");
		System.out.println("  -w --width W             Frame width in pixels.");
		System.out.println("  -h --height H            Frame height in pixels.");
		System.out.println("  -i --indexer I           Indexing algorithm:");
		System.out.println("                             TIMA    Generates a layout for use in FSET and TIMA blocks.");
		System.out.println("                             PLAIN   Just puts one sprite after the other.");
		System.out.println("  -c --code FILE           Emit rcdgen code and save it in the given file.");
		System.out.println();

		String src = null;
		File result = null;
		final String[] prefixes = new String[] { null, null, null, null };
		final Integer[] idOff = new Integer[] { null, null, null, null };
		Integer sprites_x = null;
		Integer sprites_y = null;
		Integer sprite_w = null;
		Integer sprite_h = null;
		Integer frames_to_render = null;
		Integer frames_with_skip = null;
		IndexingAlgorithm indexer = null;
		File emitCode = null;

		for (int i = 0; i < args.length; ++i) {
			switch (args[i].toLowerCase()) {
				case "-s":
				case "--src_dir":
					src = args[++i];
					break;
				case "-o":
				case "--output_file":
					result = new File(args[++i]);
					break;
				case "-c":
				case "--code":
					emitCode = new File(args[++i]);
					break;
				case "-i":
				case "--indexer":
					indexer = IndexingAlgorithm.valueOf(args[++i]);
					break;
				case "-x":
				case "--x_sprites":
					sprites_x = Integer.valueOf(args[++i]);
					break;
				case "-y":
				case "--y_sprites":
					sprites_y = Integer.valueOf(args[++i]);
					break;
				case "-w":
				case "--width":
					sprite_w = Integer.valueOf(args[++i]);
					if (sprite_w != 64) throw new IllegalArgumentException("Width should be 64");
					break;
				case "-h":
				case "--height":
					sprite_h = Integer.valueOf(args[++i]);
					break;
				case "-f":
				case "--frames":
					frames_to_render = Integer.valueOf(args[++i]);
					break;
				case "-p":
				case "--padding":
					frames_with_skip = Integer.valueOf(args[++i]);
					break;
				case "--se":
					idOff[0] = Integer.valueOf(args[++i]);
					prefixes[0] = args[++i];
					break;
				case "--ne":
					idOff[1] = Integer.valueOf(args[++i]);
					prefixes[1] = args[++i];
					break;
				case "--nw":
					idOff[2] = Integer.valueOf(args[++i]);
					prefixes[2] = args[++i];
					break;
				case "--sw":
					idOff[3] = Integer.valueOf(args[++i]);
					prefixes[3] = args[++i];
					break;
				default:
					System.out.println("Invalid argument: " + args[i]);
					System.exit(1);
			}
		}

		if (src == null) throw new IllegalArgumentException("Missing argument -s");
		if (result == null) throw new IllegalArgumentException("Missing argument -o");
		if (sprites_x == null) throw new IllegalArgumentException("Missing argument -x");
		if (sprites_y == null) throw new IllegalArgumentException("Missing argument -y");
		if (sprite_w == null) throw new IllegalArgumentException("Missing argument -w");
		if (sprite_h == null) throw new IllegalArgumentException("Missing argument -h");
		if (frames_to_render == null) throw new IllegalArgumentException("Missing argument -f");
		if (indexer == null) throw new IllegalArgumentException("Missing argument -i");
		for (String s : prefixes) if (s == null) throw new IllegalArgumentException("Missing prefix argument");
		for (Integer i : idOff) if (i == null) throw new IllegalArgumentException("Missing offset argument");
		if (frames_with_skip == null) frames_with_skip = frames_to_render;

		BufferedImage out = new BufferedImage(frames_to_render * sprite_w * sprites_x, sprite_h * sprites_y * 4, BufferedImage.TYPE_INT_ARGB);
		for (int f = 0; f < frames_to_render; ++f)
		for (int v = 0; v < 4; ++v)
		for (int x = 0; x < sprites_x; ++x)
		for (int y = 0; y < sprites_y; ++y)
		{
			int tileIndex = Integer.MIN_VALUE;
			switch (indexer) {
				case TIMA:
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
					break;
				case PLAIN:
					tileIndex = y * sprites_x + x;
					break;
				default:
					System.out.println("Invalid indexing algorithm " + indexer);
					System.exit(1);
			}
			final int imageID = idOff[v] + f + frames_with_skip * tileIndex;
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
		ImageIO.write(out, "png", result);

		if (emitCode != null) {
			System.out.println("Generating code ...");
			ArrayList<String> paths = new ArrayList<>();
			for (File f = result; !f.getName().equals("sprites"); f = f.getParentFile()) paths.add(0, f.getName());
			String out_path = "../sprites";
			for (String f : paths) out_path += "/" + f;
			String mask_path = new File(out_path).getParent() + "/" + result.getName().substring(0, 2) + "_mask" + result.getName().substring(2);

			PrintWriter write = new PrintWriter(emitCode);
			write.println("/* Automatically generated by GenerateSpritesheet. Editing this file by hand is not recommended. */");
			for (int f = 0; f < frames_to_render; ++f) {
				write.println("frame_" + f + ": FSET {");
					write.println("\twidth_x: " + sprites_x + ";");
					write.println("\twidth_y: " + sprites_y + ";");
					write.println("\ttile_width: 64;");
					write.println("\t(se_{vert(0.." + (sprites_x - 1) + ")}_{hor(0.." + (sprites_y - 1) + ")}): sheet {");
						write.println("\t\tx_base: " + f + " * " + sprites_x + " * " + sprite_w + ";");
						write.println("\t\ty_base: 0 * " + sprites_y + " * " + sprite_h + " + " + sprite_h + " * 2;");
						write.println("\t\tx_step: " + sprite_w + ";");
						write.println("\t\ty_step: -" + sprite_h + ";");
						write.println("\t\tfile: \"" + out_path + "\";");
						write.println("\t\trecolour: \"" + mask_path + "\";");
						write.println("\t\tx_offset: -32;");
						write.println("\t\ty_offset: 32 - " + sprite_h + ";");
						write.println("\t\twidth: " + sprite_w + ";");
						write.println("\t\theight: " + sprite_h + ";");
					write.println("\t}");
					write.println("\t(sw_{hor(0.." + (sprites_y - 1) + ")}_{vert(0.." + (sprites_x - 1) + ")}): sheet {");
						write.println("\t\tx_base: " + f + " * " + sprites_x + " * " + sprite_w + ";");
						write.println("\t\ty_base: 1 * " + sprites_y + " * " + sprite_h + ";");
						write.println("\t\tx_step: " + sprite_w + ";");
						write.println("\t\ty_step: " + sprite_h + ";");
						write.println("\t\tfile: \"" + out_path + "\";");
						write.println("\t\trecolour: \"" + mask_path + "\";");
						write.println("\t\tx_offset: -32;");
						write.println("\t\ty_offset: 32 - " + sprite_h + ";");
						write.println("\t\twidth: " + sprite_w + ";");
						write.println("\t\theight: " + sprite_h + ";");
					write.println("\t}");
					write.println("\t(nw_{vert(0.." + (sprites_x - 1) + ")}_{hor(0.." + (sprites_y - 1) + ")}): sheet {");
						write.println("\t\tx_base: " + f + " * " + sprites_x + " * " + sprite_w + ";");
						write.println("\t\ty_base: 2 * " + sprites_y + " * " + sprite_h + " + " + sprite_h + " * 2;");
						write.println("\t\tx_step: " + sprite_w + ";");
						write.println("\t\ty_step: -" + sprite_h + ";");
						write.println("\t\tfile: \"" + out_path + "\";");
						write.println("\t\trecolour: \"" + mask_path + "\";");
						write.println("\t\tx_offset: -32;");
						write.println("\t\ty_offset: 32 - " + sprite_h + ";");
						write.println("\t\twidth: " + sprite_w + ";");
						write.println("\t\theight: " + sprite_h + ";");
					write.println("\t}");
					write.println("\t(ne_{hor(0.." + (sprites_y - 1) + ")}_{vert(0.." + (sprites_x - 1) + ")}): sheet {");
						write.println("\t\tx_base: " + f + " * " + sprites_x + " * " + sprite_w + ";");
						write.println("\t\ty_base: 3 * " + sprites_y + " * " + sprite_h + ";");
						write.println("\t\tx_step: " + sprite_w + ";");
						write.println("\t\ty_step: " + sprite_h + ";");
						write.println("\t\tfile: \"" + out_path + "\";");
						write.println("\t\trecolour: \"" + mask_path + "\";");
						write.println("\t\tx_offset: -32;");
						write.println("\t\ty_offset: 32 - " + sprite_h + ";");
						write.println("\t\twidth: " + sprite_w + ";");
						write.println("\t\theight: " + sprite_h + ";");
					write.println("\t}");
				write.println("}");
			}
			write.close();
		}

		System.out.println("Success!");
	}
}
