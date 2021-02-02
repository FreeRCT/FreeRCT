import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.util.*;
import javax.imageio.ImageIO;
import javax.swing.*;

public class GenerateSpritesheet {
	public static void main(String[] args) throws Exception {
		System.out.println("Usage: java GenerateSpritesheet src_dir output_file idOff se ne nw sw frames x y w h");
		int arg_index = 0;
		final String src = args[arg_index++];
		final String result = args[arg_index++];
		final int idOff = Integer.valueOf(args[arg_index++]);
		final String[] prefixes = new String[4];
		for (int i = 0; i < prefixes.length; ++i) prefixes[i] = args[arg_index++];
		final int frames = Integer.valueOf(args[arg_index++]);
		final int sprites_x = Integer.valueOf(args[arg_index++]);
		final int sprites_y = Integer.valueOf(args[arg_index++]);
		final int sprite_w = Integer.valueOf(args[arg_index++]);
		final int sprite_h = Integer.valueOf(args[arg_index++]);

		BufferedImage out = new BufferedImage(frames * sprite_w * sprites_x, sprite_h * sprites_y * 4, BufferedImage.TYPE_INT_ARGB);
		for (int f = 0; f < frames; ++f)
		for (int v = 0; v < 4; ++v)
		for (int x = 0; x < sprites_x; ++x)
		for (int y = 0; y < sprites_y; ++y)
		{
			int tileIndex = Integer.MIN_VALUE;
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
			final int imageID = idOff + f + frames * tileIndex;
			String filename = "" + imageID;
			while (filename.length() < 4) filename = "0" + filename;
			filename = prefixes[v] + filename + ".png";
			System.out.println("Loading: " + src + " → " + filename);
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
