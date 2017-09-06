RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;

setSelectedBiome();

const tMainTerrain = rBiomeT1();
const tForestFloor1 = rBiomeT2();
const tForestFloor2 = rBiomeT3();
const tCliff = rBiomeT4();
const tTier1Terrain = rBiomeT5();
const tTier2Terrain = rBiomeT6();
const tTier3Terrain = rBiomeT7();
const tHill = rBiomeT8();
const tTier4Terrain = rBiomeT9();
const tRoad = rBiomeT10();
const tRoadWild = rBiomeT11();
const tTier5Terrain = rBiomeT12();
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
const oWood = "gaia/special_treasure_wood";

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

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clLand = createTileClass();

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
	var radius = scaleByMapSize(22,31);
	var shoreRadius = 4;
	var elevation = 3;

	var hillSize = PI * radius * radius;
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// create the main island
	var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 10)), floor(scaleByMapSize(25, 60)), 1, ix, iz, 0, [floor(radius)]);
	var terrainPainter = new LayeredPainter(
		[tMainTerrain , tMainTerrain, tMainTerrain],		// terrains
		[1, shoreRadius]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		elevation,				// elevation
		shoreRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clPlayer)], null);

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

	// create woods
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 13;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oWood, 14,14, 0,3)],
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

	// create starting trees
	var num = floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree1, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

log("Creating islands...");
placer = new ChainPlacer(floor(scaleByMapSize(4, 8)), floor(scaleByMapSize(8, 14)), floor(scaleByMapSize(25, 60)), 0.07, undefined, undefined, scaleByMapSize(30,70));
terrainPainter = new LayeredPainter(
	[tMainTerrain, tMainTerrain],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clLand)],
	null,
	scaleByMapSize(1, 5) * randIntInclusive(5, 10)
);

paintTerrainBasedOnHeight(2.4, 3.4, 3, tMainTerrain);
paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

//creating the city patches for the players
for (var i = 0; i < numPlayers; i++)
{
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
}

createBumps([avoidClasses(clPlayer, 10), stayClasses(clLand, 5)]);

if (randBool())
	createHills([tMainTerrain, tCliff, tHill], [avoidClasses(clPlayer, 2, clHill, 15), stayClasses(clLand, 0)], clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, [avoidClasses(clPlayer, 2, clHill, 15), stayClasses(clLand, 0)], clHill, scaleByMapSize(1, 4) * numPlayers);

createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 [avoidClasses(clPlayer, 20, clForest, 17, clHill, 0), stayClasses(clLand, 4)],
 clForest,
 1.0,
 currentBiome()
);
RMS.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 12), stayClasses(clLand, 7)]
);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 12), stayClasses(clLand, 7)]
);
RMS.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 7, clRock, 10, clHill, 1), stayClasses(clLand, 6)]
);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 7, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 6)],
 clMetal
);
RMS.SetProgress(65);

log("Creating decoration...");
var planetm = currentBiome() == g_BiomeTropic ? 8 : 1;
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
 [avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)]
);
RMS.SetProgress(70);

log("Creating animals...");
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
 [avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20), stayClasses(clLand, 3)]
);
RMS.SetProgress(75);

log("Creating fruits...");
createFood
(
 [
  [new SimpleObject(oFruitBush, 5,7, 0,4)]
 ],
 [
  3 * numPlayers
 ],
 [avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 10), stayClasses(clLand, 3)]
);
RMS.SetProgress(80);

log("Creating fish...");
createFood
(
 [
  [new SimpleObject(oFish, 2,3, 0,2)]
 ],
 [
  25 * numPlayers
 ],
 avoidClasses(clLand, 3, clPlayer, 2, clFood, 20)
);

RMS.SetProgress(85);

log("Creating straggler trees...");
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
createStragglerTrees(types, [avoidClasses(clForest, 7, clHill, 1, clPlayer, 3, clMetal, 6, clRock, 6), stayClasses(clLand, 7)]);
setWaterWaviness(4.0);
setWaterType("ocean");

ExportMap();
