const SIZE = 160;

const sand = "beach_medit_dry";
const grass1 = "grass_temperate_a";
const grass2 = "grass_mediterranean_green_flowers";
const forestFloor = "forrestfloor";
const dirt1 = "grass_sand_75";
const dirt2 = "grass_sand_50";
const dirt3 = "dirt_brown_e";
const cliffBase = "cliff base a";
const cliffBeach = "beech_cliff_a_75";
const cliff = "cliff_face3";

const oTree = "flora_tree_oak";
const oGrass = "props/flora/grass_soft_small.xml"
const oMine = "geology_stone_light";

// Initialize

init(SIZE, grass1, 0);

// Create classes

clImpassable = createTileClass();
clRock = createTileClass();

// Paint elevation

noise0 = new Noise2D(4 * SIZE/128.0);
noise1 = new Noise2D(8 * SIZE/128.0);
noise2 = new Noise2D(11 * SIZE/128.0);
noise3 = new Noise2D(30 * SIZE/128.0);
noise4 = new Noise2D(60 * SIZE/128.0);

for(ix=0; ix<SIZE+1; ix++) {
	for(iy=0; iy<SIZE+1; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		
		// Calculate base noise
		n = (noise0.eval(x, y) + 0.4 * noise1.eval(x, y)) / 1.4;
		
		T = .4;	// Water cutoff
		
		if(n < T) {
			// Tile is underwater - just scale the height down a bit
			h = Math.max(-50 * (T-n)/T, -8);
		}
		else {
			// Tile is above water - add some land noise depending on how far we are from the shoreline
			u = 27*noise1.eval(x, y) + 14*noise2.eval(x,y) + 9 * noise3.eval(x,y) - 14;
			h = 8*(n-T) + Math.max(0, lerp(0, u, Math.min(.1, n-T)*10));
			h += 0.4*noise4.eval(x, y);
		}
		
		setHeight(ix, iy, h);
	}
}

// Paint terrains

for(ix=0; ix<SIZE; ix++) {
	for(iy=0; iy<SIZE; iy++) {
		h00 = getHeight(ix, iy);
		h01 = getHeight(ix, iy+1);
		h10 = getHeight(ix+1, iy);
		h11 = getHeight(ix+1, iy+1);
		maxH = Math.max(h00, h01, h10, h11);
		minH = Math.min(h00, h01, h10, h11);
		if(maxH <= 0) {
			setTexture(ix, iy, sand);
			addToClass(ix, iy, clImpassable);
		}
		else if(maxH - minH > 3.2) {
			setTexture(ix, iy, cliff);
			addToClass(ix, iy, clImpassable);
		}
		else if(maxH - minH > 2.7) {
			setTexture(ix, iy, cliffBase);
			addToClass(ix, iy, clImpassable);
		}
		else if(minH <= 0) {
			setTexture(ix, iy, sand);
			addToClass(ix, iy, clImpassable);
		}
		else {
			setTexture(ix, iy, grass1);
		}
	}
}

// Paint forest and dirt

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
		if(maxH - minH < 1.7 && minH > 0) {
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
				addToClass(ix, iy, clImpassable);
				if(randFloat() < .7) {
					setTexture(ix, iy, forestFloor);
				}
			}
		}
	}
}

println("Creating mines...");
group = new SimpleGroup([new SimpleObject(oMine, 3,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	new AvoidTileClassConstraint(clImpassable, 2, clRock, 13),
	12, 100
);

