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

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oWoodTreasure = "gaia/special_treasure_wood";
const oFoodTreasure = "gaia/special_treasure_food_bin";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;
const aTree = g_Decoratives.tree;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = getMapArea();
var mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clHill2 = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clLand = createTileClass();

var landHeight = 3;
var hillHeight = 30;

var playerCanyonRadius = scaleByMapSize(18, 32);

initTerrain(tMainTerrain);

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

log("Reserving space for the players, their initial forests and some less space therein without trees...");
for (let i = 0; i < numPlayers; ++i)
	for (let j = 1; j <= 2; ++j)
	createArea(
		new ClumpPlacer(
			diskArea(playerCanyonRadius / j),
			0.65,
			0.1,
			10,
			playerPosition[i].x,
			playerPosition[i].y),
		[
			new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
			new SmoothElevationPainter(ELEVATION_SET, landHeight, 2),
			paintClass(j == 1 || isNomad() ? clLand : clPlayer)
		]);

log("Creating center area...");
var center = Math.round(fractionToTiles(0.5));
var lSize = Math.pow(scaleByMapSize(1, 6), 1/8);

createArea(
	new ClumpPlacer(mapArea * 0.065 * lSize, 0.7, 0.1, 10, center, center),
	[
		new LayeredPainter([tMainTerrain, tMainTerrain], [3]),
		new SmoothElevationPainter(ELEVATION_SET, landHeight, 3),
		paintClass(clLand)
	],
	null);

createArea(
	new ClumpPlacer(150, 0.6, 0.3, 10, center, center),
	paintClass(clHill),
	null);

log("Creating hills...");
for (let i = 0; i < scaleByMapSize(9, 16); ++i)
	createArea(
		new PathPlacer(
			randIntExclusive(1, mapSize),
			randIntExclusive(1, mapSize),
			randIntExclusive(1, mapSize),
			randIntExclusive(1, mapSize),
			scaleByMapSize(11, 16),
			0.4,
			3 * scaleByMapSize(1, 4),
			0.1,
			0),
		[
			new SmoothElevationPainter(ELEVATION_SET, hillHeight, 3),
			paintClass(clHill2)
		],
		avoidClasses(clPlayer, 6, clHill2, 3, clHill, 2));

for (let g = 0; g < scaleByMapSize(5, 30); ++g)
{
	var tx = randIntInclusive(1, mapSize - 1);
	var tz = randIntInclusive(1, mapSize - 1);

	var newarea = createArea(
		new ClumpPlacer(mapArea * 0.01 * lSize, 0.7, 0.1, 10, tx, tz),
		[
			new LayeredPainter([tMainTerrain, tMainTerrain], [3]),
			new SmoothElevationPainter(ELEVATION_SET, landHeight, 3),
			paintClass(clLand)
		],
		avoidClasses(clLand, 6));

	if (newarea !== null)
	{
		var distances = [];
		var d1 = 9999;
		var d2 = 9999;
		var p1 = -1;
		var p2 = 0;

		for (let i = 0; i < numPlayers; ++i)
			distances.push(Math.euclidDistance2D(tx, tz, playerPosition[i].x, playerPosition[i].y));

		for (let a = 0; a < numPlayers; ++a)
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

		createArea(
			new PathPlacer(tx, tz, playerPosition[p1].x, playerPosition[p1].y, scaleByMapSize(11, 17), 0.4, 3 * scaleByMapSize(1, 4), 0.1, 0.1),
			[
				new LayeredPainter([tMainTerrain, tMainTerrain], [3]),
				new SmoothElevationPainter(ELEVATION_SET, landHeight, 3),
				paintClass(clLand)
			],
			null);

		if (numPlayers > 1)
			createArea(
				new PathPlacer(tx, tz, playerPosition[p2].x, playerPosition[p2].y, scaleByMapSize(11, 17), 0.4, 3 * scaleByMapSize(1, 4), 0.1, 0.1),
				[
					new LayeredPainter([tMainTerrain, tMainTerrain], [3]),
					new SmoothElevationPainter(ELEVATION_SET, landHeight, 3),
					paintClass(clLand)
				],
				null);
	}
}

for (let i = 0; i < numPlayers; ++i)
{
	log("Creating path from player to center and to neighbor...");
	let neighbor = i + 1 < numPlayers ? i + 1 : 0;
	for (let position of [playerPosition[neighbor], mapCenter])
		createArea(
			new PathPlacer(
				playerPosition[i].x,
				playerPosition[i].y,
				position.x,
				position.y,
				scaleByMapSize(8, 13),
				0.4,
				3 * scaleByMapSize(1, 4),
				0.1,
				0),
			[
				new LayeredPainter([tRoadWild, tRoad], [1]),
				new SmoothElevationPainter(ELEVATION_SET, landHeight, 2),
				paintClass(clLand),
				paintClass(clHill)
			],
			null);
}

log("Painting center place...");
createArea(
	new ClumpPlacer(150, 0.6, 0.3, 10, center, center),
	new LayeredPainter([tRoad, tRoad], [1]),
	null);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	// PlayerTileClass already marked above
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoad,
		"innerTerrain": tRoad,
		"radius": playerCanyonRadius / 3
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
		],
		"distance": 11
	},
	"Trees": {
		"template": oTree1
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

paintTerrainBasedOnHeight(3.1, 29, 0, tCliff);
paintTileClassBasedOnHeight(3.1, 32, 0, clHill2);

createBumps([avoidClasses(clPlayer, 2), stayClasses(clLand, 2)]);

createHills([tCliff, tCliff, tHill], [avoidClasses(clPlayer, 2, clHill, 8, clHill2, 8), stayClasses(clLand, 5)], clHill, scaleByMapSize(10, 40));

// create hills outside the canyon
createHills([tCliff, tCliff, tMainTerrain], avoidClasses(clLand, 1, clHill, 1), clHill, scaleByMapSize(20, 150), undefined, undefined, undefined, undefined, 40);

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 [avoidClasses(clPlayer, 1, clForest, 15, clHill, 1, clHill2, 0), stayClasses(clLand, 4)],
 clForest,
 forestTrees);

Engine.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clHill2, 0), stayClasses(clLand, 3)],
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clHill2, 0), stayClasses(clLand, 3)],
 scaleByMapSize(15, 45),
 clDirt);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 3, clRock, 10, clHill, 1, clHill2, 1), stayClasses(clLand, 2)],
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 3, clMetal, 10, clRock, 5, clHill, 1, clHill2, 1), stayClasses(clLand, 2)],
 clMetal
);

Engine.SetProgress(65);

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
		3 * scaleByMapSize(16, 262),
		3 * scaleByMapSize(8, 131),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200)
	],
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0));

log("Creating actor trees...");
var group = new SimpleGroup(
	[new SimpleObject(aTree, 1,1, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clLand, 5),
	scaleByMapSize(200, 800), 50
);

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
	[avoidClasses(clForest, 0, clPlayer, 4, clHill, 1, clFood, 20, clHill2, 1), stayClasses(clLand, 3)],
	clFood);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers
	],
	[avoidClasses(clForest, 0, clPlayer, 4, clHill, 1, clFood, 10, clHill2, 1), stayClasses(clLand, 3)],
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 1, clHill, 1, clPlayer, 9, clMetal, 6, clRock, 6, clHill2, 1), stayClasses(clLand, 3)],
	clForest,
	stragglerTrees);

log("Creating treasures...");
for (let i = 0; i < randIntInclusive(3, 8); ++i)
	for (let template of [oFoodTreasure, oWoodTreasure])
		placeObject(center + randFloat(-7, 7), center + randFloat(-7, 7), template, 0, randomAngle());

placePlayersNomad(
	clPlayer,
	[stayClasses(clLand, 4), avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clHill2, 4, clFood, 2)]);

ExportMap();
