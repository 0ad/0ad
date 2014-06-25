RMS.LoadLibrary("rmgen");

//random terrain textures
var random_terrain = randomizeBiome();

const tMainTerrain = rBiomeT1();
const tForestFloor1 = rBiomeT2();
const tForestFloor2 = rBiomeT3();
const tCliff = rBiomeT4();
const tTier1Terrain = rBiomeT5();
const tTier2Terrain = rBiomeT6();
const tTier3Terrain = rBiomeT7();
const tHill = rBiomeT8();
const tDirt = rBiomeT9();
const tRoad = rBiomeT10();
const tRoadWild = rBiomeT11();
const tTier4Terrain = rBiomeT12();
const tShoreBlend = rBiomeT13();
const tShore = rBiomeT14();
const tWater = rBiomeT15();

// gaia entities
const oTree1 = rBiomeE1();
const oTree2 = rBiomeE2();
const oTree3 = rBiomeE3();
const oTree4 = rBiomeE4();
const oTree5 = rBiomeE5();
const oFruitBush = rBiomeE6();
const oChicken = rBiomeE7();
const oMainHuntableAnimal = rBiomeE8();
const oFish = rBiomeE9();
const oSecondaryHuntableAnimal = rBiomeE10();
const oStoneLarge = rBiomeE11();
const oStoneSmall = rBiomeE12();
const oMetalLarge = rBiomeE13();
const oWood = "gaia/special_treasure_wood";
const oFood = "gaia/special_treasure_food_bin";

// decorative props
const aGrass = rBiomeA1();
const aGrassShort = rBiomeA2();
const aReeds = rBiomeA3();
const aLillies = rBiomeA4();
const aRockLarge = rBiomeA5();
const aRockMedium = rBiomeA6();
const aBushMedium = rBiomeA7();
const aBushSmall = rBiomeA8();
const aTree = rBiomeA9();

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];
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
			placeTerrain(ix, iz, tMainTerrain);
	}
}

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.065 * lSize, 0.7, 0.1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tMainTerrain, tMainTerrain],		// terrains
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
		[tMainTerrain, tMainTerrain],		// terrains
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
		[new SimpleObject(oFruitBush, 5,5, 0,3)],
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
		[new SimpleObject(oTree1, num, num, 0,4)],
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
	var painter = new LayeredPainter([tMainTerrain, tMainTerrain], [1]);
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
		[tMainTerrain, tMainTerrain],		// terrains
		[3]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		30,				// elevation
		3				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill2)], avoidClasses(clPlayer, 6, clHill2, 3, clHill, 2));
}

for (var g = 0; g < scaleByMapSize(5,30); g++)
{
	var tx = randInt(1, mapSize - 1);
	var tz = randInt(1, mapSize - 1);

	placer = new ClumpPlacer(mapArea * 0.01 * lSize, 0.7, 0.1, 10, tx, tz);
	terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain],		// terrains
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
			[tMainTerrain, tMainTerrain],		// terrains
			[3]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			3				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);
		
		if (numPlayers > 1)
		{
			var placer = new PathPlacer(tx, tz, mapSize*playerX[p2], mapSize*playerZ[p2], scaleByMapSize(11,17), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.1);
			var terrainPainter = new LayeredPainter(
				[tMainTerrain, tMainTerrain],		// terrains
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
		2				// blend radius
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
		2				// blend radius
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
createBumps([avoidClasses(clPlayer, 2), stayClasses(clLand, 2)]);

// create hills
createHills([tCliff, tCliff, tHill], [avoidClasses(clPlayer, 2, clHill, 8, clHill2, 8), stayClasses(clLand, 5)], clHill, scaleByMapSize(10, 40));

// create hills outside the canyon
createHills([tCliff, tCliff, tMainTerrain], avoidClasses(clLand, 1, clHill, 1), clHill, scaleByMapSize(20, 150), undefined, undefined, undefined, undefined, 40);

// create forests
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 [avoidClasses(clPlayer, 1, clForest, 15, clHill, 1, clHill2, 0), stayClasses(clLand, 4)], 
 clForest,
 1.0,
 random_terrain
);

RMS.SetProgress(50);

// create dirt patches
log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clHill2, 0), stayClasses(clLand, 3)]
);

// create grass patches
log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clHill2, 0), stayClasses(clLand, 3)]
);

log("Creating stone mines...");
// create stone quarries
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 3, clRock, 10, clHill, 1, clHill2, 1), stayClasses(clLand, 2)]
)

log("Creating metal mines...");
// create large metal quarries
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 3, clMetal, 10, clRock, 5, clHill, 1, clHill2, 1), stayClasses(clLand, 2)],
 clMetal
)

RMS.SetProgress(65);

// create decoration
var planetm = 1;

if (random_terrain==7)
	planetm = 8;

createDecoration
(
 [[new SimpleObject(aRockMedium, 1,3, 0,1)], 
  [new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
  [new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)],
  [new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)],
  [new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
 ],
 [
  3*scaleByMapSize(16, 262),
  3*scaleByMapSize(8, 131),
  planetm * scaleByMapSize(13, 200),
  planetm * scaleByMapSize(13, 200),
  planetm * scaleByMapSize(13, 200)
 ],
 avoidClasses(clForest, 0, clPlayer, 0, clHill, 0)
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

// create animals
createFood
(
 [
  [new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
  [new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)]
 ], 
 [
  3 * numPlayers,
  3 * numPlayers
 ],
 [avoidClasses(clWater, 0, clForest, 0, clPlayer, 4, clHill, 1, clFood, 20, clHill2, 1), stayClasses(clLand, 3)]
);

// create fruits
createFood
(
 [
  [new SimpleObject(oFruitBush, 5,7, 0,4)]
 ], 
 [
  3 * numPlayers
 ],
 [avoidClasses(clWater, 0, clForest, 0, clPlayer, 4, clHill, 1, clFood, 10, clHill2, 1), stayClasses(clLand, 3)]
);

RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
createStragglerTrees(types, [avoidClasses(clForest, 1, clHill, 1, clPlayer, 9, clMetal, 1, clRock, 1, clHill2, 1), stayClasses(clLand, 3)]);


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

// Export map data
ExportMap();