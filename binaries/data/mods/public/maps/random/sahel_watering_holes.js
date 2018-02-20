Engine.LoadLibrary("rmgen");

const tGrass = "savanna_grass_a";
const tForestFloor = "savanna_forestfloor_a";
const tCliff = "savanna_cliff_b";
const tDirtRocksA = "savanna_dirt_rocks_c";
const tDirtRocksB = "savanna_dirt_rocks_a";
const tDirtRocksC = "savanna_dirt_rocks_b";
const tHill = "savanna_cliff_a";
const tRoad = "savanna_tile_a_red";
const tRoadWild = "savanna_tile_a_red";
const tGrassPatch = "savanna_grass_b";
const tShore = "savanna_riparian_bank";
const tWater = "savanna_riparian_wet";

const oBaobab = "gaia/flora_tree_baobab";
const oFig = "gaia/flora_tree_fig";
const oBerryBush = "gaia/flora_bush_berry";
const oWildebeest = "gaia/fauna_wildebeest";
const oFish = "gaia/fauna_fish";
const oGazelle = "gaia/fauna_gazelle";
const oElephant = "gaia/fauna_elephant_african_bush";
const oGiraffe = "gaia/fauna_giraffe";
const oZebra = "gaia/fauna_zebra";
const oStoneLarge = "gaia/geology_stonemine_desert_quarry";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

const aGrass = "actor|props/flora/grass_savanna.xml";
const aGrassShort = "actor|props/flora/grass_medit_field.xml";
const aRockLarge = "actor|geology/stone_savanna_med.xml";
const aRockMedium = "actor|geology/stone_savanna_med.xml";
const aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
const aBushSmall = "actor|props/flora/bush_dry_a.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oBaobab, tForestFloor + TERRAIN_SEPARATOR + oBaobab, tForestFloor];

const heightSeaGround = -4;
const heightShallows = -2;
const heightLand = 3;
const heightHill = 35;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightLand, tGrass);

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
var clShallows = g_Map.createTileClass();

var [playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.35));

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
		"template": oBerryBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oBaobab,
		"count": 5
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

g_Map.log("Creating rivers");
var riverStart = distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.15), mapCenter)[0];
var riverEnd = distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.49), mapCenter)[0];

for (let i = 0; i < numPlayers; ++i)
{
	let neighborID = (i + 1) % numPlayers;

	// Lake near the center
	createArea(
		new ClumpPlacer(diskArea(scaleByMapSize(5, 30)), 0.95, 0.6, Infinity, riverStart[i]),
		[
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 5));

	// River between the players
	createArea(
		new PathPlacer(riverStart[i], riverEnd[i], scaleByMapSize(10, 50), 0.2, 3 * scaleByMapSize(1, 4), 0.2, 0.05),
		[
			new LayeredPainter([tShore, tWater, tWater], [1, 3]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 5));

	// Lake near the map border
	createArea(
		new ClumpPlacer(diskArea(scaleByMapSize(5, 22)), 0.95, 0.6, Infinity, riverEnd[i]),
		[
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 5));

	// Shallows between neighbors
	createPassage({
		"start": playerPosition[i],
		"end": playerPosition[neighborID],
		"startWidth": 10,
		"endWidth": 10,
		"smoothWidth": 4,
		"startHeight": heightShallows,
		"endHeight": heightShallows,
		"maxHeight": heightShallows,
		"tileClass": clShallows
	});

	// Animals in shallows
	let shallowPosition = Vector2D.average([playerPosition[i], playerPosition[neighborID]]).round();
	let objects = [
		new SimpleObject(oWildebeest, 5, 6, 0, 4),
		new SimpleObject(oElephant, 2, 3, 0, 4)
	];

	for (let object of objects)
		createObjectGroup(new SimpleGroup([object], true, clFood, shallowPosition), 0);
}

paintTerrainBasedOnHeight(-6, 2, 1, tWater);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	avoidClasses(clWater, 2, clPlayer, 20),
	scaleByMapSize(100, 200));

g_Map.log("Creating hills");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, Infinity),
	[
		new LayeredPainter([tGrass, tCliff, tHill], [1, 2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 3),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 3),
	scaleByMapSize(1, 4) * numPlayers);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(160, 900, 0.02);
var types = [
	[[tForestFloor, tGrass, pForest], [tForestFloor, pForest]]
];

var size = forestTrees / (0.5 * scaleByMapSize(2,8) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(forestTrees / num, 0.1, 0.1, Infinity),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(clPlayer, 20, clForest, 10, clHill, 0, clWater, 2),
		num
	);
Engine.SetProgress(50);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tGrass, tDirtRocksA], [tDirtRocksA, tDirtRocksB], [tDirtRocksB, tDirtRocksC]],
				[1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 20),
		scaleByMapSize(15, 45));

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tGrassPatch),
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 20),
		scaleByMapSize(15, 45));
Engine.SetProgress(55);

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(65);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating large decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

g_Map.log("Creating wildebeest");
group = new SimpleGroup(
	[new SimpleObject(oWildebeest, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

Engine.SetProgress(75);

g_Map.log("Creating gazelle");
group = new SimpleGroup(
	[new SimpleObject(oGazelle, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

g_Map.log("Creating elephant");
group = new SimpleGroup(
	[new SimpleObject(oElephant, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

g_Map.log("Creating giraffe");
group = new SimpleGroup(
	[new SimpleObject(oGiraffe, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

g_Map.log("Creating zebra");
group = new SimpleGroup(
	[new SimpleObject(oZebra, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

g_Map.log("Creating fish");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

g_Map.log("Creating berry bush");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

Engine.SetProgress(85);

createStragglerTrees(
	[oBaobab, oBaobab, oBaobab, oFig],
	avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

var planetm = 4;
g_Map.log("Creating small grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2),
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(90);

g_Map.log("Creating large grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(95);

g_Map.log("Creating bushes");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1),
	planetm * scaleByMapSize(13, 200), 50
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clHill, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSkySet("sunny");

setSunRotation(randomAngle());
setSunElevation(Math.PI * randFloat(1/5, 1/4));
setWaterColor(0.478,0.42,0.384);				// greyish
setWaterTint(0.58,0.22,0.067);				// reddish
setWaterMurkiness(0.87);
setWaterWaviness(0.5);
setWaterType("clap");

g_Map.ExportMap();
