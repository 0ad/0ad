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
tDirt = "dirt_medit_a";
tDirtGrass = "dirt_medit_grass_50";
tDirtCliff = ["cliff_medit_dirt", "cliff_medit_dirt", "cliff_medit_grass_a"];
tCliff = ["cliff_medit_face_a", "cliff_medit_foliage_a"];
tForestFloor = "forestfloor_medit_rock";

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
oGrass = "props/flora/grass_medit_field.xml";
oGrassDry = "props/flora/grass_soft_dry_large_tall.xml";
oWaterLog = "props/flora/water_log.xml";

tPalmForest = [tForestFloor+"|"+oPalm, tForestFloor, tGrass];
tPineForest = [tForestFloor+"|"+oPine, tForestFloor, tForestFloor, tForestFloor];
tMainForest = [tForestFloor+"|"+oCarob, tForestFloor+"|"+oBeech, tForestFloor];

// Initialize world

init(SIZE, tGrass, 0);

// Create classes

clWater = createTileClass();
clCliff = createTileClass();
clForest = createTileClass();
clMetal = createTileClass();
clStone = createTileClass();
clFood = createTileClass();

// Paint elevation

noise0 = new Noise2D(4 * SIZE/128);
noise1 = new Noise2D(9 * SIZE/128);
noise2 = new Noise2D(17 * SIZE/128);

noise3 = new Noise2D(4 * SIZE/128);
noise4 = new Noise2D(8 * SIZE/128);
noise5 = new Noise2D(16 * SIZE/128);

for(ix=0; ix<=SIZE; ix++) {
	for(iy=0; iy<=SIZE; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		
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
			h = 15*u;
		}
		
		// add some base noise
		baseNoise = 14*noise0.eval(x,y) + 8*noise1.eval(x,y) + 4*noise2.eval(x,y) - (14+8+4)/2;
		if( distToWater>0 && h+baseNoise < 1 ) {
			baseNoise += Math.min(h, distToWater*30);
		}
		h += baseNoise;
		
		// create land noise
		if( h > -7 )
		{
			cliffNoise = (1*noise3.eval(x,y) + 0.5*noise4.eval(x,y)) / 1.5;
			if(h < -3) {
				cliffNoise *= 0.3 * (1 - ((h+3)/-4));
			}
			if(cliffNoise > .6) {
				u = 0.7 * (cliffNoise-.6);
				cliffNoise += u * noise5.eval(x,y);
				cliffNoise /= (1+u);
			}
			cliffNoise += .1 * distToWater / (0.5 - WATER_WIDTH);
			cliffNoise -= 0.58;
			if(cliffNoise > 0) {
				h += 19 * Math.min(cliffNoise, 0.06) / 0.06;
			}
		}
		
		// set the height
		setHeight(ix, iy, h);
	}
}

// Paint base terrain

noise6 = new Noise2D(10 * SIZE/128);
noise7 = new Noise2D(20 * SIZE/128);

noise8 = new Noise2D(30 * SIZE/128);
noise9 = new Noise2D(50 * SIZE/128);

for(ix=0; ix<SIZE; ix++) {
	for(iy=0; iy<SIZE; iy++) {
		x = ix / (SIZE + 1.0);
		y = iy / (SIZE + 1.0);
		
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
		
		// cliffs
		if(maxH - minH > 3) {
			t = tCliff;
		}
		else if(maxH - minH > 2.4) {
			t = tDirtCliff;
			if(minH < .3) t = tBeachCliff;
		}
		
		// forests
		if(maxH - minH < 1 && minH > 1) {
			forestNoise = (noise6.eval(x,y) + 0.5*noise7.eval(x,y)) / 1.5;
			forestNoise -= 0.6;
			if(forestNoise > 0) {
				if(minH > 20 && t==tGrass && forestNoise > .01) {
					t = tPineForest;
				}
				else if(minH > 4 && minH < 19 && t==tGrass) {
					t = tMainForest;
				}
				else if(minH < 3) {
					t = tPalmForest;
				}
			}
		}
		
		// grass variations
		if(t==tGrass)
		{
			grassNoise = (noise8.eval(x,y) + .5*noise9.eval(x,y)) / 1.5;
			if(grassNoise < .4) {
				t = tGrassDry;
			}
			else if(grassNoise > .6) {
				t = tGrassShrubs;
			}
		}
		
		placeTerrain(ix, iy, t);
	}
}
