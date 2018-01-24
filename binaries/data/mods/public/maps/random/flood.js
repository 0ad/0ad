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
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tTier4Terrain = g_Terrains.tier4Terrain;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

var tHill = g_Terrains.hill;
var tDirt = g_Terrains.dirt;
if (currentBiome() == "temperate")
{
	tDirt = ["medit_shrubs_a", "grass_field"];
	tHill = ["grass_field", "peat_temp"];
}

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
const oMetalLarge = g_Gaia.metalLarge;

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

const heightSeaGround = -2;
const heightLand = 2;
const shoreRadius = 6;

var g_Map = new RandomMap(heightSeaGround, tWater);

const clPlayer = createTileClass();
const clHill = createTileClass();
const clMountain = createTileClass();
const clForest = createTileClass();
const clDirt = createTileClass();
const clRock = createTileClass();
const clMetal = createTileClass();
const clFood = createTileClass();
const clBaseResource = createTileClass();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();

log("Creating player islands...")
var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.38));

for (let i = 0; i < numPlayers; ++i)
	createArea(
		new ClumpPlacer(diskArea(1.4 * defaultPlayerBaseRadius()), 0.8, 0.1, 10, playerPosition[i]),
		[
			new LayeredPainter([tShore, tMainTerrain], [shoreRadius]),
			new SmoothElevationPainter(ELEVATION_SET, heightLand, shoreRadius),
			paintClass(clHill)
		]);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": false,
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
		"template": oTree2,
		"count": 50,
		"maxDist": 16,
		"maxDistGroup": 7
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(40);

log("Creating central island...");
createArea(
	new ChainPlacer(
		6,
		Math.floor(scaleByMapSize(10, 15)),
		Math.floor(scaleByMapSize(200, 300)),
		1,
		mapCenter,
		0,
		[Math.floor(fractionToTiles(0.01))]),
	[
		new LayeredPainter([tShore, tMainTerrain], [shoreRadius, 100]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, shoreRadius),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 40));

for (let m = 0; m < randIntInclusive(20, 34); ++m)
{
	let elevRand = randIntInclusive(6, 20);
	createArea(
		new ChainPlacer(
			7,
			15,
			Math.floor(scaleByMapSize(15, 20)),
			1,
			new Vector2D(fractionToTiles(randFloat(0, 1)), fractionToTiles(randFloat(0, 1))),
			0,
			[Math.floor(fractionToTiles(0.01))]),
		[
			new LayeredPainter([tDirt, tHill], [Math.floor(elevRand / 3), 40]),
			new SmoothElevationPainter(ELEVATION_SET, elevRand, Math.floor(elevRand / 3)),
			paintClass(clHill)
		],
		[avoidClasses(clBaseResource, 2, clPlayer, 40), stayClasses(clHill, 6)]);
}

for (let m = 0; m < randIntInclusive(8, 17); ++m)
{
	let elevRand = randIntInclusive(15, 29);
	createArea(
		new ChainPlacer(
				5,
				8,
				Math.floor(scaleByMapSize(15, 20)),
				1,
				new Vector2D(randIntExclusive(0, mapSize), randIntExclusive(0, mapSize)),
				0,
				[Math.floor(fractionToTiles(0.01))]),
		[
			new LayeredPainter([tCliff, tForestFloor2], [Math.floor(elevRand / 3), 40]),
			new SmoothElevationPainter(ELEVATION_MODIFY, elevRand, Math.floor(elevRand / 3)),
			paintClass(clMountain)
		],
		[avoidClasses(clBaseResource, 2, clPlayer, 40), stayClasses(clHill, 6)]);
}

log("Creating center bounty...");
createObjectGroup(
	new SimpleGroup(
		[new SimpleObject(oMetalLarge, 3, 6, 25, Math.floor(fractionToTiles(0.25)))],
		true,
		clBaseResource,
		mapCenter.x,
		mapCenter.y),
	0,
	[avoidClasses(clBaseResource, 20, clPlayer, 40, clMountain, 4), stayClasses(clHill, 10)]);

createObjectGroup(
	new SimpleGroup(
		[new SimpleObject(oStoneLarge, 3, 6, 25, Math.floor(fractionToTiles(0.25)))],
		true,
		clBaseResource,
		mapCenter.x,
		mapCenter.y),
		0,
		[avoidClasses(clBaseResource, 20, clPlayer, 40, clMountain, 4), stayClasses(clHill, 10)]);

log("Creating fish...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oFish, 2, 3, 0, 2)], true, clFood),
	0,
	avoidClasses(clHill, 10, clFood, 20),
	10 * numPlayers,
	60);

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(0.7));
createForests(
	[tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
	[avoidClasses(clPlayer, 25, clForest, 10, clBaseResource, 3, clMetal, 6, clRock, 3, clMountain, 2), stayClasses(clHill, 6)],
	clForest,
	forestTrees);

let types = [oTree1, oTree2, oTree4, oTree3];
createStragglerTrees(
	types,
	[avoidClasses(clBaseResource, 2, clMetal, 6, clRock, 3, clMountain, 2, clPlayer, 25), stayClasses(clHill, 6)],
	clForest,
	stragglerTrees);
Engine.SetProgress(65);

log("Creating dirt patches...");
var numb = currentBiome() == "savanna" ? 3 : 1;
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], [1, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clForest, 0, clMountain, 0, clDirt, 5, clPlayer, 10),
		numb * scaleByMapSize(15, 45));

log("Painting shorelines...");
paintTerrainBasedOnHeight(1, heightLand, 0, tMainTerrain);
paintTerrainBasedOnHeight(heightSeaGround, 1, 3, tTier1Terrain);

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		new TerrainPainter(tTier4Terrain),
		avoidClasses(clForest, 0, clMountain, 0, clDirt, 5, clPlayer, 10),
		numb * scaleByMapSize(15, 45));

log("Creating food...");
createFood(
	[
		[new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)],
		[new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)]
	],
	[3 * numPlayers, 3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 20, clMountain, 1, clFood, 4, clRock, 6, clMetal, 6), stayClasses(clHill, 6)],
	clFood);

Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 15, clMountain, 1, clFood, 4, clRock, 6, clMetal, 6), stayClasses(clHill, 6)],
	clFood);

Engine.SetProgress(85);

log("Creating decoration...");
var planetm = currentBiome() == "tropic" ? 8 : 1;
createDecoration(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 2, 15, 0, 1)],
		[new SimpleObject(aGrass, 2, 10, 0, 1.8), new SimpleObject(aGrassShort, 3, 10, 1.2, 2.5)],
		[new SimpleObject(aBushMedium, 1, 5, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200)
	],
	avoidClasses(clForest, 2, clPlayer, 20, clMountain, 5, clFood, 1, clBaseResource, 2));

log("Creating water forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(0.1));
createForests(
	[tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
	avoidClasses(clPlayer, 30, clHill, 10, clFood, 5),
	clForest,
	forestTrees);

log("Creating small grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aGrassShort, 1, 2, 0, 1)]),
	0,
	[avoidClasses(clMountain, 2, clPlayer, 2, clDirt, 0), stayClasses(clHill, 8)],
	planetm * scaleByMapSize(13, 200));

placePlayersNomad(
	clPlayer,
	new AndConstraint([
		stayClasses(clHill, 2),
		avoidClasses(clMountain, 2, clForest, 1, clMetal, 4, clRock, 4, clFood, 2)]));

setSkySet(pickRandom(["cloudless", "cumulus", "overcast"]));
setWaterMurkiness(0.4);

g_Map.ExportMap();
