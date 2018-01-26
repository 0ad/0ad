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
const heightHill = 18;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightSeaGround, tWater);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clLand = g_Map.createTileClass();
var clIsland = g_Map.createTileClass();

var startAngle = randomAngle();
var playerIDs = sortAllPlayers();
var [playerPosition, playerAngle] = playerPlacementCustomAngle(
	fractionToTiles(0.35),
	mapCenter,
	i => startAngle - Math.PI * (i + 1) / (numPlayers + 1));

log("Creating player islands and docks...");
for (let i = 0; i < numPlayers; ++i)
{
	createArea(
		new ClumpPlacer(diskArea(defaultPlayerBaseRadius()), 0.8, 0.1, 10, playerPosition[i]),
		[
			new LayeredPainter([tWater, tShore, tMainTerrain], [1, 4]),
			new SmoothElevationPainter(ELEVATION_SET, heightLand, 4),
			new TileClassPainter(clIsland),
			new TileClassPainter(isNomad() ? clLand : clPlayer)
		]);

	if (isNomad())
		continue;

	let dockLocation = findLocationInDirectionBasedOnHeight(playerPosition[i], mapCenter, -3 , 2.6, 3);
	placeObject(dockLocation.x, dockLocation.y, oDock, playerIDs[i], playerAngle[i] + Math.PI);
}
Engine.SetProgress(10);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": false,
	// No city patch
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
		"count": scaleByMapSize(12, 30)
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(15);

log("Create the continent body...");
var continentPosition = Vector2D.add(mapCenter, new Vector2D(0, fractionToTiles(0.38)).rotate(-startAngle)).round()
createArea(
	new ClumpPlacer(diskArea(fractionToTiles(0.4)), 0.8, 0.08, 10, continentPosition),
	[
		new LayeredPainter([tWater, tShore, tMainTerrain], [4, 2]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 4),
		new TileClassPainter(clLand)
	],
	avoidClasses(clIsland, 8));
Engine.SetProgress(20);

log("Creating shore jaggedness...");
createAreas(
	new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1),
	[
		new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 4),
		new TileClassPainter(clLand)
	],
	[
		borderClasses(clLand, 6, 3),
		avoidClasses(clIsland, 8)
	],
	scaleByMapSize(2, 15) * 20,
	150);

paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
Engine.SetProgress(25);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	[avoidClasses(clIsland, 10), stayClasses(clLand, 3)],
	scaleByMapSize(100, 200)
);
Engine.SetProgress(30);

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	[avoidClasses(clIsland, 10, clHill, 15), stayClasses(clLand, 7)],
	scaleByMapSize(1, 4) * numPlayers
);
Engine.SetProgress(34);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];

var size = forestTrees / (scaleByMapSize(2,8) * numPlayers) *
	(currentBiome() == "generic/savanna" ? 2 : 1);

var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(forestTrees / num, 0.1, 0.1, 1),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		[avoidClasses(clPlayer, 6, clForest, 10, clHill, 0), stayClasses(clLand, 7)],
		num);
Engine.SetProgress(38);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]],
				[1, 1]),
			new TileClassPainter(clDirt)
		],
		[
			avoidClasses(
				clForest, 0,
				clHill, 0,
				clDirt, 5,
				clIsland, 0),
			stayClasses(clLand, 7)
		],
		scaleByMapSize(15, 45));

Engine.SetProgress(42);

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tTier4Terrain),
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clIsland, 0), stayClasses(clLand, 7)],
		scaleByMapSize(15, 45));
Engine.SetProgress(46);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1), stayClasses(clLand, 7)],
	scaleByMapSize(4,16), 100
);
Engine.SetProgress(50);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1), stayClasses(clLand, 7)],
	scaleByMapSize(4,16), 100
);
Engine.SetProgress(54);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 7)],
	scaleByMapSize(4,16), 100
);
Engine.SetProgress(58);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 6)],
	scaleByMapSize(16, 262), 50
);
Engine.SetProgress(62);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 6)],
	scaleByMapSize(8, 131), 50
);
Engine.SetProgress(66);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), stayClasses(clLand, 7)],
	3 * numPlayers, 50
);
Engine.SetProgress(70);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), stayClasses(clLand, 7)],
	3 * numPlayers, 50
);
Engine.SetProgress(74);

log("Creating fruit bush...");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 7)],
	randIntInclusive(1, 4) * numPlayers + 2, 50
);
Engine.SetProgress(78);

log("Creating fish...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oFish, 2,3, 0,2)], true, clFood),
	0,
	avoidClasses(clLand, 2, clPlayer, 2, clHill, 0, clFood, 20),
	25 * numPlayers, 60
);
Engine.SetProgress(82);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 1, clHill, 1, clPlayer, 9, clMetal, 6, clRock, 6), stayClasses(clLand, 9)],
	clForest,
	stragglerTrees);

Engine.SetProgress(86);

var planetm = currentBiome() == "generic/tropic" ? 8 : 1;

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
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200)
);
Engine.SetProgress(94);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200), 50
);
Engine.SetProgress(98);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randomAngle());
setSunElevation(randFloat(1/5, 1/3) * Math.PI);
setWaterWaviness(2);

placePlayersNomad(clPlayer, [stayClasses(clIsland, 4), avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2)]);

g_Map.ExportMap();
