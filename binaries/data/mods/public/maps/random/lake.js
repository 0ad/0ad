Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.hill;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tTier4Terrain = g_Terrains.tier4Terrain;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

initTerrain(tMainTerrain);

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

log("Preventing water in player territory...");
for (let i = 0; i < numPlayers; ++i)
	addCivicCenterAreaToClass(
		Math.round(fractionToTiles(playerX[i])),
		Math.round(fractionToTiles(playerZ[i])),
		clPlayer);

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	placeCivDefaultEntities(fx, fz, id);

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

	// create starting trees
	var num = 5;
	var tAngle = randFloat(0, TWO_PI);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree1, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

log("Creating the lake...")
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(5, 16)),
		Math.floor(scaleByMapSize(35, 200)),
		1,
		Math.round(fractionToTiles(0.5)),
		Math.round(fractionToTiles(0.5)),
		0,
		[Math.floor(mapSize * 0.17 * Math.pow(scaleByMapSize(1, 6), 1/8))]),
	[
		new SmoothElevationPainter(ELEVATION_SET, -3, 4),
		paintClass(clWater)
	],
	avoidClasses(clPlayer, 20));

log("Creating more shore jaggedness...");
createAreas(
	new ChainPlacer(2, Math.floor(scaleByMapSize(4, 6)), 3, 1),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		unPaintClass(clWater)
	],
	borderClasses(clWater, 4, 7),
	scaleByMapSize(12, 130) * 2,
	150);

paintTerrainBasedOnHeight(2.4, 3.4, 3, tMainTerrain);
paintTerrainBasedOnHeight(1, 2.4, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
paintTileClassBasedOnHeight(-6, 0, 1, clWater);

for (var i = 0; i < numPlayers; ++i)
{
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
}

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

if (randBool())
	createHills([tMainTerrain, tCliff, tHill], avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(1, 4) * numPlayers);

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 2),
 clForest,
 forestTrees);

Engine.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
);

Engine.SetProgress(65);

var planetm = 1;

if (currentBiome() == "tropic")
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

Engine.SetProgress(70);

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

Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	avoidClasses(clWater, 5, clForest, 7, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

setWaterWaviness(4.0);
setWaterType("lake");

ExportMap();
