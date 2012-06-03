//ImageJ macros for managing renders
//Note there are issues with using these scripts in ImageJ versions later than 1.44 (due to changes in handling of transparency)
//ImageJ version 1.44 is known to work correctly.

//Key resource arrays
//Tile widths to expect for renders
var tilew=newArray(64, 128, 256);
//Arrays for LUT/palette
var r=newArray(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 35, 47, 63, 75, 91, 111, 131, 159, 183, 211, 239, 51, 63, 79, 91, 107, 119, 135, 151, 167, 187, 203, 223, 67, 87, 111, 127, 143, 159, 179, 191, 203, 219, 231, 247, 71, 95, 119, 143, 167, 191, 215, 243, 255, 255, 255, 255, 35, 79, 95, 111, 127, 143, 163, 179, 199, 215, 235, 255, 27, 35, 47, 59, 71, 87, 99, 115, 131, 147, 163, 183, 31, 47, 59, 75, 91, 111, 135, 159, 183, 195, 207, 223, 15, 19, 23, 31, 39, 55, 71, 91, 111, 139, 163, 195, 79, 99, 119, 139, 167, 187, 207, 215, 227, 239, 247, 255, 15, 39, 51, 63, 83, 99, 119, 139, 159, 183, 211, 239, 0, 0, 7, 15, 27, 43, 67, 91, 119, 143, 175, 215, 11, 15, 23, 35, 47, 59, 79, 99, 123, 147, 175, 207, 63, 75, 83, 95, 107, 123, 135, 155, 171, 191, 215, 243, 63, 87, 115, 143, 171, 199, 227, 255, 255, 255, 255, 255, 79, 111, 147, 183, 219, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 7, 23, 43, 71, 99, 131, 171, 207, 63, 91, 119, 147, 179, 199, 219, 239, 243, 247, 251, 255, 39, 55, 71, 91, 107, 123, 143, 163, 187, 207, 231, 255, 255, 255, 255, 255, 7, 7, 7, 27, 39, 55, 55, 55, 115, 155, 47, 87, 47, 0, 27, 39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
var g=newArray(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35, 51, 67, 83, 99, 115, 131, 151, 175, 195, 219, 243, 47, 59, 75, 91, 107, 123, 139, 155, 175, 191, 207, 227, 43, 59, 75, 87, 99, 115, 131, 151, 175, 199, 219, 239, 27, 43, 63, 83, 111, 139, 167, 203, 231, 243, 251, 255, 0, 0, 7, 15, 27, 39, 59, 79, 103, 127, 159, 191, 51, 63, 79, 95, 111, 127, 143, 155, 171, 187, 203, 219, 55, 71, 83, 99, 111, 135, 159, 183, 207, 219, 231, 247, 63, 83, 103, 123, 143, 159, 175, 191, 207, 223, 239, 255, 43, 55, 71, 87, 99, 115, 131, 151, 171, 191, 207, 227, 19, 43, 55, 67, 83, 99, 119, 139, 159, 183, 211, 239, 27, 39, 51, 67, 83, 103, 135, 163, 187, 211, 231, 247, 43, 55, 71, 83, 99, 115, 135, 155, 175, 199, 219, 243, 0, 7, 15, 31, 43, 63, 83, 103, 127, 155, 195, 235, 0, 0, 0, 0, 0, 0, 7, 7, 79, 123, 171, 219, 39, 51, 63, 71, 79, 83, 111, 139, 163, 183, 203, 219, 51, 63, 75, 87, 107, 127, 147, 167, 187, 207, 231, 255, 0, 0, 0, 7, 11, 31, 59, 91, 119, 151, 183, 215, 19, 31, 47, 63, 83, 103, 127, 147, 171, 195, 219, 243, 0, 183, 219, 255, 107, 107, 107, 131, 143, 155, 155, 155, 203, 227, 47, 71, 47, 0, 43, 59, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
var b=newArray(255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35, 51, 67, 83, 99, 115, 131, 151, 175, 195, 219, 243, 0, 0, 11, 19, 31, 47, 59, 79, 95, 115, 139, 163, 7, 11, 23, 31, 39, 51, 67, 87, 111, 135, 163, 195, 0, 0, 0, 7, 7, 15, 19, 27, 47, 95, 143, 195, 0, 0, 7, 15, 27, 39, 59, 79, 103, 127, 159, 191, 19, 23, 31, 39, 43, 51, 59, 67, 75, 83, 95, 103, 27, 35, 43, 55, 67, 79, 95, 111, 127, 147, 167, 191, 0, 0, 0, 0, 7, 23, 39, 63, 87, 115, 143, 179, 19, 27, 43, 59, 67, 83, 99, 115, 131, 151, 171, 195, 55, 87, 103, 119, 139, 155, 175, 191, 207, 223, 239, 255, 111, 151, 167, 187, 203, 223, 227, 231, 239, 243, 251, 255, 15, 23, 31, 43, 59, 75, 95, 119, 139, 167, 195, 223, 95, 115, 127, 143, 155, 171, 187, 199, 215, 231, 243, 255, 0, 0, 0, 0, 0, 0, 0, 0, 67, 115, 163, 215, 0, 0, 0, 0, 0, 0, 23, 51, 79, 107, 135, 163, 47, 55, 67, 79, 99, 119, 143, 163, 187, 207, 231, 255, 27, 39, 59, 75, 99, 119, 143, 171, 187, 203, 223, 239, 0, 7, 15, 31, 51, 75, 107, 127, 147, 171, 195, 223, 255, 0, 0, 0, 99, 99, 99, 123, 135, 151, 151, 151, 203, 227, 47, 47, 47, 99, 139, 151, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
//Length of string for tile index number
var indexleng=5;

//8bpp-ize renders
//Converts all images in a folder which match a certain naming scheme to 8bpp
//The user specifies the target folder. The resulting images are saved to a parallel folder with the same name but appended with "8bpp"
//Uses a shaded and unshaded render pass to generate the 8bpp image
//The final pixel values are limited to those which match the hue of the shadeless render
macro "8bpp-ize Renders [q]" {
	//Determine the directory to work in
	fpath=getDirectory("");
	path=File.getParent(fpath)+"\\";
	name=File.getName(fpath);

	//Get user input - whether to overwrite and dither level
	Dialog.create("8bpp-ize Renders");
	Dialog.addNumber("Dither factor:", 8, 0, 5, "");
	Dialog.addCheckbox("Overwrite existing 8bpp images", false);
	Dialog.show();
	dither=Dialog.getNumber();
	overwrite=Dialog.getCheckbox();

	//If the target output directory does not exist then create it
	if (File.exists(path+name+"8bpp\\")==false) {
		File.makeDirectory(path+name+"8bpp\\");
	}

	setBatchMode(true);
	//Repeat for all tile widths
	for (i=0; i<lengthOf(tilew); i++) {
		j=1;
		str=padString(j);
		//While the files exist loop through files named <tilewidth>_XXXXX.png and convert
		while (File.exists(path+name+"\\"+tilew[i]+"_"+str+".png")==true) {
			showStatus(j);
			//Check if output file exists or if overwriting is enabled
			if (File.exists(path+name+"8bpp\\"+tilew[i]+"_"+str+".png")==false || overwrite==true) {
				//Open and resize the shadeless image
				open(path+name+"\\"+tilew[lengthOf(tilew)-1]+"m_"+str+".png");
				run("Size...", "width="+tilew[i]+" height="+tilew[i]+" interpolation=None");
				run("Select All");
				run("Copy");
				close();
				//Open the shaded image
				open(path+name+"\\"+tilew[i]+"_"+str+".png");
				run("Add Slice");
				run("Paste");
				//Call the convert to 8bpp function
				convertTo8Bit(dither);
				setSlice(1);
				//Save output
				saveAs("PNG", path+name+"8bpp\\"+tilew[i]+"_"+str+".png");
				close();
			}
			//Prepare for next image
			j++;
			str=padString(j);
		}
	}
}

//Padding function to increase string length to match the blender output naming scheme
function padString(str) {
	str=""+str;
	while (lengthOf(str)<indexleng) {
		str="0"+str;
	}
	return str;
}

//Function for 8bpp conversion
function convertTo8Bit(dither) {
	//Loop over all pixels
	w=getWidth();
	h=getHeight();
	for (x=0; x<w; x++) {
		showProgress(x/w);
		for (y=0; y<h; y++) {
			//Check the shadeless image and determine the colour hue
			setSlice(2);
			v=getPixel(x, y);
			rp=(v>>16)&0xff;
			gp=(v>>8)&0xff;
			bp=(v>>0)&0xff;
			if (rp==0 && gp==0 && bp==0) {
				//If the shadeless image is pure black then set it to transparent
				index=0;
			} else {
				//Else search for the nearest hue
				d=256*256;
				index=0;
				min=10;
				max=10+18*12;
				for (i=min; i<max; i++) {
					di=pow(pow(rp-r[i], 2)+pow(gp-g[i], 2)+pow(bp-b[i], 2), 0.5);
					if (di<d) {
						d=di;
						index=i;
					}
				}
				huelimit=floor((index-10)/12);
				//Using the hue limit determine the 8bpp palette entry for the shaded image
				setSlice(1);
				v=getPixel(x, y);
				rp=(v>>16)&0xff;
				gp=(v>>8)&0xff;
				bp=(v>>0)&0xff;
				if (rp==0 && gp==0 && bp==0) {
					//If the shaded image is pure black then set it to transparent
					index=0;
				} else {
					//else search for the nearest match
					rp+=floor((random()-0.5)*dither);
					gp+=floor((random()-0.5)*dither);
					bp+=floor((random()-0.5)*dither);
					d=256*256;
					index=0;
					min=huelimit*12+10;
					max=min+12;
					for (i=min; i<max; i++) {
						di=pow(pow(rp-r[i], 2)+pow(gp-g[i], 2)+pow(bp-b[i], 2), 0.5);
						if (di<d) {
							d=di;
							index=i;
						}
					}
				}
			}
			//Set the new pixel value
			vn=(index&0xff)<<16+(index&0xff)<<8+(index&0xff)<<0;
			setSlice(1);
			setPixel(x, y, vn);
		}
	}
	//Convert to 8bpp
	//Note 8bpp with a look up table (lut) == paletted 8bpp colour image
	run("8-bit");
	setLut(r, g, b);
}

//Merge renders
//Takes all the images in folder and tiles them into a grid
//Used to merge together the single renders into the sprite sheets
macro "Merge Renders [w]" {
	//Determine the directory to work in
	fpath=getDirectory("");
	path=File.getParent(fpath)+"\\";
	name=File.getName(fpath);

	//Get user input - whether to overwrite and dither level
	bitdepths=newArray("8bpp", "32bpp");
	Dialog.create("Merge Renders");
	Dialog.addNumber("X size factor:", 1, 0, 5, "");
	Dialog.addNumber("Y size factor:", 1, 0, 5, "");
	Dialog.addNumber("Columns:", 4, 0, 5, "");
	Dialog.addNumber("Rows:", 8, 0, 5, "");
	Dialog.addCheckbox("Auto-count rows", true);
	Dialog.addChoice("Bit depth:", bitdepths, bitdepths[0]);
	Dialog.show();
	tiw=Dialog.getNumber();
	tih=Dialog.getNumber();
	w=Dialog.getNumber();
	h=Dialog.getNumber();
	autoh=Dialog.getCheckbox();
	bitdepth=Dialog.getChoice();

	setBatchMode(true);

	//Count the images for auto height
	for (i=0; i<lengthOf(tilew); i++) {
		j=1;
		str=padString(j);
		//While the files exist loop through files named <tilewidth>_XXXXX.png and count
		while (File.exists(path+name+"\\"+tilew[i]+"_"+str+".png")==true) {
			showStatus(j);
			j++;
			str=padString(j);
		}
	}
	if (autoh==true) {
		h=floor(j-1)/w;
	}

	//Loop through tile widths
	for (i=0; i<lengthOf(tilew); i++) {
		//Calculate output image size and make the output image
		iw=tilew[i]*tiw*w;
		ih=tilew[i]*tih*h;
		if (bitdepth=="32bpp") {
			newImage("Traces", "RGB White", iw, ih, 1);
		} else {
			newImage("Traces", "8-bit White", iw, ih, 1);
		}
		//Wile the files exist loop through named <tilewidth>_XXXXX.png and arrange into a grid
		j=1;
		str=padString(j);
		while (File.exists(path+name+"\\"+tilew[i]+"_"+str+".png")==true) {
			showStatus(j);
			open(path+name+"\\"+tilew[i]+"_"+str+".png");
			run("Select All");
			run("Copy");
			close();
			setPasteMode("Copy");
			makeRectangle(((j-1)%w)*tilew[i]*tiw, floor((j-1)/w)*tilew[i]*tih, tilew[i]*tiw, tilew[i]*tih);
			run("Paste");
			j++;
			str=padString(j);
		}
		//Finalise image and save
		if (bitdepth=="8bpp") {
			setLut(r, g, b);
		}
		saveAs("PNG", path+name+tilew[i]+".png");
		close();
	}
}

//Sprite Sheet Masking
//Masks all sprite sheets in the selected directory
macro "Sprite Sheet Masking [e]" {
	//Determine the directory to work in
	fpath=getDirectory("");

	setBatchMode(true);
	filearr=getFileList(fpath);
	for (i=0; i<lengthOf(filearr); i++) {
		if (endsWith(filearr[i], ".png")==true) {
			for (j=0; j<lengthOf(tilew); j++) {
				name=substring(filearr[i], 0, lengthOf(filearr[i])-4);
				if (endsWith(name, ""+tilew[j])==true) {
					for (k=0; k<3; k++) {
						if (k==0) {
							str="";
						} else {
							str=""+k;
						}
						open(fpath+"\\mask\\"+tilew[j]+"px"+str+".png");
						maskh=getHeight();
						maskw=getWidth();
						run("Select All");
						run("Copy");
						close();
						open(fpath+name+".png");
						setPasteMode("Transparent-white");
						for (l=0; l<getHeight()/maskh; l++) {
							makeRectangle(0, l*maskh, maskw, maskh);
							run("Paste");
						}
						saveAs("PNG", fpath+name+"_masked"+str+".png");
						close();
					}
				}
			}
		}
	}
}