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
const oWoodTreasure = "gaia/special_treasure_wood";
const oDock = "skirmish/structures/default_dock";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

const heightSeaGround = -5;
const heightLand = 3;
const heightOffsetBump = 2;
const heightHill = 18;

InitMap(heightSeaGround, tWater);

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clLand = createTileClass();

var playerIslandRadius = scaleByMapSize(20, 29);

var [playerIDs, playerPosition, playerAngle] = playerPlacementCircle(fractionToTiles(0.35));

if (!isNomad())
{
	log("Creating player islands and docks...");
	for (let i = 0; i < numPlayers; i++)
	{
		createArea(
			new ClumpPlacer(diskArea(playerIslandRadius), 0.8, 0.1, 10, playerPosition[i]),
			[
				new LayeredPainter([tMainTerrain , tMainTerrain, tMainTerrain], [1, 6]),
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 6),
				paintClass(clLand),
				paintClass(clPlayer)
			]);

		let dockLocation = findLocationInDirectionBasedOnHeight(playerPosition[i], mapCenter, -3 , heightLand - 0.5, heightLand);
		placeObject(dockLocation.x, dockLocation.y, oDock, playerIDs[i], playerAngle[i] + Math.PI);
	}
}

log("Creating big islands...");
createAreas(
	new ChainPlacer(
		Math.floor(scaleByMapSize(4, 8)),
		Math.floor(scaleByMapSize(8, 14)),
		Math.floor(scaleByMapSize(25, 60)),
		0.07),
	[
		new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 6),
		paintClass(clLand)
	],
	avoidClasses(clLand, scaleByMapSize(8, 12)),
	scaleByMapSize(4, 14));

log("Creating small islands...");
createAreas(
	new ChainPlacer(
		Math.floor(scaleByMapSize(4, 7)),
		Math.floor(scaleByMapSize(7, 10)),
		Math.floor(scaleByMapSize(16, 40)),
		0.07),
	[
		new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 6),
		paintClass(clLand)
	],
	avoidClasses(clLand, scaleByMapSize(8, 12)),
	scaleByMapSize(6, 54));

paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	// PlayerTileClass marked above
	"BaseResourceClass": clBaseResource,
	"Walls": "towers",
	"CityPatch": {
		"radius": playerIslandRadius / 3,
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
	"Treasures": {
		"types": [
			{
				"template": oWoodTreasure,
				"count": 14
			}
		]
	},
	"Trees": {
		"template": oTree1,
		"count": 5
	},
	"Decoratives": {
		"template": aGrassShort
	}
});

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	[avoidClasses(clPlayer, 0), stayClasses(clLand, 3)],
	scaleByMapSize(20, 100));

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		paintClass(clHill)
	],
	[avoidClasses(clPlayer, 2, clHill, 15), stayClasses(clLand, 0)],
	scaleByMapSize(4, 13));

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];

if (currentBiome() != "savanna")
{
	var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);
	var num = Math.floor(size / types.length);
	for (let type of types)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 5))), 0.5),
			[
				new LayeredPainter(type, [2]),
				paintClass(clForest)
			],
			[avoidClasses(clPlayer, 0, clForest, 10, clHill, 0), stayClasses(clLand, 6)],
			num);
}

Engine.SetProgress(50);
log("Creating dirt patches...");
var numberOfPatches = scaleByMapSize(15, 45) * (currentBiome() == "savanna" ? 3 : 1);
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], [1, 1]),
			paintClass(clDirt)
		],
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 6)],
		numberOfPatches);

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		new TerrainPainter(tTier4Terrain),
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 6)],
		numberOfPatches);

Engine.SetProgress(55);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 0, clRock, 10, clHill, 1), stayClasses(clLand, 5)],
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 0, clRock, 10, clHill, 1), stayClasses(clLand, 5)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 0, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 5)],
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)],
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)],
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	3 * numPlayers, 50
);

Engine.SetProgress(75);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	3 * numPlayers, 50
);

log("Creating fruit bush...");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clLand, 4, clForest, 2, clPlayer, 2, clHill, 2, clFood, 20),
	25 * numPlayers, 60
);

Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 1, clHill, 1, clPlayer, 0, clMetal, 6, clRock, 6), stayClasses(clLand, 6)],
	clForest,
	stragglerTrees);

var planetm = 1;
if (currentBiome() == "tropic")
	planetm = 8;

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 5)],
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200), 50
);

placePlayersNomad(clPlayer, [stayClasses(clLand, 4), avoidClasses(clHill, 2, clForest, 1, clMetal, 4, clRock, 4, clFood, 2)]);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randomAngle());
setSunElevation(randFloat(1/5, 1/3) * Math.PI);
setWaterWaviness(2);

ExportMap();
