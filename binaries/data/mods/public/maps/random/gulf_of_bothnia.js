Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");

TILE_CENTERED_HEIGHT_MAP = true;

if (g_MapSettings.Biome)
	setSelectedBiome();
else
	// TODO: Replace ugly default for atlas by a dropdown
	setBiome("gulf_of_bothnia/late_spring");

const tPrimary = g_Terrains.mainTerrain;
const tForestFloor = g_Terrains.forestFloor1;
const tCliff = g_Terrains.cliff;
const tSecondary = g_Terrains.tier1Terrain;
const tHalfSnow = g_Terrains.tier2Terrain;
const tSnowLimited = g_Terrains.tier3Terrain;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oPine = g_Gaia.tree1;
const oBerryBush = g_Gaia.fruitBush;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oDeer = g_Gaia.mainHuntableAnimal;
const oRabbit = g_Gaia.secondaryHuntableAnimal;
const oFish = g_Gaia.fish;

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const heightSeaGround = g_Heights.seaGround;
const heightShore = g_Heights.shore;
const heightLand = g_Heights.land;

const fishCount = g_ResourceCounts.fish;
const bushCount = g_ResourceCounts.bush;

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];

var g_Map = new RandomMap(heightLand, tPrimary);

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

var startAngle = randomAngle();

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), ...playerPlacementCustomAngle(
			fractionToTiles(0.35),
			mapCenter,
			i => startAngle + 1/3 * Math.PI * (1 + 2 * (numPlayers == 1 ? 1 : 2 * i / (numPlayers - 1))))],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
	},
	"Berries": {
		"template": oBerryBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oPine,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

g_Map.log("Creating the gulf");
var gulfLakePositions = [
	{ "numCircles": 200, "x": fractionToTiles(0), "radius": fractionToTiles(0.175) },
	{ "numCircles": 120, "x": fractionToTiles(0.3), "radius": fractionToTiles(0.2) },
	{ "numCircles": 100, "x": fractionToTiles(0.5), "radius": fractionToTiles(0.225) }
];

for (let gulfLake of gulfLakePositions)
{
	let position = Vector2D.add(mapCenter, new Vector2D(gulfLake.x, 0).rotate(-startAngle)).round();

	createArea(
		new ChainPlacer(
			2,
			Math.floor(scaleByMapSize(5, 16)),
			Math.floor(scaleByMapSize(35, gulfLake.numCircles)),
			Infinity,
			position,
			0,
			[Math.floor(gulfLake.radius)]),
		[
			new LayeredPainter([tPrimary, tPrimary, tPrimary, tPrimary], [1, 4, 2]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer,scaleByMapSize(20, 28)));
}

paintTerrainBasedOnHeight(heightShore, heightLand, Elevation_ExcludeMin_ExcludeMax, tShore);
paintTerrainBasedOnHeight(-Infinity, heightShore, Elevation_ExcludeMin_IncludeMax, tWater);

createBumps(avoidClasses(clWater, 2, clPlayer, 10));

if (randBool())
	createHills([tPrimary, tCliff, tPrimary], avoidClasses(clPlayer, 20, clHill, 15, clWater, 0), clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 0), clHill, scaleByMapSize(1, 4) * numPlayers);

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
 [tPrimary, tForestFloor, tForestFloor, pForest, pForest],
 avoidClasses(clPlayer, 20, clForest, 16, clHill, 0, clWater, 2),
 clForest,
 forestTrees);

Engine.SetProgress(60);

g_Map.log("Creating dirt patches");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tPrimary,tSecondary],[tSecondary,tHalfSnow], [tHalfSnow,tSnowLimited]],
 [1,1],
 avoidClasses(clWater, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);

g_Map.log("Creating grass patches");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tHalfSnow,
 avoidClasses(clWater, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(65);

g_Map.log("Creating stone mines");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
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
Engine.SetProgress(70);

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
		scaleByMapSize(bushCount.min, bushCount.max),
		scaleByMapSize(bushCount.min, bushCount.max),
		scaleByMapSize(bushCount.min, bushCount.max)
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 5, clHill, 0, clBaseResource, 5));
Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oDeer, 5, 7, 0, 4)],
		[new SimpleObject(oRabbit, 2, 3, 0, 2)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	clFood);

createFood(
	[[new SimpleObject(oBerryBush, 5, 7, 0, 4)]],
	[randIntInclusive(1, 4) * numPlayers + 2],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);

createFood(
	[[new SimpleObject(oFish, 2, 3, 0, 2)]],
	[scaleByMapSize(fishCount.min, fishCount.max)],
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oPine],
	avoidClasses(clWater, 3, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

// Avoid the lake, even if frozen
placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSunElevation(Math.PI * randFloat(1/6, 1/4));

g_Map.ExportMap();
