RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;
var random_terrain = randInt(1,3);
if (random_terrain == 1)
{
	setFogThickness(0.26);
	setFogFactor(0.4);

	setPPEffect("hdr");
	setPPSaturation(0.48);
	setPPContrast(0.53);
	setPPBloom(0.12);

	var tPrimary = ["alpine_dirt_grass_50"];
	var tForestFloor = "alpine_forrestfloor";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
	var tSecondary = "alpine_grass_rocky";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_rocky"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_grass_50";
	var tWater = "alpine_shore_rocks";

	// gaia entities
	var oPine = "gaia/flora_tree_pine";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
	
	// decorative props
	var aGrass = "actor|props/flora/grass_soft_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm.xml";
}
else if (random_terrain == 2)
{
	setFogFactor(0.35);
	setFogThickness(0.19);
	setPPSaturation(0.37);
	setPPEffect("hdr");

	var tPrimary = ["alpine_snow_a", "alpine_snow_b"];
	var tForestFloor = "alpine_forrestfloor_snow";
	var tCliff = ["alpine_cliff_snow"];
	var tSecondary = "alpine_grass_snow_50";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_a", "alpine_snow_b"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_icy";
	var tWater = "alpine_shore_rocks";

	// gaia entities
	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
	
	// decorative props
	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}
else
{
	setFogFactor(0.41);
	setFogThickness(0.23);
	setPPSaturation(0.34);
	setPPEffect("hdr");

	var tPrimary = ["alpine_snow_a", "alpine_snow_b"];
	var tForestFloor = "alpine_snow_a";
	var tCliff = ["alpine_cliff_snow"];
	var tSecondary = "polar_ice_snow";
	var tHalfSnow = ["polar_ice_cracked"];
	var tSnowLimited = ["alpine_snow_a", "alpine_snow_b"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "polar_ice_snow";
	var tWater = ["polar_ice_snow", "polar_ice"];

	// gaia entities
	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
	
	// decorative props
	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];
const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tPrimary);
	}
}

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = sortPlayers(playerIDs);

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = -PI/6;
for (var i = 0; i < numPlayers; i++)
{
	if (numPlayers == 1)
		playerAngle[i] = startAngle + TWO_PI/3;
	else
		playerAngle[i] = startAngle + i*TWO_PI/(numPlayers-1)*2/3;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
	
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
	
	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
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
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oPine, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
	// create grass tufts
	var num = hillSize / 250;
	for (var j = 0; j < num; j++)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = radius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aGrassShort, 2,5, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gZ
		);
		createObjectGroup(group, 0);
	}
}

RMS.SetProgress(20);

//create the gulf

if (random_terrain == 3)
{
	var seaHeight = 0;
}
else
{
	var seaHeight = -3;
}

//create the upper part

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = 1;

var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 200)), 1, ix, iz, 0, [floor(mapSize * 0.17 * lSize)]);
var terrainPainter = new LayeredPainter(
	[tPrimary, tPrimary, tPrimary, tPrimary],		// terrains
	[1, 4, 2]		// widths
);

var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(20,28)));

//the middle part

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.3);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 120)), 1, ix, iz, 0, [floor(mapSize * 0.18 * lSize)]);
var terrainPainter = new LayeredPainter(
	[tPrimary, tPrimary, tPrimary, tPrimary],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(20,28)));

//the lower part

var fx = fractionToTiles(0.5);
var fz = 0;
ix = round(fx);
iz = round(fz)+1;

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 100)), 1, ix, iz, 0, [floor(mapSize * 0.19 * lSize)]);
var terrainPainter = new LayeredPainter(
	[tPrimary, tPrimary, tPrimary, tPrimary],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(20,28)));

if (random_terrain == 3)
{
	paintTerrainBasedOnHeight(2, 3, 0, tShore);
	paintTerrainBasedOnHeight(-1, 2, 2, tWater);
}
else
{
	paintTerrainBasedOnHeight(1, 3, 0, tShore);
	paintTerrainBasedOnHeight(-8, 1, 2, tWater);
}

// create bumps
createBumps(avoidClasses(clWater, 2, clPlayer, 10));

// create hills
if (randInt(1,2) == 1)
	createHills([tPrimary, tCliff, tPrimary], avoidClasses(clPlayer, 20, clHill, 15, clWater, 0), clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 0), clHill, scaleByMapSize(1, 4) * numPlayers);

// create forests
createForests(
 [tPrimary, tForestFloor, tForestFloor, pForest, pForest],
 avoidClasses(clPlayer, 20, clForest, 16, clHill, 0, clWater, 2), 
 clForest,
 1.0,
 random_terrain
);

RMS.SetProgress(60);

// create dirt patches
log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tPrimary,tSecondary],[tSecondary,tHalfSnow], [tHalfSnow,tSnowLimited]],
 [1,1],
 avoidClasses(clWater, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

// create grass patches
log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tHalfSnow,
 avoidClasses(clWater, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

RMS.SetProgress(65);

log("Creating stone mines...");
// create stone quarries
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1)
)

log("Creating metal mines...");
// create large metal quarries
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
)

RMS.SetProgress(70);

// create decoration
var multiplier = 0;
if (random_terrain !== 3) multiplier = 1;

createDecoration
(
 [[new SimpleObject(aRockMedium, 1,3, 0,1)], 
  [new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
  [new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)],
  [new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)],
  [new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
 ],
 [
  scaleByMapSize(16, 262),
  scaleByMapSize(8, 131),
  multiplier * scaleByMapSize(13, 200),
  multiplier * scaleByMapSize(13, 200),
  multiplier * scaleByMapSize(13, 200)
 ],
 avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0)
);


RMS.SetProgress(75);

// create animals
createFood
(
 [
  [new SimpleObject(oDeer, 5,7, 0,4)],
  [new SimpleObject(oRabbit, 2,3, 0,2)]
 ], 
 [
  3 * numPlayers,
  3 * numPlayers
 ],
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20)
);

// create fruits
createFood
(
 [
  [new SimpleObject(oBerryBush, 5,7, 0,4)]
 ], 
 [
  randInt(1, 4) * numPlayers + 2
 ],
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10)
);

if (random_terrain !== 3)
{
	// create fish
	createFood
	(
	 [
	  [new SimpleObject(oFish, 2,3, 0,2)]
	 ], 
	 [
	  25 * numPlayers
	 ],
	 [avoidClasses(clFood, 20), stayClasses(clWater, 6)]
	);
}

RMS.SetProgress(85);

// create straggler trees
log("Creating straggler trees...");
var types = [oPine];
createStragglerTrees(types, avoidClasses(clWater, 3, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1));

setSkySet("stormy");
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 6, PI / 4));

setWaterColour(0.035,0.098,0.314);
setWaterWaviness(5.0);
setWaterType("lake");
setWaterMurkiness(0.88);

// Export map data

ExportMap();