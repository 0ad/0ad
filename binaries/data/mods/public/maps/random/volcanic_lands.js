Engine.LoadLibrary("rmgen");

var tGrass = ["cliff volcanic light", "ocean_rock_a", "ocean_rock_b"];
var tGrassA = "cliff volcanic light";
var tGrassB = "ocean_rock_a";
var tGrassC = "ocean_rock_b";
var tCliff = ["cliff volcanic coarse", "cave_walls"];
var tRoad = "road1";
var tRoadWild = "road1";
var tLava1 = "LavaTest05";
var tLava2 = "LavaTest04";
var tLava3 = "LavaTest03";

var oTree = "gaia/flora_tree_dead";
var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_alpine_slabs";

var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";

var pForestD = [tGrassC + TERRAIN_SEPARATOR + oTree, tGrassC];
var pForestP = [tGrassB + TERRAIN_SEPARATOR + oTree, tGrassB];

const heightLand = 1;
const heightHill = 18;

var g_Map = new RandomMap(heightLand, tGrassB);

var numPlayers = getNumPlayers();
var mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.35)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	// No berries, no chicken, no decoratives
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTree,
		"count": scaleByMapSize(12, 30)
	}
});
Engine.SetProgress(15);

createVolcano(mapCenter, clHill, tCliff, [tLava1, tLava2, tLava3], true, ELEVATION_SET);
Engine.SetProgress(45);

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tCliff, tGrass], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 12, clHill, 15, clBaseResource, 2),
	scaleByMapSize(2, 8) * numPlayers
);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(200, 1250, 0.7);
var types = [
	[[tGrassB, tGrassA, pForestD], [tGrassB, pForestD]],
	[[tGrassB, tGrassA, pForestP], [tGrassB, pForestP]]
];
var size = forestTrees / (scaleByMapSize(2,8) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(forestTrees / num, 0.1, 0.1, 1),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0, clBaseResource, 6),
		num);

Engine.SetProgress(70);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([tGrassA, tGrassA], [1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
		scaleByMapSize(20, 80));

for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([tGrassB, tGrassB], [1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
		scaleByMapSize(20, 80));

for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([tGrassC, tGrassC], [1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
		scaleByMapSize(20, 80)
	);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(90);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(95);

createStragglerTrees(
	[oTree],
	avoidClasses(clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6, clBaseResource, 6),
	clForest,
	stragglerTrees);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4));

g_Map.ExportMap();
