RMS.LoadLibrary("rmgen");


//random terrain textures
var rt = randomizeBiome();

var tGrass = rBiomeT1();
var tGrassPForest = rBiomeT2();
var tGrassDForest = rBiomeT3();
var tCliff = rBiomeT4();
var tGrassA = rBiomeT5();
var tGrassB = rBiomeT6();
var tGrassC = rBiomeT7();
var tHill = rBiomeT8();
var tDirt = rBiomeT9();
var tRoad = rBiomeT10();
var tRoadWild = rBiomeT11();
var tGrassPatch = rBiomeT12();
var tShoreBlend = rBiomeT13();
var tShore = rBiomeT14();
var tWater = rBiomeT15();

// gaia entities
var oOak = rBiomeE1();
var oOakLarge = rBiomeE2();
var oApple = rBiomeE3();
var oPine = rBiomeE4();
var oAleppoPine = rBiomeE5();
var oBerryBush = rBiomeE6();
var oChicken = rBiomeE7();
var oDeer = rBiomeE8();
var oFish = rBiomeE9();
var oSheep = rBiomeE10();
var oStoneLarge = rBiomeE11();
var oStoneSmall = rBiomeE12();
var oMetalLarge = rBiomeE13();
var oWood = "gaia/special_treasure_wood";
var oFood = "gaia/special_treasure_food_bin";

// decorative props
var aGrass = rBiomeA1();
var aGrassShort = rBiomeA2();
var aReeds = rBiomeA3();
var aLillies = rBiomeA4();
var aRockLarge = rBiomeA5();
var aRockMedium = rBiomeA6();
var aBushMedium = rBiomeA7();
var aBushSmall = rBiomeA8();
var aTree = rBiomeA9();

var pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oOak, tGrassDForest + TERRAIN_SEPARATOR + oOakLarge, tGrassDForest];
var pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oPine, tGrassPForest + TERRAIN_SEPARATOR + oAleppoPine, tGrassPForest];
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
var clHill2 = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clLand = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tGrass);
	}
}

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.065 * lSize, 0.7, 0.1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tGrass, tGrass],		// terrains
	[3]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	3,				// elevation
	3				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);

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

var startAngle = randFloat(0, TWO_PI);
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*TWO_PI/numPlayers;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(18,32);
	var cliffRadius = 2;
	var elevation = 20;
	var hillSize = PI * radius * radius;
	// get the x and z in tiles
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	// create the hill
	var placer = new ClumpPlacer(hillSize, 0.65, 0.1, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass],		// terrains
		[cliffRadius]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		cliffRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);

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
	var mDist = 11;
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
	var num = floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = 12;
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oOak, num, num, 0,4)],
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
	
	// create the city patch
	var cityRadius = radius/2;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, fractionToTiles(playerX[i]), fractionToTiles(playerZ[i]));
	var painter = new LayeredPainter([tGrass, tGrass], [1]);
	createArea(placer, [painter, paintClass(clPlayer)], null);
	
}

placer = new ClumpPlacer(150, 0.6, 0.3, 10, fractionToTiles(0.5), fractionToTiles(0.5));
var painter = new LayeredPainter([tRoad, tRoad], [1]);
createArea(placer, [painter, paintClass(clHill)], null);

for (var i = 0; i < scaleByMapSize(9,16); i++)
{
	var ix = randInt(1, mapSize - 1);
	var iz = randInt(1, mapSize - 1);
	var ix2 = randInt(1, mapSize - 1);
	var iz2 = randInt(1, mapSize - 1);
	var placer = new PathPlacer(ix, iz, ix2, iz2, scaleByMapSize(11,16), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0);
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass],		// terrains
		[3]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		30,				// elevation
		3				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill2)], avoidClasses(clPlayer, 6, clHill2, 3, clHill, 2));
}

for (var g = 0; g < scaleByMapSize(20,80); g++)
{
	var tx = randInt(1, mapSize - 1);
	var tz = randInt(1, mapSize - 1);

	placer = new ClumpPlacer(mapArea * 0.01 * lSize, 0.7, 0.1, 10, tx, tz);
	terrainPainter = new LayeredPainter(
		[tGrass, tGrass],		// terrains
		[3]		// widths
	);
	elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		3				// blend radius
	);
	var newarea = createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], avoidClasses(clLand, 6));
	if (newarea !== null)
	{
		var distances = new Array(0);
		var d1 = 9999;
		var d2 = 9999;
		var p1 = -1;
		var p2 = 0;
		for (var i = 0; i < numPlayers; i++)
		{
			distances.push(sqrt((tx-mapSize*playerX[i])*(tx-mapSize*playerX[i])+(tz-mapSize*playerZ[i])*(tz-mapSize*playerZ[i])));
		}
		for (var a = 0; a < numPlayers; a++)
		{
			if (d1 >= distances[a])
			{
				d2 = d1;
				d1 = distances[a];
				p2 = p1;
				p1 = a;
			}
			else if (d2 >= distances[a])
			{
				d2 = distances[a];
				p2 = a;
			}
		}

		var placer = new PathPlacer(tx, tz, mapSize*playerX[p1], mapSize*playerZ[p1], scaleByMapSize(11,17), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.1);
		var terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
			[3]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			3				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);
		
		var placer = new PathPlacer(tx, tz, mapSize*playerX[p2], mapSize*playerZ[p2], scaleByMapSize(11,17), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.1);
		var terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
			[3]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			3				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);
	}
}


for (var i = 0; i < numPlayers; i++)
{
	if (i+1 == numPlayers)
	{
		var placer = new PathPlacer(fractionToTiles(playerX[i]), fractionToTiles(playerZ[i]), fractionToTiles(playerX[0]), fractionToTiles(playerZ[0]), scaleByMapSize(8,13), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0);
	}
	else
	{
		var placer = new PathPlacer(fractionToTiles(playerX[i]), fractionToTiles(playerZ[i]), fractionToTiles(playerX[i+1]), fractionToTiles(playerZ[i+1]), scaleByMapSize(8,13), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0);
	}
	var terrainPainter = new LayeredPainter(
		[tRoadWild, tRoad],		// terrains
		[1]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		3				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand), paintClass(clHill)], null);
	
	var placer = new PathPlacer(fractionToTiles(playerX[i]), fractionToTiles(playerZ[i]), fractionToTiles(0.5), fractionToTiles(0.5), scaleByMapSize(8,13), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0);
	var terrainPainter = new LayeredPainter(
		[tRoadWild, tRoad],		// terrains
		[1]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		3				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand), paintClass(clHill)], null);
	
}

for (var i = 0; i < numPlayers; i++)
{
	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, fractionToTiles(playerX[i]), fractionToTiles(playerZ[i]));
	var painter = new LayeredPainter([tRoad, tRoad], [1]);
	createArea(placer, [painter, paintClass(clPlayer)], null);
}

placer = new ClumpPlacer(150, 0.6, 0.3, 10, fractionToTiles(0.5), fractionToTiles(0.5));
var painter = new LayeredPainter([tRoad, tRoad], [1]);
createArea(placer, [painter, paintClass(clHill)], null);

RMS.SetProgress(20);

paintTerrainBasedOnHeight(3.1, 29, 0, tCliff);
paintTileClassBasedOnHeight(3.1, 32, 0, clHill2);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	[avoidClasses(clWater, 2, clPlayer, 2), stayClasses(clLand, 2)],
	scaleByMapSize(100, 200)
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	[avoidClasses(clPlayer, 2, clHill, 8, clHill2, 8), stayClasses(clLand, 5)],
	scaleByMapSize(1, 4) * numPlayers
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, tGrass],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 40, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clLand, 1, clHill, 1),
	scaleByMapSize(20, 150)
);

// calculate desired number of trees for map (based on size)
if (rt == 6)
{
var MIN_TREES = 200;
var MAX_TREES = 1250;
var P_FOREST = 0.02;
}
else if (rt == 7)
{
var MIN_TREES = 1000;
var MAX_TREES = 6000;
var P_FOREST = 0.6;
}
else
{
var MIN_TREES = 500;
var MAX_TREES = 3000;
var P_FOREST = 0.7;
}
var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]],
	[[tGrassPForest, tGrass, pForestP], [tGrassPForest, pForestP]]
];	// some variation

if (rt == 6)
{
var size = numForest / (0.5 * scaleByMapSize(2,8) * numPlayers);
}
else
{
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
}
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
		[avoidClasses(clPlayer, 1, clForest, 5, clHill, 1, clHill2, 0), stayClasses(clLand, 4)],
		num
	);
}

RMS.SetProgress(50);
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
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clHill2, 0), stayClasses(clLand, 3)],
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clHill2, 0), stayClasses(clLand, 3)],
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(55);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 3, clRock, 10, clHill, 1, clHill2, 1), stayClasses(clLand, 2)],
	scaleByMapSize(6,24), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 3, clRock, 10, clHill, 1, clHill2, 1), stayClasses(clLand, 2)],
	scaleByMapSize(6,24), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 3, clMetal, 10, clRock, 5, clHill, 1, clHill2, 1), stayClasses(clLand, 2)],
	scaleByMapSize(6,24), 100
);

RMS.SetProgress(65);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	3*scaleByMapSize(16, 262), 50
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
	3*scaleByMapSize(8, 131), 50
);

// create actor trees
log("Creating actor trees...");
group = new SimpleGroup(
	[new SimpleObject(aTree, 1,1, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clLand, 5),
	scaleByMapSize(200, 800), 50
);

RMS.SetProgress(70);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 4, clHill, 1, clFood, 20, clHill2, 1), stayClasses(clLand, 3)],
	3 * numPlayers, 50
);

// create berries
log("Creating berries...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 4, clHill, 1, clFood, 20, clHill2, 1), stayClasses(clLand, 3)],
	3 * numPlayers, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 4, clHill, 1, clFood, 20, clHill2, 1), stayClasses(clLand, 3)],
	3 * numPlayers, 50
);

RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oOakLarge, oPine, oApple];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 9, clMetal, 1, clRock, 1, clHill2, 1), stayClasses(clLand, 3)],
		num
	);
}

var planetm = 1;
if (rt==7)
{
	planetm = 8;
}
//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200), 50
);

// create treasures
var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
for (var i = 0; i < randInt(3,8); i++)
{
	placeObject(fx+randFloat(-7,7), fz+randFloat(-7,7), oWood, 0, randFloat(0, TWO_PI))
}
for (var i = 0; i < randInt(3,8); i++)
{
	placeObject(fx+randFloat(-7,7), fz+randFloat(-7,7), oFood, 0, randFloat(0, TWO_PI))
}

rt = randInt(1,3)
if (rt==1){
setSkySet("cirrus");
}
else if (rt ==2){
setSkySet("cumulus");
}
else if (rt ==3){
setSkySet("sunny");
}
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterTint(0.447, 0.412, 0.322);				// muddy brown
setWaterReflectionTint(0.447, 0.412, 0.322);	// muddy brown
setWaterMurkiness(1.0);
setWaterReflectionTintStrength(0.677);

// Export map data
ExportMap();