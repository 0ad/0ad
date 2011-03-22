RMS.LoadLibrary("rmgen");

const WATER_WIDTH = 0.2;

// terrain textures
const tOceanDepths = "medit_sea_depths";
const tOceanRockDeep = "medit_sea_coral_deep";
const tOceanRockShallow = "medit_rocks_wet";
const tOceanCoral = "medit_sea_coral_plants";
const tBeachWet = "medit_sand_wet";
const tBeachDry = "medit_sand";
const tBeachGrass = "beach_medit_grass_50";
const tBeachCliff = "cliff_medit_beach";
const tGrassDry = ["medit_grass_field_brown", "medit_grass_field_dry", "medit_grass_field_b"];
const tGrass = ["medit_grass_field", "medit_grass_field_a", "medit_grass_flowers"];
const tGrassLush = ["grass_temperate_dry_tufts", "medit_grass_flowers"];
const tGrassShrubs = ["medit_grass_shrubs", "medit_grass_flowers"];
const tGrassRock = ["medit_rocks_grass"];
const tDirt = "medit_dirt";
const tDirtGrass = "medit_dirt_b";
const tDirtCliff = "medit_cliff_italia";
const tGrassCliff = "medit_cliff_italia_grass";
const tCliff = ["medit_cliff_italia", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tForestFloor = "forestfloor_medit_dirt";

// gaia entities
const oBeech = "gaia/flora_tree_euro_beech";
const oBerryBush = "gaia/flora_bush_berry";
const oCarob = "gaia/flora_tree_carob";
const oCypress1 = "gaia/flora_tree_cypress";
const oCypress2 = "gaia/flora_tree_cypress";
const oLombardyPoplar = "gaia/flora_tree_poplar_lombardy";
const oOak = "gaia/flora_tree_oak";
const oPalm = "gaia/flora_tree_medit_fan_palm";
const oPine = "gaia/flora_tree_aleppo_pine";
const oPoplar = "gaia/flora_tree_poplar";
const oSheep = "gaia/fauna_sheep";
const oStone = "gaia/geology_stone_greek";
const oMetal = "gaia/geology_metal_greek";

// decorative props
const aBushLargeDry = "actor|props/flora/bush_medit_la_dry.xml";
const aBushLarge = "actor|props/flora/bush_medit_la.xml";
const aBushMedDry = "actor|props/flora/bush_medit_me_dry.xml";
const aBushMed = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";
const aBushSmallDry = "actor|props/flora/bush_medit_sm_dry.xml";
const aGrass = "actor|props/flora/grass_medit_field.xml";
const aGrassDry = "actor|props/flora/grass_soft_dry_small.xml";
const aRockLarge = "actor|geology/stone_granite_greek_large.xml";
const aRockMed = "actor|geology/stone_granite_greek_med.xml";
const aRockSmall = "actor|geology/stone_granite_greek_small.xml";
const aWaterLog = "actor|props/flora/water_log.xml";

// terrain + entity (for painting)
var pPalmForest = tForestFloor+TERRAIN_SEPARATOR+oPalm;
var pPineForest = tForestFloor+TERRAIN_SEPARATOR+oPine;
var pCarobForest = tForestFloor+TERRAIN_SEPARATOR+oCarob;
var pBeechForest = tForestFloor+TERRAIN_SEPARATOR+oBeech;
var pPoplarForest = tForestFloor+TERRAIN_SEPARATOR+oLombardyPoplar;
var tPalmForest = [pPalmForest, tGrass];
var tPineForest = [pPineForest, tGrass];
var tMainForest = [pCarobForest, pBeechForest, tGrass, tGrass];
var tPoplarForest = [pPoplarForest, tGrass];

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

// Create classes

var clWater = createTileClass();
var clCliff = createTileClass();
var clForest = createTileClass();
var clMetal = createTileClass();
var clStone = createTileClass();
var clFood = createTileClass();
var clPlayer = createTileClass();
var clBaseResource = createTileClass();

// Place players

log("Placing players...");

var playerX = new Array(numPlayers+1);
var playerY = new Array(numPlayers+1);

var numLeftPlayers = floor(numPlayers/2);
for (var i=1; i <= numLeftPlayers; i++)
{
	playerX[i] = 0.28 + (2*randFloat()-1)*0.01;
	playerY[i] = (0.5+i-1)/numLeftPlayers + (2*randFloat()-1)*0.01;
}
for (var i=numLeftPlayers+1; i <= numPlayers; i++)
{
	playerX[i] = 0.72 + (2*randFloat()-1)*0.01;
	playerY[i] = (0.5+i-numLeftPlayers-1)/numLeftPlayers + (2*randFloat()-1)*0.01;
}

for (var i=1; i <= numPlayers; i++)
{
	log("Creating base for player " + i + "...");
	
	// get fractional locations in tiles
	var ix = round(fractionToTiles(playerX[i]));
	var iy = round(fractionToTiles(playerY[i]));
	addToClass(ix, iy, clPlayer);
	
	// create TC and starting units
	// TODO: Get civ specific starting units
	var civ = getCivCode(i - 1);
	placeObject("structures/"+civ + "_civil_centre", i, ix, iy, PI*3/4);
	var group = new SimpleGroup(
		[new SimpleObject("units/"+civ+"_support_female_citizen", 3,3, 5,5)],
		true, null,	ix, iy
	);
	createObjectGroup(group, i);
	
	// create starting berry bushes
	var bbAngle = randFloat()*2*PI;
	var bbDist = 9;
	var bbX = round(ix + bbDist * cos(bbAngle));
	var bbY = round(iy + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,2)],
		true, clBaseResource, bbX, bbY
	);
	createObjectGroup(group, 0);
	
	// create starting mines
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3) {
		mAngle = randFloat()*2*PI;
	}
	var mDist = 9;
	var mX = round(ix + mDist * cos(mAngle));
	var mY = round(iy + mDist * sin(mAngle));
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

function distanceToPlayers(x, y)
{
	var r = 10000;
	for (var i=1; i <= numPlayers; i++)
	{
		var dx = x - playerX[i];
		var dy = y - playerY[i];
		r = min(r, dx*dx + dy*dy);
	}
	return sqrt(r);
}

function playerNearness(x, y)
{
	var d = fractionToTiles(distanceToPlayers(x,y));
	
	if (d < 13)
		return 0;
	else if (d < 19)
		return (d-13)/(19-13);
	else
		return 1;
}

// Paint elevation

log("Painting elevation...");

var noise0 = new Noise2D(4 * mapSize/128);
var noise1 = new Noise2D(8 * mapSize/128);
var noise2 = new Noise2D(15 * mapSize/128);

var noise2a = new Noise2D(20 * mapSize/128);
var noise2b = new Noise2D(35 * mapSize/128);

var noise3 = new Noise2D(4 * mapSize/128);
var noise4 = new Noise2D(6 * mapSize/128);
var noise5 = new Noise2D(11 * mapSize/128);

for (var ix=0; ix<=mapSize; ix++)
{
	for (var iy=0; iy<=mapSize; iy++)
	{
		var x = ix / (mapSize + 1.0);
		var y = iy / (mapSize + 1.0);
		var pn = playerNearness(x, y);
		
		var h = 0;
		var distToWater = 0;
		
		h = 32 * (x - 0.5);
		
		// add the rough shape of the water
		if (x < WATER_WIDTH)
		{
			h = max(-16.0, -28.0*(WATER_WIDTH-x)/WATER_WIDTH);
		}
		else if (x > 1.0-WATER_WIDTH)
		{
			h = max(-16.0, -28.0*(x-(1.0-WATER_WIDTH))/WATER_WIDTH);
		}
		else
		{
			distToWater = (0.5 - WATER_WIDTH - abs(x-0.5));
			var u = 1 - abs(x-0.5) / (0.5-WATER_WIDTH);
			h = 12*u;
		}
		
		// add some base noise
		var baseNoise = 16*noise0.get(x,y) + 8*noise1.get(x,y) + 4*noise2.get(x,y) - (16+8+4)/2;
		if ( baseNoise < 0 ) 
		{
			baseNoise *= pn;
			baseNoise *= max(0.1, distToWater / (0.5-WATER_WIDTH));
		}
		var oldH = h;
		h += baseNoise;
		
		// add some higher-frequency noise on land
		if ( oldH > 0 )
		{
			h += (0.4*noise2a.get(x,y) + 0.2*noise2b.get(x,y)) * min(oldH/10.0, 1.0);
		}
		
		// create cliff noise
		if ( h > -10 )
		{
			var cliffNoise = (noise3.get(x,y) + 0.5*noise4.get(x,y)) / 1.5;
			if (h < 1)
			{
				var u = 1 - 0.3*((h-1)/-10);
				cliffNoise *= u;
			}
			cliffNoise += 0.05 * distToWater / (0.5 - WATER_WIDTH);
			if (cliffNoise > 0.6)
			{
				var u = 0.8 * (cliffNoise - 0.6);
				cliffNoise += u * noise5.get(x,y);
				cliffNoise /= (1 + u);
			}
			cliffNoise -= 0.59;
			cliffNoise *= pn;
			if (cliffNoise > 0)
			{
				h += 19 * min(cliffNoise, 0.045) / 0.045;
			}
		}
		
		// set the height
		setHeight(ix, iy, h);
	}
}

// Paint base terrain

log("Painting terrain...");

var noise6 = new Noise2D(10 * mapSize/128);
var noise7 = new Noise2D(20 * mapSize/128);

var noise8 = new Noise2D(13 * mapSize/128);
var noise9 = new Noise2D(26 * mapSize/128);

var noise10 = new Noise2D(50 * mapSize/128);

for (var ix=0; ix<mapSize; ix++)
{
	for (var iy=0; iy<mapSize; iy++)
	{
		var x = ix / (mapSize + 1.0);
		var y = iy / (mapSize + 1.0);
		var pn = playerNearness(x, y);
		
		// get heights of surrounding vertices
		var h00 = getHeight(ix, iy);
		var h01 = getHeight(ix, iy+1);
		var h10 = getHeight(ix+1, iy);
		var h11 = getHeight(ix+1, iy+1);
		
		// find min and max height
		var maxH = Math.max(h00, h01, h10, h11);
		var minH = Math.min(h00, h01, h10, h11);
		
		// figure out if we're at the top of a cliff using min adjacent height
		var minAdjHeight = minH;
		if (maxH > 15)
		{
			var maxNx = min(ix+2, mapSize);
			var maxNy = min(iy+2, mapSize);
			for (var nx=max(ix-1, 0); nx <= maxNx; nx++)
			{
				for (var ny=max(iy-1, 0); ny <= maxNy; ny++)
				{
					minAdjHeight = min(minAdjHeight, getHeight(nx, ny));
				}
			}
		}
		
		// choose a terrain based on elevation
		var t = tGrass;
		
		// water
		if (maxH < -12)
		{
			t = tOceanDepths;
		}
		else if (maxH < -8.8)
		{
			t = tOceanRockDeep;
		}
		else if (maxH < -4.7)
		{
			t = tOceanCoral;
		}
		else if (maxH < -2.8)
		{
			t = tOceanRockShallow;
		}
		else if (maxH < 0.9 && minH < 0.35)
		{
			t = tBeachWet;
		}
		else if (maxH < 1.5 && minH < 0.9)
		{
			t = tBeachDry;
		}
		else if (maxH < 2.3 && minH < 1.3)
		{
			t = tBeachGrass;
		}
		
		if (minH < 0)
		{
			addToClass(ix, iy, clWater);
		}
		
		// cliffs
		if (maxH - minH > 2.9 && minH > -7)
		{
			t = tCliff;
			addToClass(ix, iy, clCliff);
		}
		else if ((maxH - minH > 2.5 && minH > -5) || (maxH-minAdjHeight > 2.9 && minH > 0) )
		{
			if (minH < -1)
				t = tCliff;
			else if (minH < 0.5)
				t = tBeachCliff;
			else
				t = [tDirtCliff, tGrassCliff, tGrassCliff, tGrassRock, tCliff];
			
			addToClass(ix, iy, clCliff);
		}
		
		// forests
		if (maxH - minH < 1 && minH > 1)
		{
			var forestNoise = (noise6.get(x,y) + 0.5*noise7.get(x,y)) / 1.5 * pn;
			forestNoise -= 0.59;
			
			if (forestNoise > 0)
			{
				if (minH > 5)
				{
					var typeNoise = noise10.get(x,y);
					
					if (typeNoise < 0.43 && forestNoise < 0.05)
						t = tPoplarForest;
					else if (typeNoise < 0.63)
						t = tMainForest;
					else
						t = tPineForest;
					
					addToClass(ix, iy, clForest);
				}
				else if (minH < 3)
				{
					t = tPalmForest;
					addToClass(ix, iy, clForest);
				}
			}
		}
		
		// grass variations
		if (t == tGrass)
		{
			var grassNoise = (noise8.get(x,y) + 0.6*noise9.get(x,y)) / 1.6;
			if (grassNoise < 0.3)
			{
				t = (maxH - minH > 1.2) ? tDirtCliff : tDirt;
			}
			else if (grassNoise < 0.34)
			{
				t = (maxH - minH > 1.2) ? tGrassCliff : tGrassDry;
				if (maxH - minH < 0.5 && randFloat() < 0.03)
				{
					placeObject(aGrassDry, 0, ix+randFloat(), iy+randFloat(), randFloat()*2*PI);
				}
			}
			else if (grassNoise > 0.61)
			{
				t = ((maxH - minH) > 1.2 ? tGrassRock : tGrassShrubs);
			}
			else
			{
				if ((maxH - minH) < 0.5 && randFloat() < 0.05)
				{
					placeObject(aGrass, 0, ix+randFloat(), iy+randFloat(), randFloat()*2*PI);
				}
			}
		}
		
		placeTerrain(ix, iy, t);
	}
}

log("Placing straggler trees...");
// create straggler trees
var trees = [oCarob, oBeech, oLombardyPoplar, oLombardyPoplar, oPine];
for (var t in trees)
{
	group = new SimpleGroup([new SimpleObject(trees[t], 1,1, 0,1)], true, clForest);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clCliff, 0, clForest, 1, clPlayer, 15),
		mapSize*mapSize/7000, 50
	);
}

log("Placing cypress trees...");
// create cypresses
group = new SimpleGroup(
	[new SimpleObject(oCypress2, 1,3, 0,3),
	new SimpleObject(oCypress1, 0,2, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 2, clForest, 1, clPlayer, 15),
	mapSize*mapSize/3500, 50
);

log("Placing bushes...");
// create bushes
group = new SimpleGroup(
	[new SimpleObject(aBushSmall, 0,2, 0,2), new SimpleObject(aBushSmallDry, 0,2, 0,2), 
	new SimpleObject(aBushMed, 0,1, 0,2), new SimpleObject(aBushMedDry, 0,1, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 2),
	mapSize*mapSize/1800, 50
);

log("Placing rocks...");
// create rocks
group = new SimpleGroup(
	[new SimpleObject(aRockSmall, 0,3, 0,2), new SimpleObject(aRockMed, 0,2, 0,2), 
	new SimpleObject(aRockLarge, 0,1, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clCliff, 0),
	mapSize*mapSize/1800, 50
);

log("Placing stone mines...");
// create stone
group = new SimpleGroup([new SimpleObject(oStone, 2,3, 0,2)], true, clStone);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clStone, 15), 
	 new BorderTileClassConstraint(clCliff, 0, 5)],
	3 * numPlayers, 100
);

log("Placing metal mines...");
// create metal
group = new SimpleGroup([new SimpleObject(oMetal, 2,3, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clMetal, 15, clStone, 5), 
	 new BorderTileClassConstraint(clCliff, 0, 5)],
	3 * numPlayers, 100
);

log("Placing sheep...");
// create sheep
group = new SimpleGroup([new SimpleObject(oSheep, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, clMetal, 2, clStone, 2, clFood, 8),
	3 * numPlayers, 100
);

log("Placing berry bushes...");
// create berry bushes
group = new SimpleGroup([new SimpleObject(oBerryBush, 5,7, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, clMetal, 2, clStone, 2, clFood, 8),
	1.5 * numPlayers, 100
);

// Export map data
ExportMap();
