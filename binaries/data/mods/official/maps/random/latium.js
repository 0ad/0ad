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

// Place players

println("Placing players...");

playerX = new Array(NUM_PLAYERS+1);
playerY = new Array(NUM_PLAYERS+1);

numLeftPlayers = Math.floor(NUM_PLAYERS/2);
for(i=0; i<numLeftPlayers; i++) {
	playerX[i] = 0.28 + (2*randFloat()-1)*0.01;
	playerY[i] = (0.5+i)/numLeftPlayers + (2*randFloat()-1)*0.01;
}
for(i=numLeftPlayers; i<NUM_PLAYERS; i++) {
	playerX[i] = 0.72 + (2*randFloat()-1)*0.01;
	playerY[i] = (0.5+i-numLeftPlayers)/numLeftPlayers + (2*randFloat()-1)*0.01;
}

for(i=0; i<NUM_PLAYERS; i++) {
	// get fractional locations in tiles
	ix = round(fractionToTiles(playerX[i]));
	iy = round(fractionToTiles(playerY[i]));
	addToClass(ix, iy, clPlayer);
	
	// create the TC and the starting villagers
	group = new SimpleGroup(
		[							// elements (type, count, distance)
			new SimpleObject("hele_civil_centre", 1,1, 0,0),
			new SimpleObject("hele_infantry_javelinist_b", 3,3, 5,5)
		],
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
	for(var i=0; i<NUM_PLAYERS; i++) {
		var dx = x-playerX[i];
		var dy = y-playerY[i];
		r = Math.min(r, dx*dx + dy*dy);
	}
	return Math.sqrt(r);
}

function playerNearness(x, y) {
	var d = fractionToTiles(distanceToPlayers(x,y));
	if(d < 13) return 0;
	else if(d < 19) return (d-13)/(19-13);
	else return 1;
}

// Paint elevation

println("Painting elevation...");

noise0 = new Noise2D(4 * SIZE/128);
noise1 = new Noise2D(9 * SIZE/128);
noise2 = new Noise2D(15 * SIZE/128);

noise3 = new Noise2D(4 * SIZE/128);
noise4 = new Noise2D(7 * SIZE/128);
noise5 = new Noise2D(11 * SIZE/128);

for(ix=0; ix<=SIZE; ix++) {
	for(iy=0; iy<=SIZE; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		pn = playerNearness(x, y);
		
		h = 0;
		distToWater = 0;
		
		// add the rough shape of the water
		if(x < WATER_WIDTH) {
			h = Math.max(-15, -30*(WATER_WIDTH-x)/WATER_WIDTH);
		}
		else if(x > 1-WATER_WIDTH) {
			h = Math.max(-15, -30*(x-(1-WATER_WIDTH))/WATER_WIDTH);
		}
		else {
			distToWater = (0.5 - WATER_WIDTH - Math.abs(x-0.5));
			u = 1 - Math.abs(x-0.5)/(0.5-WATER_WIDTH);
			h = 12*u;
		}
		
		// add some base noise
		baseNoise = 14*noise0.eval(x,y) + 7*noise1.eval(x,y) + 4*noise2.eval(x,y) - (14+7+4)/2;
		if( baseNoise < 0 ) {
			baseNoise *= pn;
		}
		if( distToWater>0 && h+baseNoise < 1 ) {
			baseNoise += Math.min(h, distToWater*50);
		}
		h += baseNoise;
		
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
				u = 0.5 * (cliffNoise-.6);
				cliffNoise += u * noise5.eval(x,y);
				cliffNoise /= (1+u);
			}
			cliffNoise -= 0.58;
			cliffNoise *= pn;
			if(cliffNoise > 0) {
				h += 20 * Math.min(cliffNoise, 0.045) / 0.045;
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

noise8 = new Noise2D(20 * SIZE/128);
noise9 = new Noise2D(40 * SIZE/128);

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
		maxH = Math.max(h00, h01, h10, h11);
		minH = Math.min(h00, h01, h10, h11);
		
		// choose a terrain based on elevation
		t = tGrass;
		
		// water
		if(maxH < -11) {
			t = tOceanDepths;
		}
		else if(maxH < -8) {
			t = tOceanRockDeep;
		}
		else if(maxH < -5) {
			t = tOceanCoral;
		}
		else if(maxH < -3) {
			t = tOceanRockShallow;
		}
		else if(maxH < .5 && minH < -.3) {
			t = tBeachWet;
		}
		else if(maxH < .9 && minH < .4) {
			t = tBeachDry;
		}
		else if(maxH < 2.1 && minH < .8) {
			t = tBeachGrass;
		}
		
		if(minH < 0) {
			addToClass(ix, iy, clWater);
		}
		
		// cliffs
		if(maxH - minH > 3) {
			t = tCliff;
			addToClass(ix, iy, clCliff);
		}
		else if(maxH - minH > 2.4) {
			if(minH < -1) t = tCliff;
			else if(minH < .5) t = tBeachCliff;
			else t = [tDirtCliff, tGrassCliff];
			addToClass(ix, iy, clCliff);
		}
		
		// forests
		if(maxH - minH < 1 && minH > 1) {
			forestNoise = (noise6.eval(x,y) + 0.5*noise7.eval(x,y)) / 1.5 * pn;
			forestNoise -= 0.6;
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
			if(grassNoise < .35) {
				t = (maxH - minH > 1.2) ? tDirtCliff : tDirt;
			}
			else if(grassNoise < .39) {
				t = (maxH - minH > 1.2) ? tGrassCliff : tGrassDry;
				if(maxH - minH < .5 && randFloat() < .03) {
					placeObject(oGrassDry, 0, ix+randFloat(), iy+randFloat(), randFloat()*2*Math.PI);
				}
			}
			else if(grassNoise > .64) {
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

// create straggler trees
for(t in [oCarob, oBeech, oLombardyPoplar, oPine]) {
	group = new SimpleGroup([new SimpleObject(t, 1,3, 0,2)], true, clForest);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clCliff, 0, clForest, 1),
		SIZE*SIZE/8000, 50
	);
}

// create bushes
group = new SimpleGroup([
	new SimpleObject(oBushSmall, 0,2, 0,2), 
	new SimpleObject(oBushSmallDry, 0,2, 0,2), 
	new SimpleObject(oBushMed, 0,1, 0,2), 
	new SimpleObject(oBushMedDry, 0,1, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 2),
	SIZE*SIZE/2000, 50
);

// create rocks
group = new SimpleGroup([
	new SimpleObject(oRockSmall, 0,3, 0,2), 
	new SimpleObject(oRockMed, 0,2, 0,2), 
	new SimpleObject(oRockLarge, 0,1, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clCliff, 0),
	SIZE*SIZE/2000, 50
);

// create stone
group = new SimpleGroup([new SimpleObject(oStone, 2,3, 0,2)], true, clStone);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clStone, 15), 
	 new BorderTileClassConstraint(clCliff, 0, 5)],
	3 * NUM_PLAYERS, 100
);

// create metal
group = new SimpleGroup([new SimpleObject(oMetal, 2,3, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clMetal, 15, clStone, 5), 
	 new BorderTileClassConstraint(clCliff, 0, 5)],
	3 * NUM_PLAYERS, 100
);

// create sheep
group = new SimpleGroup([new SimpleObject(oSheep, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, clMetal, 2, clStone, 2, clFood, 8),
	5 * NUM_PLAYERS, 100
);