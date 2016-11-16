RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;

//random terrain textures
var random_terrain = randomizeBiome();

const tMainTerrain = rBiomeT1();
const tForestFloor1 = rBiomeT2();
const tForestFloor2 = rBiomeT3();
const tCliff = rBiomeT4();
const tTier1Terrain = rBiomeT5();
const tTier2Terrain = rBiomeT6();
const tTier3Terrain = rBiomeT7();
const tHill = rBiomeT1();
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
const oMainHuntableAnimal = rBiomeE8();
const oFish = rBiomeE9();
const oSecondaryHuntableAnimal = rBiomeE10();
const oStoneLarge = rBiomeE11();
const oStoneSmall = rBiomeE12();
const oMetalLarge = rBiomeE13();

// decorative props
const aGrass = rBiomeA1();
const aGrassShort = rBiomeA2();
const aReeds = rBiomeA3();
const aLillies = rBiomeA4();
const aRockLarge = rBiomeA5();
const aRockMedium = rBiomeA6();
const aBushMedium = rBiomeA7();
const aBushSmall = rBiomeA8();

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

log(mapSize);

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
var clLand = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
	for (var iz = 0; iz < mapSize; iz++)
		placeTerrain(ix, iz, tMainTerrain);

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
var ix = round(fx);
var iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 8))));

var placer = new ClumpPlacer(mapArea * 0.23, 1, 1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater, tWater],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	-3,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
	playerIDs.push(i+1);

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
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id, { 'iberWall': 'towers' });

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oFruitBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);

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
	var tAngle = randFloat(0, TWO_PI);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree1, num, num, 0,3)],
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

var split = 1;
if (mapSize == 128 && numPlayers <= 2)
	split = 2;
else if (mapSize == 192 && numPlayers <= 3)
	split = 2;
else if (mapSize == 256)
{
	if (numPlayers <= 3)
		split = 3;
	else if (numPlayers == 4)
		split = 2;
}
else if (mapSize == 320)
{
	if (numPlayers <= 3)
		split = 3;
	else if (numPlayers == 4)
		split = 2;
}
else if (mapSize == 384)
{
	if (numPlayers <= 3)
		split = 4;
	else if (numPlayers == 4)
		split = 3;
	else if (numPlayers == 5)
		split = 2;
}
else if (mapSize == 448)
{
	if (numPlayers <= 2)
		split = 5;
	else if (numPlayers <= 4)
		split = 4;
	else if (numPlayers == 5)
		split = 3;
	else if (numPlayers == 6)
		split = 2;
}

log ("Creating rivers...");
for (var m = 0; m < numPlayers*split; m++)
{
	var tang = startAngle + (m+0.5)*TWO_PI/(numPlayers*split);
	var placer = new PathPlacer(fractionToTiles(0.5 + 0.15*cos(tang)), fractionToTiles(0.5 + 0.15*sin(tang)), fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)), scaleByMapSize(14,40), 0.0, 3*(scaleByMapSize(1,3)), 0.2, 0.05);
	var terrainPainter = new LayeredPainter(
		[tShore, tWater, tWater],		// terrains
		[1, 3]								// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-4,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));
	placer = new ClumpPlacer(floor(PI*scaleByMapSize(14,40)*scaleByMapSize(14,40)/4), 1, 0, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
	var painter = new LayeredPainter([tWater, tWater], [1]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 4);
	createArea(placer, [painter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));

	var tang = startAngle + (m)*TWO_PI/(numPlayers*split);
	var placer = new PathPlacer(fractionToTiles(0.5 + 0.05*cos(tang)), fractionToTiles(0.5 + 0.05*sin(tang)), fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)), scaleByMapSize(10,40), 0.0, 3*(scaleByMapSize(1,3)), 0.2, 0.05);
	var terrainPainter = new LayeredPainter(
		[tWater, tShore, tMainTerrain],		// terrains
		[1, 3]								// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter], null);
}


var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var placer = new ClumpPlacer(mapArea * 0.15, 1, 1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater, tWater],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	4,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter], null);

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 8))));

var placer = new ClumpPlacer(mapArea * 0.09, 1, 1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater],		// terrains
	[1, 3]								// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	-2,				// elevation
	3				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter], null);

var placer = new ClumpPlacer((mapSize - 50) * (mapSize - 50) * 0.09, 1, 1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater, tWater],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	4,				// elevation
	3				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, unPaintClass(clWater)], null);

var placer = new ClumpPlacer(scaleByMapSize(6, 18)*scaleByMapSize(6, 18)*22, 1, 1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tMainTerrain, tMainTerrain],		// terrains
	[1]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	20,				// elevation
	8				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter], null);

paintTerrainBasedOnHeight(-6, 1, 1, tWater);
paintTerrainBasedOnHeight(1, 2, 1, tShore);
paintTerrainBasedOnHeight(2, 5, 1, tMainTerrain);

paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);
unPaintTileClassBasedOnHeight(0.5, 10, 1, clWater);

for (var i = 0; i < numPlayers; i++)
{
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
}

if (randInt(1,2) == 1)
	createHills([tMainTerrain, tCliff, tHill], avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(1, 4) * numPlayers);

createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 2),
 clForest,
 1.0,
 random_terrain
);

RMS.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

RMS.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1)
);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
);

RMS.SetProgress(65);

// create decoration
var planetm = 1;

if (random_terrain == g_BiomeTropic)
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
  scaleByMapSize(16, 262),
  scaleByMapSize(8, 131),
  planetm * scaleByMapSize(13, 200),
  planetm * scaleByMapSize(13, 200),
  planetm * scaleByMapSize(13, 200)
 ],
 avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0)
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
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20)
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
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10)
);

// create straggler trees
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
createStragglerTrees(types, avoidClasses(clWater, 5, clForest, 7, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6));


// Export map data
ExportMap();
