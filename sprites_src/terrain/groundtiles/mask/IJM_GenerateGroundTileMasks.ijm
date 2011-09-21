path=getDirectory("");

tilew=newArray(64, 128, 256);
var carrn=newArray(0, 1, 1, 1, 1, 2);
var carre=newArray(0, 0, 1, 0, 1, 1);
var carrs=newArray(0, 0, 0, 1, 1, 0);
var carrw=newArray(0, 0, 0, 0, 0, 1);
slfac=0.25;
borderfac=16;
boffs=newArray(0, 0, 0.25, 0, 0.5, 0.5);
dotdensity=4;

nlayouts=3;
simgs=newArray(nlayouts);
timgs=newArray(nlayouts);

//setBatchMode(true);
for (i=0; i<lengthOf(tilew); i++) {
	for (j=0; j<nlayouts; j++) {
		newImage("Temp1", "8-bit Black", tilew[i], tilew[i], lengthOf(carrn)*4);
		simgs[j]=getImageID();
	}
	for (c=0; c<lengthOf(carrn); c++) {
		for (o=0; o<4; o++) {
			var carr=newArray(4);
			carr[(0+o)%4]=carrn[c];
			carr[(1+o)%4]=carre[c];
			carr[(2+o)%4]=carrs[c];
			carr[(3+o)%4]=carrw[c];
			selectImage(simgs[0]);
			setSlice(c*4+o+1);
			makeSele(tilew[i], slfac, 0, 0, 255);
			selectImage(simgs[1]);
			setSlice(c*4+o+1);
			makeSele(tilew[i], slfac, 0, 0, 255);
			twtmp=tilew[i]-2*tilew[i]/borderfac;
			twxoff=tilew[i]/borderfac;
			ta=carr[1]+carr[3];
			twyoff=(boffs[c]+0.5)*tilew[i]/borderfac;
			makeSele(twtmp, slfac, twxoff, twyoff, 0);
			run("Select All");
			run("Copy");
			selectImage(simgs[2]);
			setSlice(c*4+o+1);
			run("Paste");
			makeSele(twtmp, slfac, twxoff, twyoff, 128);
			for (x=0; x<getWidth(); x++) {
				for (y=0; y<getHeight(); y++) {
					if (x%dotdensity==0 && y%dotdensity==0 && getPixel(x, y)==128) {
						setPixel(x, y, 255);
					}
				}
			}
			changeValues(0, 254, 0);
		}
	}
	for (j=0; j<nlayouts; j++) {
		selectImage(simgs[j]);
		run("Flip Vertically", "stack");
		newImage("Temp1", "8-bit Black", tilew[i]*4, tilew[i]*lengthOf(carrn), 1);
		timgs[j]=getImageID();
	}
	for (c=0; c<lengthOf(carrn); c++) {
		for (o=0; o<4; o++) {
			for (j=0; j<nlayouts; j++) {
				selectImage(simgs[j]);
				setSlice(c*4+o+1);
				run("Select All");
				run("Copy");
				setPasteMode("Copy");
				selectImage(timgs[j]);
				makeRectangle(o*tilew[i], c*tilew[i], tilew[i], tilew[i]);
				run("Paste");
			}
		}
	}
	for (j=0; j<nlayouts; j++) {
		selectImage(simgs[j]);
		close();
		selectImage(timgs[j]);
		if (j==0) {
			str="";
		} else {
			str=j;
		}
		saveAs("PNG", path+tilew[i]+"px"+str+".png");
		close();
	}
}

function makeSele(tilew, slfac, xoffs, yoffs, col) {
	slshift=tilew*slfac;
	xs=newArray(0, -1, tilew/2+1, tilew/2-1, tilew+1, tilew+1, tilew/2-1, tilew/2+1);
	xs[0]=-1;
	ys=newArray(0, tilew/4+carr[3]*slshift, 0-1+carr[2]*slshift, 0-1+carr[2]*slshift, tilew/4+carr[1]*slshift, tilew/4-1+carr[1]*slshift, tilew/2+carr[0]*slshift, tilew/2+carr[0]*slshift);
	ys[0]=tilew/4-1+carr[3]*slshift;
	for (i=0; i<lengthOf(xs); i++) {
		xs[i]+=xoffs;
		ys[i]+=yoffs;
	}
	makeSelection("polygon", xs, ys);
	setColor(col);
	getRawStatistics(area);
	if (area!=0) {
		fill();
	}
	run("Select None");
}
