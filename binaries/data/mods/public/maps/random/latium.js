RMS.LoadLibrary("rmgen");

const WATER_WIDTH = 0.1;

// terrain textures
const tOceanDepths = "medit_sea_depths";
const tOceanRockDeep = "medit_sea_coral_deep";
const tOceanRockShallow = "medit_rocks_wet";
const tOceanCoral = "medit_sea_coral_plants";
const tBeachWet = "medit_sand_wet";
const tBeachDry = "medit_sand";
const tBeachGrass = "medit_rocks_grass";
const tBeachCliff = "medit_dirt";
const tCity = "medit_city_tile";
const tGrassDry = ["medit_grass_field_brown", "medit_grass_field_dry", "medit_grass_field_b"];
const tGrass = ["medit_grass_field_dry", "medit_grass_field_brown", "medit_grass_field_b"];
const tGrassLush = ["grass_temperate_dry_tufts", "medit_grass_flowers"];
const tGrassShrubs = ["medit_grass_shrubs", "medit_grass_flowers"];
const tGrassRock = ["medit_rocks_grass"];
const tDirt = "medit_dirt";
const tDirtGrass = "medit_dirt_b";
const tDirtCliff = "medit_cliff_italia";
const tGrassCliff = "medit_cliff_italia_grass";
const tCliff = ["medit_cliff_italia", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tForestFloor = "medit_grass_wild";

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
const oChicken = "gaia/fauna_chicken";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

// decorative props
const aBushLargeDry = "actor|props/flora/bush_medit_la_dry.xml";
const aBushLarge = "actor|props/flora/bush_medit_la.xml";
const aBushMedDry = "actor|props/flora/bush_medit_me_dry.xml";
const aBushMed = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";
const aBushSmallDry = "actor|props/flora/bush_medit_sm_dry.xml";
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassDry = "actor|props/flora/grass_soft_dry_large_tall.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMed = "actor|geology/stone_granite_med.xml";
const aRockSmall = "actor|geology/stone_granite_small.xml";
const aWaterLog = "actor|props/flora/water_log.xml";

// terrain + entity (for painting)
const pPalmForest = [tForestFloor+TERRAIN_SEPARATOR+oPalm, tGrass];
const pPineForest = [tForestFloor+TERRAIN_SEPARATOR+oPine, tGrass];
const pPoplarForest = [tForestFloor+TERRAIN_SEPARATOR+oLombardyPoplar, tGrass];
const pMainForest = [tForestFloor+TERRAIN_SEPARATOR+oCarob, tForestFloor+TERRAIN_SEPARATOR+oBeech, tGrass, tGrass];

const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

// Create classes

var clWater = createTileClass();
var clCliff = createTileClass();
var clForest = createTileClass();
var clMetal = createTileClass();
var clStone = createTileClass();
var clFood = createTileClass();
var clPlayer = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = sortPlayers(playerIDs);

// Place players

log("Creating players...");

var playerX = new Array(numPlayers+1);
var playerZ = new Array(numPlayers+1);

var numLeftPlayers = ceil(numPlayers/2);
for (var i = 1; i <= numLeftPlayers; i++)
{
	playerX[i] = 0.28 + randFloat(-0.01, 0.01);
	playerZ[i] = (0.5+i-1)/numLeftPlayers + randFloat(-0.01, 0.01);
}
for (var i = numLeftPlayers+1; i <= numPlayers; i++)
{
	playerX[i] = 0.72 + randFloat(-0.01, 0.01);
	playerZ[i] = (0.5+i-numLeftPlayers-1)/numLeftPlayers + randFloat(-0.01, 0.01);
}

function distanceToPlayers(x, z)
{
	var r = 10000;
	for (var i = 1; i <= numPlayers; i++)
	{
		var dx = x - playerX[i];
		var dz = z - playerZ[i];
		r = min(r, dx*dx + dz*dz);
	}
	return sqrt(r);
}

function playerNearness(x, z)
{
	var d = fractionToTiles(distanceToPlayers(x,z));
	
	if (d < 13)
	{
		return 0;
	}
	else if (d < 19)
	{
		return (d-13)/(19-13);
	}
	else
	{
		return 1;
	}
}

// Paint elevation

log("Painting elevation...");

var noise0 = new Noise2D(scaleByMapSize(4, 16));
var noise1 = new Noise2D(scaleByMapSize(8, 32));
var noise2 = new Noise2D(scaleByMapSize(15, 60));

var noise2a = new Noise2D(scaleByMapSize(20, 80));
var noise2b = new Noise2D(scaleByMapSize(35, 140));

var noise3 = new Noise2D(scaleByMapSize(4, 16));
var noise4 = new Noise2D(scaleByMapSize(6, 24));
var noise5 = new Noise2D(scaleByMapSize(11, 44));

for (var ix = 0; ix <= mapSize; ix++)
{
	for (var iz = 0; iz <= mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		var pn = playerNearness(x, z);
		
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
		var baseNoise = 16*noise0.get(x,z) + 8*noise1.get(x,z) + 4*noise2.get(x,z) - (16+8+4)/2;
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
			h += (0.4*noise2a.get(x,z) + 0.2*noise2b.get(x,z)) * min(oldH/10.0, 1.0);
		}
		
		// create cliff noise
		if ( h > -10 )
		{
			var cliffNoise = (noise3.get(x,z) + 0.5*noise4.get(x,z)) / 1.5;
			if (h < 1)
			{
				var u = 1 - 0.3*((h-1)/-10);
				cliffNoise *= u;
			}
			cliffNoise += 0.05 * distToWater / (0.5 - WATER_WIDTH);
			if (cliffNoise > 0.6)
			{
				var u = 0.8 * (cliffNoise - 0.6);
				cliffNoise += u * noise5.get(x,z);
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
		setHeight(ix, iz, h);
	}
}

RMS.SetProgress(15);

// Paint base terrain

log("Painting terrain...");

var noise6 = new Noise2D(scaleByMapSize(10, 40));
var noise7 = new Noise2D(scaleByMapSize(20, 80));

var noise8 = new Noise2D(scaleByMapSize(13, 52));
var noise9 = new Noise2D(scaleByMapSize(26, 104));

var noise10 = new Noise2D(scaleByMapSize(50, 200));

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		var pn = playerNearness(x, z);
		
		// get heights of surrounding vertices
		var h00 = getHeight(ix, iz);
		var h01 = getHeight(ix, iz+1);
		var h10 = getHeight(ix+1, iz);
		var h11 = getHeight(ix+1, iz+1);
		
		// find min and max height
		var maxH = Math.max(h00, h01, h10, h11);
		var minH = Math.min(h00, h01, h10, h11);
		var diffH = maxH - minH;
		
		// figure out if we're at the top of a cliff using min adjacent height
		var minAdjHeight = minH;
		if (maxH > 15)
		{
			var maxNx = min(ix+2, mapSize);
			var maxNz = min(iz+2, mapSize);
			for (var nx = max(ix-1, 0); nx <= maxNx; nx++)
			{
				for (var nz = max(iz-1, 0); nz <= maxNz; nz++)
				{
					minAdjHeight = min(minAdjHeight, getHeight(nx, nz));
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
			addToClass(ix, iz, clWater);
		}
		
		// cliffs
		if (diffH > 2.9 && minH > -7)
		{
			t = tCliff;
			addToClass(ix, iz, clCliff);
		}
		else if ((diffH > 2.5 && minH > -5) || ((maxH - minAdjHeight) > 2.9 && minH > 0) )
		{
			if (minH < -1)
				t = tCliff;
			else if (minH < 0.5)
				t = tBeachCliff;
			else
				t = [tDirtCliff, tGrassCliff, tGrassCliff, tGrassRock, tCliff];
			
			addToClass(ix, iz, clCliff);
		}
		
		if (minH >= 7)
		{
			addToClass(ix, iz, clCliff);
		}
		
		// forests
		if (getHeight(ix, iz) <11){
			if (diffH < 2 && minH > 1)
			{
				var forestNoise = (noise6.get(x,z) + 0.5*noise7.get(x,z)) / 1.5 * pn - 0.59;
				
				// Thin out trees a bit
				if (forestNoise > 0 && randFloat() < 0.5)
				{
					if (minH < 11 && minH >= 4)
					{
						var typeNoise = noise10.get(x,z);
						
						if (typeNoise < 0.43 && forestNoise < 0.05)
							t = pPoplarForest;
						else if (typeNoise < 0.63)
							t = pMainForest;
						else
							t = pPineForest;
						
						addToClass(ix, iz, clForest);
					}
					else if (minH < 4)
					{
						t = pPalmForest;
						addToClass(ix, iz, clForest);
					}
				}
			}
		}
		// grass variations
		if (t == tGrass)
		{
			var grassNoise = (noise8.get(x,z) + 0.6*noise9.get(x,z)) / 1.6;
			if (grassNoise < 0.3)
			{
				t = (diffH > 1.2) ? tDirtCliff : tDirt;
			}
			else if (grassNoise < 0.34)
			{
				t = (diffH > 1.2) ? tGrassCliff : tGrassDry;
				if (diffH < 0.5 && randFloat() < 0.02)
				{
					placeObject(ix+randFloat(), iz+randFloat(), aGrassDry, 0, randFloat(0, TWO_PI));
				}
			}
			else if (grassNoise > 0.61)
			{
				t = (diffH > 1.2 ? tGrassRock : tGrassShrubs);
			}
			else
			{
				if (diffH < 0.5 && randFloat() < 0.02)
				{
					placeObject(ix+randFloat(), iz+randFloat(), aGrass, 0, randFloat(0, TWO_PI));
				}
			}
		}
		
		placeTerrain(ix, iz, t);
	}
}

RMS.SetProgress(30);

for (var i = 1; i <= numPlayers; i++)
{
	var id = playerIDs[i-1];
	log("Creating base for player " + id + "...");
	
	// get fractional locations in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	addToClass(ix, iz, clPlayer);
	
	// create the city patch, flatten area under TC
	var cityRadius = 11;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tGrass, tCity], [4]);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,	// type
		5,				// elevation
		2				// blend radius
	);
	createArea(placer, [painter, elevationPainter], null);
	
	// create starting units
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE);
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(oChicken, 5,5, 0,2)],
			true, clBaseResource, aX, aZ
		);
		createObjectGroup(group, 0);
	}
	
	// create starting berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 9;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,2)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	var radius = scaleByMapSize(15,25);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = 5;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(10, 11);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oPalm, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

RMS.SetProgress(40);

log("Creating bushes...");
// create bushes
group = new SimpleGroup(
	[new SimpleObject(aBushSmall, 0,2, 0,2), new SimpleObject(aBushSmallDry, 0,2, 0,2), 
	new SimpleObject(aBushMed, 0,1, 0,2), new SimpleObject(aBushMedDry, 0,1, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 2),
	scaleByMapSize(9, 146), 50
);

RMS.SetProgress(45);

log("Creating rocks...");
// create rocks
group = new SimpleGroup(
	[new SimpleObject(aRockSmall, 0,3, 0,2), new SimpleObject(aRockMed, 0,2, 0,2), 
	new SimpleObject(aRockLarge, 0,1, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clCliff, 1),
	scaleByMapSize(9, 146), 50
);

RMS.SetProgress(50);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clStone);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clPlayer, 20, clStone, 15, clCliff, 3)],
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clStone);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clStone, 15, clCliff, 3)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clMetal, 15, clStone, 5, clCliff, 3), 
	 borderClasses(clCliff, 0, 5)],
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(60);

log("Creating straggler trees...");
// create straggler trees
var trees = [oCarob, oBeech, oLombardyPoplar, oLombardyPoplar, oPine];
for (var t in trees)
{
	group = new SimpleGroup([new SimpleObject(trees[t], 1,1, 0,1)], true, clForest);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clCliff, 4, clForest, 1, clPlayer, 15, clMetal, 1, clStone, 1),
		scaleByMapSize(2, 38), 50
	);
}

RMS.SetProgress(70);

// create straggler cypresses
group = new SimpleGroup(
	[new SimpleObject(oCypress2, 1,3, 0,3), new SimpleObject(oCypress1, 0,2, 0,2)],
	true
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 4, clCliff, 4, clForest, 1, clPlayer, 15, clMetal, 1, clStone, 1),
	scaleByMapSize(5, 75), 50
);

RMS.SetProgress(80);

log("Creating sheep...");
// create sheep
group = new SimpleGroup([new SimpleObject(oSheep, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, clMetal, 2, clStone, 2, clFood, 8),
	3 * numPlayers, 50
);

RMS.SetProgress(85);

log("Creating fish...");
// create fish
var num = scaleByMapSize(4, 16);
var offsetX = mapSize * WATER_WIDTH/2;
for (var i = 0; i < num; ++i)
{
	var cX = round(offsetX + offsetX/2 * randFloat(-1, 1));
	var cY = round((i + 0.5) * mapSize/num);
	group = new SimpleGroup([new SimpleObject(oFish, 1,1, 0,1)], true, clFood, cX, cY);
	createObjectGroup(group, 0);
}

for (var i = 0; i < num; ++i)
{
	var cX = round(mapSize - offsetX + offsetX/2 * randFloat(-1, 1));
	var cY = round((i + 0.5) * mapSize/num);
	group = new SimpleGroup([new SimpleObject(oFish, 1,1, 0,1)], true, clFood, cX, cY);
	createObjectGroup(group, 0);
}

RMS.SetProgress(90);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, clMetal, 2, clStone, 2, clFood, 8),
	3 * numPlayers, 50
);

RMS.SetProgress(95);

log("Creating berry bushes...");
// create berry bushes
group = new SimpleGroup([new SimpleObject(oBerryBush, 5,7, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clCliff, 1, clPlayer, 20, clMetal, 2, clStone, 2, clFood, 8),
	1.5 * numPlayers, 100
);

// Adjust environment
setSkySet("sunny");
setWaterColour(0.024,0.212,0.024);
setWaterTint(0.133, 0.725,0.855);
setWaterWaviness(3);
setWaterMurkiness(0.8);

// Export map data
ExportMap();
