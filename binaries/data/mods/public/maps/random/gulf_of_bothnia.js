RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;
var rt = randInt(1,3);
if (rt == 1)
{
	var tGrass = ["alpine_dirt_grass_50"];
	var tGrassPForest = "alpine_forrestfloor";
	var tGrassDForest = "alpine_forrestfloor";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
	var tGrassA = "alpine_grass_rocky";
	var tGrassB = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tGrassC = ["alpine_snow_a", "alpine_snow_b"];
	var tDirt = ["alpine_dirt", "alpine_grass_d"];
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_grass_50";
	var tWater = "alpine_shore_rocks";

	// gaia entities
	var oPine = "gaia/flora_tree_pine";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oGoat = "gaia/fauna_goat";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
}
else if (rt == 2)
{
	var tGrass = ["alpine_snow_a", "alpine_snow_b"];
	var tGrassPForest = "alpine_forrestfloor_snow";
	var tGrassDForest = "alpine_forrestfloor_snow";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b"];
	var tGrassA = "alpine_grass_snow_50";
	var tGrassB = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tGrassC = ["alpine_snow_rocky"];
	var tDirt = ["alpine_dirt_snow", "alpine_snow_a"];
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_icy";
	var tWater = "alpine_shore_rocks";

	// gaia entities
	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oGoat = "gaia/fauna_goat";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
}
else
{
	var tGrass = ["alpine_snow_a", "alpine_snow_b"];
	var tGrassPForest = "alpine_forrestfloor_snow";
	var tGrassDForest = "alpine_forrestfloor_snow";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b"];
	var tGrassA = "alpine_grass_snow_50";
	var tGrassB = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tGrassC = ["alpine_snow_rocky"];
	var tDirt = ["alpine_dirt_snow", "alpine_snow_a"];
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "polar_ice_snow";
	var tWater = "polar_ice_b";

	// gaia entities
	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oGoat = "gaia/fauna_goat";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
}

// decorative props
var aGrass = "actor|props/flora/grass_soft_small_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_large.xml";
var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
var aLillies = "actor|props/flora/water_lillies.xml";
var aBushMedium = "actor|props/flora/bush_medit_me.xml";
var aBushSmall = "actor|props/flora/bush_medit_sm.xml";

var pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oPine, tGrassDForest];
var pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oPine, tGrassPForest];
const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

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
var clPass = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tGrass);
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
			[new SimpleObject(oChicken, 5,5, 0,3)],
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

if (rt == 3)
{
var seaHeight = 0;
}
else
{
var seaHeight = -3;
}

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.08 * lSize, 0.7, 0.05, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tGrass, tGrass, tGrass, tGrass],		// terrains
	[1, 4, 2]		// widths
);

var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)));

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.3);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.13 * lSize, 0.7, 0.05, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tGrass, tGrass, tGrass, tGrass],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)));

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.0);
ix = round(fx);
iz = round(fz)+1;

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.17 * lSize, 0.7, 0.05, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tGrass, tGrass, tGrass, tGrass],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	0				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)+4));
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	seaHeight,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)));


// create islands
log("Creating islands...");
placer = new ClumpPlacer(floor(hillSize*randFloat(0.05,0.3)), 0.80, 0.1, 10);
terrainPainter = new LayeredPainter(
	[tGrass, tGrass],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
createAreas(
	placer,
	[terrainPainter, elevationPainter], 
	borderClasses(clWater, 0, 1),
	scaleByMapSize(2, 5)*randInt(8,14), 30
);

if (rt == 3)
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
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 10),
	scaleByMapSize(100, 200)
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tGrass, tCliff, tGrass],		// terrains
	[1, 2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 12, clHill, 15, clWater, 0, clPass, 1),
	scaleByMapSize(1, 4) * numPlayers
);


// calculate desired number of trees for map (based on size)

var MIN_TREES = 600;
var MAX_TREES = 3600;
var P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]],
	[[tGrassPForest, tGrass, pForestP], [tGrassPForest, pForestP]]
];	// some variation



var size = numForest / (scaleByMapSize(2,8) * numPlayers);

var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 12, clForest, 8, clHill, 0, clWater, 2, clPass, 1),
		num
	);
}

RMS.SetProgress(60);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassB);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(65);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(70);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);


// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(75);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create rabbit
log("Creating rabbit...");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

if (rt !== 3)
{
	// create fish
	log("Creating fish...");
	group = new SimpleGroup(
		[new SimpleObject(oFish, 2,3, 0,2)],
		true, clFood
	);
	createObjectGroups(group, 0,
		[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
		11 * numPlayers, 60
	);
}
RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oPine, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 3, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}

if (rt !== 3)
{
	//create small grass tufts
	log("Creating small grass tufts...");
	var planetm = 1;

	group = new SimpleGroup(
		[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
		planetm * scaleByMapSize(13, 200)
	);

	RMS.SetProgress(90);

	// create large grass tufts
	log("Creating large grass tufts...");
	group = new SimpleGroup(
		[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
		planetm * scaleByMapSize(13, 200)
	);

	RMS.SetProgress(95);

	// create bushes
	log("Creating bushes...");
	group = new SimpleGroup(
		[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 2, clHill, 1, clPlayer, 1, clDirt, 1),
		planetm * scaleByMapSize(13, 200), 50
	);
}



setSkySet("stormy");
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterTint(0.447, 0.412, 0.322);				// muddy brown
setWaterReflectionTint(0.447, 0.412, 0.322);	// muddy brown
setWaterMurkiness(1.0);
setWaterReflectionTintStrength(0.677);

// Export map data

ExportMap();