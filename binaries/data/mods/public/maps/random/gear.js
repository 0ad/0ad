Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");

TILE_CENTERED_HEIGHT_MAP = true;

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.mainTerrain;
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

var heightSeaGround = -4;
var heightShallow = -2;
var heightLand = 3;
var heightRing = 4;
var heightHill = 20;

var g_Map = new RandomMap(heightLand, tMainTerrain);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

var radiusPlayers = fractionToTiles(0.35);
var radiusCentralLake = fractionToTiles(0.27);
var radiusCentralRingLand = fractionToTiles(0.21);
var radiusCentralWaterRing = fractionToTiles(0.17);
var radiusCentralIsland = fractionToTiles(0.14);
var radiusCentralHill = fractionToTiles(0.12);

var [playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(radiusPlayers);

g_Map.log("Determining number of rivers between players");
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

g_Map.log("Creating big circular lake");
createArea(
	new DiskPlacer(radiusCentralLake, mapCenter),
	new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4));

g_Map.log("Creating rivers between players");
for (let m = 0; m < numPlayers * split; ++m)
{
	let angle = startAngle + (m + 0.5) * 2 * Math.PI / (numPlayers * split);
	let position1 = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.15), 0).rotate(-angle));
	let position2 = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.6), 0).rotate(-angle));
	createArea(
		new PathPlacer(position1, position2, scaleByMapSize(14, 40), 0, scaleByMapSize(3, 9), 0.2, 0.05),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
		avoidClasses(clPlayer, 5));
}

g_Map.log("Create path from the island to the center");
for (let m = 0; m < numPlayers * split; ++m)
{
	let angle = startAngle + m * 2 * Math.PI / (numPlayers * split);
	let position1 = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.05), 0).rotate(-angle));
	let position2 = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.49), 0).rotate(-angle));
	createArea(
		new PathPlacer(position1, position2, scaleByMapSize(10, 40), 0, scaleByMapSize(3, 9), 0.2, 0.05),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 4));
}

g_Map.log("Creating ring of land connecting players");
createArea(
	new DiskPlacer(radiusCentralRingLand, mapCenter),
	new SmoothElevationPainter(ELEVATION_SET, heightRing, 4));

g_Map.log("Creating inner ring of water");
createArea(
	new DiskPlacer(radiusCentralWaterRing, mapCenter),
	new SmoothElevationPainter(ELEVATION_SET, heightShallow, 3));

g_Map.log("Creating central island");
createArea(
	new DiskPlacer(radiusCentralIsland, mapCenter),
	new SmoothElevationPainter(ELEVATION_SET, heightRing, 3));

g_Map.log("Creating hill on the central island");
createArea(
	new DiskPlacer(radiusCentralHill, mapCenter),
	new SmoothElevationPainter(ELEVATION_SET, heightHill, 8));

paintTerrainBasedOnHeight(-6, 1, 1, tWater);
paintTerrainBasedOnHeight(1, 2, 1, tShore);
paintTerrainBasedOnHeight(2, 21, 1, tMainTerrain);

paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": avoidClasses(clWater, 2),
	"Walls": "towers",
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

g_Map.log("Creating dirt patches");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);

g_Map.log("Creating grass patches");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(55);

g_Map.log("Creating stone mines");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
 clRock);

g_Map.log("Creating metal mines");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
);

g_Map.log("Creating fish");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oFish, 1,1, 0,3)], true, clFood),
	0,
	[stayClasses(clWater, 8), avoidClasses(clFood, 14)],
	scaleByMapSize(400, 2000),
	100);

Engine.SetProgress(65);

var planetm = 1;

if (currentBiome() == "generic/tropic")
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
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0));

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
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	clFood);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	avoidClasses(clWater, 5, clForest, 7, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

g_Map.ExportMap();
