const SIZE = 160;
const SEA_LEVEL = 5;

init(SIZE, "ocean_medit_rock_shallow", 0);

noise0 = new Noise2D(4 * SIZE/128.0);
noise1 = new Noise2D(8 * SIZE/128.0);
noise2 = new Noise2D(11 * SIZE/128.0);
noise3 = new Noise2D(30 * SIZE/128.0);
noise4 = new Noise2D(60 * SIZE/128.0);

// Paint elevation

for(ix=0; ix<SIZE+1; ix++) {
	for(iy=0; iy<SIZE+1; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		
		// Calculate base noise
		n = (noise0.eval(x, y) + 0.4 * noise1.eval(x, y)) / 1.4;
		
		T = .4;	// Water cutoff
		
		if(n < T) {
			// Tile is underwater - just scale the height down a bit
			h = Math.max(-60 * (T-n)/T, -30);
		}
		else {
			// Tile is above water - add some land noise depending on how far
			// we are from the shoreline
			u = 25*noise1.eval(x, y) + 12*noise2.eval(x,y) + 8 * noise3.eval(x,y) - 11;
			h = 8*(n-T) + Math.max(0, lerp(0, u, Math.min(.1, n-T)*10));
			h += 0.4*noise4.eval(x, y);
		}
		
		h += SEA_LEVEL;
		
		setHeight(ix, iy, h);
	}
}

// Paint terrains

sand = "beach_medit_dry";
grass1 = "grass_temperate_a";
grass2 = "grass_mediterranean_green_flowers";
forestFloor = "forrestfloor";
dirt1 = "grass_sand_75";
dirt2 = "grass_sand_50";
dirt3 = "dirt_brown_e";
cliffBase = "cliff base a";
cliffBeach = "beech_cliff_a_75";
cliff = "cliff_face3";

for(ix=0; ix<SIZE; ix++) {
	for(iy=0; iy<SIZE; iy++) {
		h00 = getHeight(ix, iy);
		h01 = getHeight(ix, iy+1);
		h10 = getHeight(ix+1, iy);
		h11 = getHeight(ix+1, iy+1);
		maxH = Math.max(h00, h01, h10, h11);
		minH = Math.min(h00, h01, h10, h11);
		if(maxH <= SEA_LEVEL) {
			setTexture(ix, iy, sand);
		}
		else if(maxH - minH > 3.2 && minH >= SEA_LEVEL) {
			setTexture(ix, iy, cliff);
		}
		else if(maxH - minH > 2.7 && minH < SEA_LEVEL) {
			setTexture(ix, iy, cliffBeach);
		}
		else if(maxH - minH > 2.7) {
			setTexture(ix, iy, cliffBase);
		}
		else if(minH <= SEA_LEVEL) {
			setTexture(ix, iy, sand);
		}
		else {
			setTexture(ix, iy, grass1);
		}
	}
}

oTree = "flora_tree_oak";
oGrass = "props/flora/grass_soft_small.xml"

forestNoise1 = new Noise2D(20 * SIZE/128.0);
forestNoise2 = new Noise2D(40 * SIZE/128.0);

dirtNoise = new Noise2D(80 * SIZE/128.0);

for(ix=0; ix<SIZE; ix++) {
	for(iy=0; iy<SIZE; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		h00 = getHeight(ix, iy);
		h01 = getHeight(ix, iy+1);
		h10 = getHeight(ix+1, iy);
		h11 = getHeight(ix+1, iy+1);
		maxH = Math.max(h00, h01, h10, h11);
		minH = Math.min(h00, h01, h10, h11);
		if(maxH - minH < 1.7 && minH > SEA_LEVEL) {
			fn = (forestNoise1.eval(x,y) + .5*forestNoise1.eval(x,y)) / 1.5;
			
			if(minH > .5 && fn < .38 && dirtNoise.eval(x,y) > .55) {
				if(dirtNoise.eval(x,y) > .72) {
					setTexture(ix, iy, dirt2);
				}
				else {
					setTexture(ix, iy, dirt1);
				}
			}
			
			if(fn > .6 && randFloat() < (.3 + .7 * Math.min(fn-.6, .1) / .1) ) {
				placeObject(oTree, 0, ix+.4+.2*randFloat(), iy+.4+.2*randFloat(), randFloat()*2*Math.PI);
				if(randFloat() < .7) {
					setTexture(ix, iy, forestFloor);
				}
			}
			else if(getTexture(ix, iy)==grass1 && maxH-minH < 1 && randFloat() < .13) {
				placeObject(oGrass, 0, ix+.3+.4*randFloat(), iy+.3+.4*randFloat(), randFloat()*2*Math.PI);
			}
			else if(getTexture(ix, iy)==grass1 && randFloat() < .03) {
				placeObject(oTree, 0, ix+.3+.4*randFloat(), iy+.3+.4*randFloat(), randFloat()*2*Math.PI);
			}
			
			/*else if(getTerrain(ix, iy)==grass && fn > .5 && randFloat() < .04) {
				placeBushClump(ix+.5, iy+.5);
			}
			else if(randFloat() < .04 && (maxH < .5 || randFloat() < .2)) {
				placeRockClump(ix+.5, iy+.5);
			}
			else if(getTerrain(ix, iy)==grass && maxH-minH < .4 && randFloat() < .1) {
				addSprite(new Sprite(makeGrass(), 
					ix+.3+.4*randFloat(), iy+.3+.4*randFloat(), STATIC));
			}
			else if(randFloat() < .015) {
				addSprite(new Sprite(makeSmallRock(), 
					ix+randFloat(), iy+randFloat(), STATIC));
			}
			else if(getTerrain(ix, iy)==grass && randFloat() < .03) {
				var t = new Sprite(makeOak(), 
					ix+.3+.4*randFloat(), iy+.3+4*randFloat(), STATIC);
				t.maxY = 4;
				t.collidable = true;
				addSprite(t);
			}*/
		}
		
		/*if(maxH < .5 && minH < -0.3 && minH > -2 && randFloat() < .007) {
			var l = new Sprite(waterLog, ix+randFloat(), iy+randFloat(), STATIC);
			l.floating = true;
			addSprite(l);
		}
		else if(maxH < 0 && minH > -3 && randFloat() < .04) {
			var l = new Sprite(lillies, ix+randFloat(), iy+randFloat(), STATIC);
			l.floating = true;
			addSprite(l);
		}*/
	}
}
