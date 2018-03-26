Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
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
const tWater = g_Terrains.water;

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

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

const heightIsland = 20;
const heightSeaGround = -5;

var g_Map = new RandomMap(heightSeaGround, tWater);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clLand = g_Map.createTileClass();

const playerIslandRadius = scaleByMapSize(15, 30);

const islandBetweenPlayerAndCenterDist = 0.16;
const islandBetweenPlayerAndCenterRadius = 0.81;
const centralIslandRadius = 0.36;

var [playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.35));

var numIslands = 0;
var isConnected = [];
var islandPos = [];

function initIsConnected()
{
	for (let m = 0; m < numIslands; ++m)
	{
		isConnected[m] = [];
		for (let n = 0; n < numIslands; ++n)
			isConnected[m][n] = 0;
	}
}

function createIsland(islandID, size, tileClass)
{
	createArea(
		new ClumpPlacer(size * diskArea(playerIslandRadius), 0.95, 0.6, Infinity, islandPos[islandID]),
		[
			new TerrainPainter(tHill),
			new SmoothElevationPainter(ELEVATION_SET, heightIsland, 2),
			new TileClassPainter(tileClass)
		]);
}

function createIslandAtRadialLocation(playerID, islandID, playerIDOffset, distFromCenter, islandRadius)
{
	let angle = startAngle + (playerID * 2 + playerIDOffset) * Math.PI / numPlayers;
	islandPos[islandID] = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(distFromCenter), 0).rotate(-angle)).round();
	createIsland(islandID, islandRadius, clLand);
}

function createSnowflakeSearockWithCenter(sizeID)
{
	let [tertiaryIslandDist, tertiaryIslandRadius, islandBetweenPlayersDist, islandBetweenPlayersRadius] = islandSizes[sizeID];

	let islandID_center = 4 * numPlayers;
	numIslands = islandID_center + 1;
	initIsConnected();

	g_Map.log("Creating central island");
	islandPos[islandID_center] = mapCenter;
	createIsland(islandID_center, centralIslandRadius, clLand);

	for (let playerID = 0; playerID < numPlayers; ++playerID)
	{
		let playerID_neighbor = playerID + 1 < numPlayers ? playerID + 1 : 0;

		let islandID_player = playerID;
		let islandID_playerNeighbor = playerID_neighbor;
		let islandID_betweenPlayers = playerID + numPlayers;
		let islandID_betweenPlayerAndCenter = playerID + 2 * numPlayers;
		let islandID_betweenPlayerAndCenterNeighbor = playerID_neighbor + 2 * numPlayers;
		let islandID_tertiary = playerID + 3 * numPlayers;

		g_Map.log("Creating island between the player and their neighbor");
		isConnected[islandID_betweenPlayers][islandID_player] = 1;
		isConnected[islandID_betweenPlayers][islandID_playerNeighbor] = 1;
		createIslandAtRadialLocation(playerID, islandID_betweenPlayers, 1, islandBetweenPlayersDist, islandBetweenPlayersRadius);

		g_Map.log("Creating an island between the player and the center");
		isConnected[islandID_betweenPlayerAndCenter][islandID_player] = 1;
		isConnected[islandID_betweenPlayerAndCenter][islandID_center] = 1;
		isConnected[islandID_betweenPlayerAndCenter][islandID_betweenPlayerAndCenterNeighbor] = 1;
		createIslandAtRadialLocation(playerID, islandID_betweenPlayerAndCenter, 0, islandBetweenPlayerAndCenterDist, islandBetweenPlayerAndCenterRadius);

		g_Map.log("Creating tertiary island, at the map border");
		isConnected[islandID_tertiary][islandID_betweenPlayers] = 1;
		createIslandAtRadialLocation(playerID, islandID_tertiary, 1, tertiaryIslandDist, tertiaryIslandRadius);
	}
}

/**
 * Creates one island in front of every player and connects it with the neighbors.
 */
function createSnowflakeSearockWithoutCenter()
{
	numIslands = 2 * numPlayers;
	initIsConnected();

	for (let playerID = 0; playerID < numPlayers; ++playerID)
	{
		let playerID_neighbor = playerID + 1 < numPlayers ? playerID + 1 : 0;

		let islandID_player = playerID;
		let islandID_playerNeighbor = playerID_neighbor;
		let islandID_inFrontOfPlayer = playerID + numPlayers;
		let islandID_inFrontOfPlayerNeighbor = playerID_neighbor + numPlayers;

		isConnected[islandID_player][islandID_playerNeighbor] = 1;
		isConnected[islandID_player][islandID_inFrontOfPlayer] = 1;
		isConnected[islandID_inFrontOfPlayer][islandID_inFrontOfPlayerNeighbor] = 1;

		createIslandAtRadialLocation(playerID, islandID_inFrontOfPlayer, 0, islandBetweenPlayerAndCenterDist, islandBetweenPlayerAndCenterRadius);
	}
}

function createSnowflakeSearockTiny()
{
	numIslands = numPlayers + 1;
	initIsConnected();

	let islandID_center = numPlayers;

	g_Map.log("Creating central island");
	islandPos[islandID_center] = mapCenter;
	createIsland(numPlayers, 1, clLand);

	for (let playerID = 0; playerID < numPlayers; ++playerID)
	{
		let islandID_player = playerID;
		isConnected[islandID_player][islandID_center] = 1;
	}
}

const islandSizes = {
	"medium":  [0.41, 0.49, 0.26, 1],
	"large1":  [0.41, 0.49, 0.24, 1],
	"large2":  [0.41, 0.36, 0.28, 0.81]
};

if (mapSize <= 128)
{
	createSnowflakeSearockTiny();
}
else if (mapSize <= 192)
{
	createSnowflakeSearockWithoutCenter();
}
else if (mapSize <= 256)
{
	if (numPlayers < 6)
		createSnowflakeSearockWithCenter("medium");
	else
		createSnowflakeSearockWithoutCenter();
}
else if (mapSize <= 320)
{
	if (numPlayers < 8)
		createSnowflakeSearockWithCenter("medium");
	else
		createSnowflakeSearockWithoutCenter();
}
else
	createSnowflakeSearockWithCenter(numPlayers < 6 ? "large1" : "large2");

g_Map.log("Creating player islands");
for (let i = 0; i < numPlayers; ++i)
{
	islandPos[i] = playerPosition[i];
	createIsland(i, 1, isNomad() ? clLand : clPlayer);
}

g_Map.log("Creating connectors");
for (let i = 0; i < numIslands; ++i)
	for (let j = 0; j < numIslands; ++j)
		if (isConnected[i][j])
			createArea(
				new PathPlacer(islandPos[i], islandPos[j], 11, 0, 1, 0, 0, Infinity),
				[
					new SmoothElevationPainter(ELEVATION_SET, heightIsland, 2),
					new TerrainPainter(tHill),
					new TileClassPainter(clLand)
				]);

g_Map.log("Painting cliffs")
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tCliff),
	new SlopeConstraint(2, Infinity));
Engine.SetProgress(30);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	// PlayerTileClass already marked above
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": stayClasses(clPlayer, 4),
	"Walls": "towers",
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
	},
	"Berries": {
		"template": oFruitBush,
		"distance": playerIslandRadius - 4
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		],
		"distance": playerIslandRadius - 4
	},
	"Trees": {
		"template": oTree1,
		"count": scaleByMapSize(10, 50),
		"minDist": 11,
		"maxDist": 11
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(40);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];

var size = forestTrees / (scaleByMapSize(2, 8) * numPlayers) * (currentBiome() == "generic/savanna" ? 2 : 1);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(forestTrees / num, 0.1, 0.1, Infinity),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		[avoidClasses(clPlayer, 6, clForest, 10), stayClasses(clLand, 4)],
		num);
Engine.SetProgress(55);

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10), stayClasses(clLand, 5)],
	5*scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10), stayClasses(clLand, 5)],
	5*scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5), stayClasses(clLand, 5)],
	5*scaleByMapSize(4,16), 100
);

Engine.SetProgress(65);
g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([[tMainTerrain, tTier1Terrain],[tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], [1, 1]),
			new TileClassPainter(clDirt)
		],
		[avoidClasses(clForest, 0, clDirt, 5, clPlayer, 12), stayClasses(clLand, 5)],
		scaleByMapSize(15, 45));

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tTier4Terrain),
		[avoidClasses(clForest, 0, clDirt, 5, clPlayer, 12), stayClasses(clLand, 5)],
		scaleByMapSize(15, 45));

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0), stayClasses(clLand, 4)],
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating large decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0), stayClasses(clLand, 4)],
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

g_Map.log("Creating deer");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clFood, 20), stayClasses(clLand, 4)],
	3 * numPlayers, 50
);

Engine.SetProgress(75);

g_Map.log("Creating sheep");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clFood, 20), stayClasses(clLand, 4)],
	3 * numPlayers, 50
);

g_Map.log("Creating fruits");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clFood, 20), stayClasses(clLand, 4)],
	3 * numPlayers, 50
);
Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 1, clPlayer, 9, clMetal, 6, clRock, 6), stayClasses(clLand, 4)],
	clForest,
	stragglerTrees);

var planetm = 1;
if (currentBiome() == "generic/tropic")
	planetm = 8;

g_Map.log("Creating small grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clPlayer, 2, clDirt, 0), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(90);

g_Map.log("Creating large grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200)
);
Engine.SetProgress(95);

g_Map.log("Creating bushes");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clPlayer, 1, clDirt, 1), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200), 50
);

placePlayersNomad(
	clPlayer,
	[
		stayClasses(clLand, 8),
		avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clFood, 2)
	]);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randomAngle());
setSunElevation(Math.PI * randFloat(1/5, 1/3));

g_Map.ExportMap();
