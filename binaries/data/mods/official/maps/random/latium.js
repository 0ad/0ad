const SIZE = 176;
const NUM_PLAYERS = 4;
const WATER_WIDTH = .2;

// Terrain and object constants

tOceanDepths = "ocean_medit_depths";
tOceanRockDeep = "ocean_medit_rock_deep";
tOceanRockShallow = "ocean_medit_rock_shallow";
tOceanCoral = "ocean_medit_coral";
tBeachWet = "beach_medit_wet";
tBeachDry = "beach_medit_dry";
tBeachGrass = "beach_medit_grass_50";
tBeachCliff = "cliff_medit_beach";
tGrassDry = ["grass_mediterranean_dry_a", "grass_mediterranean_dry_b", "grass_mediterranean_dry_c"];
tGrass = ["grass_mediterranean_green_50", "grass_mediterranean_green_flowers"];
tGrassLush = ["grass_temperate_dry_tufts", "grass_mediterranean_green_flowers"];
tGrassShrubs = ["grass_mediterranean_green_shrubs", "grass_mediterranean_green_flowers"];
tGrassRock = ["grass_mediterranean_green_rock"];
tDirt = "dirt_medit_a";
tDirtGrass = "dirt_medit_grass_50";
tDirtCliff = "cliff_medit_dirt";
tGrassCliff = "cliff_medit_grass_a";
tCliff = ["cliff_medit_face_b", "cliff_medit_face_b", "cliff_medit_foliage_a"];
tForestFloor = "forestfloor_medit_dirt";

oPalm = "flora_tree_medit_fan_palm";
oLombardyPoplar = "flora_tree_poplar_lombardy";
oOak = "flora_tree_oak";
oPoplar = "flora_tree_poplar";
oCarob = "flora_tree_carob";
oBeech = "flora_tree_euro_beech";
oPine = "flora_tree_aleppo_pine";
oBerryBush = "flora_bush_berry";
oSheep = "fauna_sheep";
oStone = "geology_stone_greek";
oMetal = "geology_metal_greek";
oBushLargeDry = "props/flora/bush_medit_la_dry.xml";
oBushLarge = "props/flora/bush_medit_la.xml";
oBushMedDry = "props/flora/bush_medit_me_dry.xml";
oBushMed = "props/flora/bush_medit_me.xml";
oBushSmall = "props/flora/bush_medit_sm.xml"
oBushSmallDry = "props/flora/bush_medit_sm_dry.xml"
oGrass = "props/flora/grass_medit_field.xml";
oGrassDry = "props/flora/grass_soft_dry_small.xml";
oRockLarge = "geology/stone_granite_greek_large.xml";
oRockMed = "geology/stone_granite_greek_med.xml";
oRockSmall = "geology/stone_granite_greek_small.xml";
oWaterLog = "props/flora/water_log.xml";

tPalmForest = [tForestFloor+"|"+oPalm, tGrass];
tPineForest = [tForestFloor+"|"+oPine, tGrass];
tMainForest = [tForestFloor+"|"+oCarob, tForestFloor+"|"+oBeech, tGrass, tGrass];
tPoplarForest = [tForestFloor+"|"+oLombardyPoplar, tGrass];

// Initialize world

init(SIZE, tGrass, 0);

// Create classes

clWater = createTileClass();
clCliff = createTileClass();
clForest = createTileClass();
clMetal = createTileClass();
clStone = createTileClass();
clFood = createTileClass();
clPlayer = createTileClass();
clBaseResource = createTileClass();
clSettlement = createTileClass();

// Place players

println("Placing players...");

playerX = new Array(NUM_PLAYERS+1);
playerY = new Array(NUM_PLAYERS+1);

numLeftPlayers = Math.floor(NUM_PLAYERS/2);
for(i=1; i<=numLeftPlayers; i++) {
	playerX[i] = 0.28 + (2*randFloat()-1)*0.01;
	playerY[i] = (0.5+i-1)/numLeftPlayers + (2*randFloat()-1)*0.01;
}
for(i=numLeftPlayers+1; i<=NUM_PLAYERS; i++) {
	playerX[i] = 0.72 + (2*randFloat()-1)*0.01;
	playerY[i] = (0.5+i-numLeftPlayers-1)/numLeftPlayers + (2*randFloat()-1)*0.01;
}

for(i=1; i<=NUM_PLAYERS; i++) {
	// get fractional locations in tiles
	ix = round(fractionToTiles(playerX[i]));
	iy = round(fractionToTiles(playerY[i]));
	addToClass(ix, iy, clPlayer);
	
	// create TC and starting units
	placeObject("special_settlement", i, ix, iy, PI*3/4);
	placeObject("hele_civil_centre", i, ix, iy, PI*3/4);
	group = new SimpleGroup(
		[new SimpleObject("hele_infantry_javelinist_b", 3,3, 5,5)],
		true, null,	ix, iy
	);
	createObjectGroup(group, i);
	
	// create starting berry bushes
	bbAngle = randFloat()*2*PI;
	bbDist = 9;
	bbX = round(ix + bbDist * cos(bbAngle));
	bbY = round(iy + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,2)],
		true, clBaseResource, bbX, bbY
	);
	createObjectGroup(group, 0);
	
	// create starting mines
	mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3) {
		mAngle = randFloat()*2*PI;
	}
	mDist = 9;
	mX = round(ix + mDist * cos(mAngle));
	mY = round(iy + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStone, 2,2, 0,3),
		new SimpleObject(oMetal, 2,2, 0,3)],
		true, clBaseResource, mX, mY
	);
	createObjectGroup(group, 0);
	
	// create starting straggler trees
	group = new SimpleGroup(
		[new SimpleObject(oPalm, 3,3, 7,10)],
		true, clBaseResource, ix, iy
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

function distanceToPlayers(x, y) {
	var r = 10000;
	for(var i=1; i<=NUM_PLAYERS; i++) {
		var dx = x-playerX[i];
		var dy = y-playerY[i];
		r = min(r, dx*dx + dy*dy);
	}
	return Math.sqrt(r);
}

function playerNearness(x, y) {
	var d = fractionToTiles(distanceToPlayers(x,y));
	if(d < 13) return 0;
	else if(d < 19) return (d-13)/(19-13);
	else return 1;
}

function max(x, y) {
	return x > y ? x : y;
}

function min(x, y) {
	return x < y ? x : y;
}

// Paint elevation

println("Painting elevation...");

noise0 = new Noise2D(4 * SIZE/128);
noise1 = new Noise2D(8 * SIZE/128);
noise2 = new Noise2D(15 * SIZE/128);

noise2a = new Noise2D(20 * SIZE/128);
noise2b = new Noise2D(35 * SIZE/128);

noise3 = new Noise2D(4 * SIZE/128);
noise4 = new Noise2D(6 * SIZE/128);
noise5 = new Noise2D(11 * SIZE/128);

for(ix=0; ix<=SIZE; ix++) {
	for(iy=0; iy<=SIZE; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		pn = playerNearness(x, y);
		
		h = 0;
		distToWater = 0;
		
		h = 32 * (x-.5);
		
		
		// add the rough shape of the water
		if(x < WATER_WIDTH) {
			h = max(-16.0, -28.0*(WATER_WIDTH-x)/WATER_WIDTH);
		}
		else if(x > 1.0-WATER_WIDTH) {
			h = max(-16.0, -28.0*(x-(1.0-WATER_WIDTH))/WATER_WIDTH);
		}
		else {
			distToWater = (0.5 - WATER_WIDTH - Math.abs(x-0.5));
			u = 1 - Math.abs(x-0.5) / (0.5-WATER_WIDTH);
			h = 12*u;
		}
		
		// add some base noise
		baseNoise = 16*noise0.eval(x,y) + 8*noise1.eval(x,y) + 4*noise2.eval(x,y) - (16+8+4)/2;
		if( baseNoise < 0 ) {
			baseNoise *= pn;
			baseNoise *= max(0.1, distToWater / (0.5-WATER_WIDTH));
		}
		oldH = h;
		h += baseNoise;
		
		// add some higher-frequency noise on land
		if( oldH > 0 )
		{
			h += (0.4*noise2a.eval(x,y) + 0.2*noise2b.eval(x,y)) * min(oldH/10.0, 1.0);
		}
		
		// create cliff noise
		if( h > -10 )
		{
			cliffNoise = (1*noise3.eval(x,y) + 0.5*noise4.eval(x,y)) / 1.5;
			if(h < 1) {
				u = 1 - .3*((h-1)/-10);
				cliffNoise *= u;
			}
			cliffNoise += .05 * distToWater / (0.5 - WATER_WIDTH);
			if(cliffNoise > .6) {
				u = 0.8 * (cliffNoise-.6);
				cliffNoise += u * noise5.eval(x,y);
				cliffNoise /= (1+u);
			}
			cliffNoise -= 0.59;
			cliffNoise *= pn;
			if(cliffNoise > 0) {
				h += 19 * min(cliffNoise, 0.045) / 0.045;
			}
		}
		
		// set the height
		setHeight(ix, iy, h);
	}
}

// Paint base terrain

println("Painting terrain...");

noise6 = new Noise2D(10 * SIZE/128);
noise7 = new Noise2D(20 * SIZE/128);

noise8 = new Noise2D(13 * SIZE/128);
noise9 = new Noise2D(26 * SIZE/128);

noise10 = new Noise2D(50 * SIZE/128);

for(ix=0; ix<SIZE; ix++) {
	for(iy=0; iy<SIZE; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		pn = playerNearness(x, y);
		
		// get heights of surrounding vertices
		h00 = getHeight(ix, iy);
		h01 = getHeight(ix, iy+1);
		h10 = getHeight(ix+1, iy);
		h11 = getHeight(ix+1, iy+1);
		
		// find min and max height
		maxH = max(h00, h01, h10, h11);
		minH = min(h00, h01, h10, h11);
		
		// figure out if we're at the top of a cliff using min adjacent height
		minAdjHeight = minH;
		if(maxH > 15) {
			for(nx=max(ix-1, 0); nx<=min(ix+2, SIZE); nx++) {
				for(ny=max(iy-1, 0); ny<=min(iy+2, SIZE); ny++) {
					minAdjHeight = min(minAdjHeight, getHeight(nx, ny));
				}
			}
		}
		
		// choose a terrain based on elevation
		t = tGrass;
		
		// water
		if(maxH < -12) {
			t = tOceanDepths;
		}
		else if(maxH < -8.8) {
			t = tOceanRockDeep;
		}
		else if(maxH < -4.7) {
			t = tOceanCoral;
		}
		else if(maxH < -2.8) {
			t = tOceanRockShallow;
		}
		else if(maxH < .9 && minH < .35) {
			t = tBeachWet;
		}
		else if(maxH < 1.5 && minH < .9) {
			t = tBeachDry;
		}
		else if(maxH < 2.3 && minH < 1.3) {
			t = tBeachGrass;
		}
		
		if(minH < 0) {
			addToClass(ix, iy, clWater);
		}
		
		// cliffs
		if(maxH - minH > 2.9 && minH > -7) {
			t = tCliff;
			addToClass(ix, iy, clCliff);
		}
		else if((maxH - minH > 2.5 && minH > -5) || (maxH-minAdjHeight > 2.9 && minH > 0) ) {
			if(minH < -1) t = tCliff;
			else if(minH < .5) t = tBeachCliff;
			else t = [tDirtCliff, tGrassCliff, tGrassCliff, tGrassRock, tCliff];
			addToClass(ix, iy, clCliff);
		}
		
		// forests
		if(maxH - minH < 1 && minH > 1) {
			forestNoise = (noise6.eval(x,y) + 0.5*noise7.eval(x,y)) / 1.5 * pn;
			forestNoise -= 0.59;
			if(forestNoise > 0) {
				if(minH > 5) {
					typeNoise = noise10.eval(x,y);
					if(typeNoise < .43 && forestNoise < .05) t = tPoplarForest;
					else if(typeNoise < .63) t = tMainForest;
					else t = tPineForest;
					addToClass(ix, iy, clForest);
				}
				else if(minH < 3) {
					t = tPalmForest;
					addToClass(ix, iy, clForest);
				}
			}
		}
		
		// grass variations
		if(t==tGrass)
		{
			grassNoise = (noise8.eval(x,y) + .6*noise9.eval(x,y)) / 1.6;
			if(grassNoise < .3) {
				t = (maxH - minH > 1.2) ? tDirtCliff : tDirt;
			}
			else if(grassNoise < .34) {
				t = (maxH - minH > 1.2) ? tGrassCliff : tGrassDry;
				if(maxH - minH < .5 && randFloat() < .03) {
					placeObject(oGrassDry, 0, ix+randFloat(), iy+randFloat(), randFloat()*2*Math.PI);
				}
			}
			else if(grassNoise > .61) {
				t = (maxH - minH > 1.2) ? tGrassRock : tGrassShrubs;
			}
			else {
				if(maxH - minH < .5 && randFloat() < .05) {
					placeObject(oGrass, 0, ix+randFloat(), iy+randFloat(), randFloat()*2*Math.PI);
				}
			}
		}
		
		placeTerrain(ix, iy, t);
	}
}

println("Placing object groups...");

// create settlements
group = new SimpleGroup([new SimpleObject("special_settlement", 1,1, 0,0)], true, clSettlement);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 4, clPlayer, 25, clCliff, 4, clSettlement, 35),
	2 * NUM_PLAYERS, 50
);

// create straggler trees
trees = [oCarob, oBeech, oLombardyPoplar, oLombardyPoplar, oPine]
for(t in trees) {
	group = new SimpleGroup([new SimpleObject(trees[t], 1,1, 0,1)], true, clForest);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clCliff, 0, clForest, 1, clSettlement, 4, clPlayer, 15),
		SIZE*SIZE/7000, 50
	);
}

// create cypresses
group = new SimpleGroup([
	new SimpleObject("flora/trees/cypress2.xml", 1,3, 0,3), 
	new SimpleObject("flora/trees/cypress1.xml", 0,2, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 2, clForest, 1, clSettlement, 4, clPlayer, 15),
	SIZE*SIZE/3500, 50
);

// create bushes
group = new SimpleGroup([
	new SimpleObject(oBushSmall, 0,2, 0,2), 
	new SimpleObject(oBushSmallDry, 0,2, 0,2), 
	new SimpleObject(oBushMed, 0,1, 0,2), 
	new SimpleObject(oBushMedDry, 0,1, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 2),
	SIZE*SIZE/1800, 50
);

// create rocks
group = new SimpleGroup([
	new SimpleObject(oRockSmall, 0,3, 0,2), 
	new SimpleObject(oRockMed, 0,2, 0,2), 
	new SimpleObject(oRockLarge, 0,1, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clCliff, 0),
	SIZE*SIZE/1800, 50
);

// create stone
group = new SimpleGroup([new SimpleObject(oStone, 2,3, 0,2)], true, clStone);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clStone, 15, clSettlement, 4), 
	 new BorderTileClassConstraint(clCliff, 0, 5)],
	3 * NUM_PLAYERS, 100
);

// create metal
group = new SimpleGroup([new SimpleObject(oMetal, 2,3, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clMetal, 15, clStone, 5, clSettlement, 4), 
	 new BorderTileClassConstraint(clCliff, 0, 5)],
	3 * NUM_PLAYERS, 100
);

// create sheep
group = new SimpleGroup([new SimpleObject(oSheep, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, 
		clMetal, 2, clStone, 2, clFood, 8, clSettlement, 4),
	3 * NUM_PLAYERS, 100
);

// create berry bushes
group = new SimpleGroup([new SimpleObject(oBerryBush, 5,7, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, 
		clMetal, 2, clStone, 2, clFood, 8, clSettlement, 4),
	1.5 * NUM_PLAYERS, 100
);
