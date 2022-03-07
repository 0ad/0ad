Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

TILE_CENTERED_HEIGHT_MAP = true;

if (g_MapSettings.Biome)
	setSelectedBiome();
else
	// TODO: Replace ugly default for atlas by a dropdown
	setBiome("gulf_of_bothnia/winter");

const isLakeFrozen = g_Environment.Water.Frozen;

const tPrimary = g_Terrains.mainTerrain;
const tForestFloor = g_Terrains.forestFloor1;
const tCliff = g_Terrains.cliff;
const tSecondary = g_Terrains.tier1Terrain;
const tPatch1 = g_Terrains.tier2Terrain;
const tPatch2 = g_Terrains.tier3Terrain;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oBerryBush = g_Gaia.fruitBush;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oMetalSmall = g_Gaia.metalSmall;
const oMainHunt = g_Gaia.mainHuntableAnimal;
const oSecHunt = g_Gaia.secondaryHuntableAnimal;
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
const huntCount = g_ResourceCounts.hunt;
const berriesCount = g_ResourceCounts.berries;
const bushCount = g_ResourceCounts.bush;


const pForest1 = [
	tForestFloor + TERRAIN_SEPARATOR + oTree1,
	tForestFloor + TERRAIN_SEPARATOR + oTree2, tForestFloor];

const pForest2 = [
	tForestFloor + TERRAIN_SEPARATOR + oTree4,
	tForestFloor + TERRAIN_SEPARATOR + oTree5, tForestFloor];

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clLake = g_Map.createTileClass();
var clWater = isLakeFrozen ? g_Map.createTileClass() : clLake;
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
	"StartingAnimal": {
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
		"template": oTree1,
		"count": 2
	},
	"Decoratives": {
		"template": aRockMedium
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
			new TerrainPainter(tPrimary),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			new TileClassPainter(clLake)
		],
		avoidClasses(clPlayer,scaleByMapSize(20, 28)));
}

if (isLakeFrozen)
{
	const areas = createAreas(
		new ChainPlacer(
			1,
			4,
			scaleByMapSize(16, 40),
			0.3),
		[
			new ElevationPainter(-6),
			new TileClassPainter(clWater)
		],
		stayClasses(clLake, 2),
		scaleByMapSize(10, 40));

	createObjectGroupsByAreas(
		new SimpleGroup([new SimpleObject(oFish, 1, 2, 0, 2)], true, clFood),
		0,
		stayClasses(clWater, 2),
		scaleByMapSize(fishCount.min, fishCount.max),
		20,
		areas);
}

paintTerrainBasedOnHeight(heightShore, heightLand, Elevation_ExcludeMin_ExcludeMax, tShore);
paintTerrainBasedOnHeight(-Infinity, heightShore, Elevation_ExcludeMin_IncludeMax, tWater);

createBumps(avoidClasses(clLake, 2, clPlayer, 10));

if (randBool())
	createHills(
		[tPrimary, tCliff, tCliff],
		avoidClasses(clPlayer, 20, clHill, 15, clLake, 0),
		clHill,
		scaleByMapSize(1, 4) * numPlayers,
		undefined,
		undefined,
		undefined,
		0.5,
		18,
		4);
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clLake, 0), clHill, scaleByMapSize(1, 4) * numPlayers, Math.floor(scaleByMapSize(20, 40)));

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createDefaultForests(
	[tForestFloor, tForestFloor, tForestFloor, pForest1, pForest2],
	avoidClasses(clPlayer, 20, clForest, 16, clHill, 0, clLake, 2),
	clForest,
	forestTrees);

Engine.SetProgress(60);

g_Map.log("Creating dirt patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tPrimary, tSecondary], [tSecondary, tPatch1], [tPatch1, tPatch2]],
	[1, 1],
	avoidClasses(clLake, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);

g_Map.log("Creating grass patches");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tPatch1,
	avoidClasses(clLake, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(65);

g_Map.log("Creating metal mines");
createBalancedMetalMines(
	oMetalSmall,
	oMetalLarge,
	clMetal,
	avoidClasses(clLake, 2, clForest, 0, clPlayer, scaleByMapSize(15, 25), clHill, 1),
	0.9
);

g_Map.log("Creating stone mines");
createBalancedStoneMines(
	oStoneSmall,
	oStoneLarge,
	clRock,
	avoidClasses(clLake, 2, clForest, 0, clPlayer, scaleByMapSize(15, 25), clHill, 1, clMetal, 10),
	0.9
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
		scaleByMapAreaAbsolute(16),
		scaleByMapAreaAbsolute(8),
		scaleByMapSize(bushCount.min, bushCount.max),
		scaleByMapSize(bushCount.min, bushCount.max),
		scaleByMapSize(bushCount.min, bushCount.max)
	],
	avoidClasses(clLake, 0, clForest, 0, clPlayer, 5, clHill, 0, clBaseResource, 5));
Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oMainHunt, 5, 7, 0, 4)],
		[new SimpleObject(oSecHunt, 2, 3, 0, 2)]
	],
	[
		scaleByMapSize(huntCount.min / 2, huntCount.max / 2),
		scaleByMapSize(huntCount.min / 2, huntCount.max / 2)
	],
	avoidClasses(clLake, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	clFood);

createFood(
	[[new SimpleObject(oBerryBush, 5, 7, 0, 4)]],
	[scaleByMapSize(berriesCount.min, berriesCount.max)],
	avoidClasses(clLake, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);

if (!isLakeFrozen)
{
	// actually it would work without an error without this if statement
	// but since this placer would fail most of the time on the frozen lake -> save some time by just skipping it
	// fish are already placed when creating the holes in the ice
	createFood(
		[[new SimpleObject(oFish, 2, 3, 0, 2)]],
		[scaleByMapSize(fishCount.min, fishCount.max)],
		[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
		clFood);
}

Engine.SetProgress(85);

createStragglerTrees(
	[oTree3],
	avoidClasses(clLake, 3, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

// Avoid the lake, even if frozen
placePlayersNomad(clPlayer, avoidClasses(clLake, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

g_Map.ExportMap();
