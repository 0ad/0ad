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

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clLand = createTileClass();

log("Creating continent...");
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(5, 12)),
		Math.floor(scaleByMapSize(60, 700)),
		1,
		Math.round(fractionToTiles(0.5)),
		Math.round(fractionToTiles(0.5)),
		0,
		[Math.floor(mapSize * 0.33)]),
	[
		new SmoothElevationPainter(ELEVATION_SET, 3, 4),
		paintClass(clLand)
	],
	null);

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.25));

log("Ensuring initial player land...");
for (let i = 0; i < numPlayers; ++i)
	createArea(
		new ChainPlacer(
			2,
			Math.floor(scaleByMapSize(5, 9)),
			Math.floor(scaleByMapSize(5, 20)),
			1,
			playerPosition[i].x,
			playerPosition[i].y,
			0,
			[Math.floor(scaleByMapSize(23, 50))]),
		[
			new SmoothElevationPainter(ELEVATION_SET, 3, 4),
			paintClass(clLand)
		]);

Engine.SetProgress(20);

paintTerrainBasedOnHeight(3, 4, 3, tMainTerrain);
paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
	},
	"Berries": {
		"template": oFruitBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTree1,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(30);

createBumps([avoidClasses(clPlayer, 10), stayClasses(clLand, 5)]);

if (randBool())
	createHills([tMainTerrain, tCliff, tHill], [avoidClasses(clPlayer, 20, clHill, 15, clBaseResource, 3), stayClasses(clLand, 5)], clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, [avoidClasses(clPlayer, 20, clHill, 15, clBaseResource, 3), stayClasses(clLand, 5)], clHill, scaleByMapSize(1, 4) * numPlayers);

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 [avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clBaseResource,2), stayClasses(clLand, 4)],
 clForest,
 forestTrees);

Engine.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12), stayClasses(clLand, 5)],
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12), stayClasses(clLand, 5)],
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1), stayClasses(clLand, 6)],
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 6)],
 clMetal
);

Engine.SetProgress(65);

// create decoration
var planetm = 1;

if (currentBiome() == "tropic")
	planetm = 8;

createDecoration(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
		[new SimpleObject(aGrass, 2, 4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
		[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200)
	],
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)]);

Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)],
		[new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	[avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	clFood);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers
	],
	[avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10), stayClasses(clLand, 5)],
	clFood);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		25 * numPlayers
	],
	avoidClasses(clLand, 2, clFood, 20),
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 7, clHill, 1, clPlayer, 9, clMetal, 6, clRock, 6), stayClasses(clLand, 7)],
	clForest,
	stragglerTrees);

placePlayersNomad(
	clPlayer,
	[stayClasses(clLand, 4), avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2)]);

setWaterWaviness(1.0);
setWaterType("ocean");

ExportMap();
