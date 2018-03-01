Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

const tSnowA = ["polar_snow_b"];
const tSnowB = "polar_ice_snow";
const tSnowC = "polar_ice";
const tSnowD = "polar_snow_a";
const tForestFloor = "polar_tundra_snow";
const tCliff = "polar_snow_rocks";
const tSnowE = ["polar_snow_glacial"];
const tRoad = "new_alpine_citytile";
const tRoadWild = "new_alpine_citytile";
const tShoreBlend = "alpine_shore_rocks_icy";
const tShore = "alpine_shore_rocks";
const tWater = "alpine_shore_rocks";

const oPine = "gaia/flora_tree_pine_w";
const oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
const oStoneSmall = "gaia/geology_stone_alpine_a";
const oMetalLarge = "gaia/geology_metal_alpine_slabs";
const oFish = "gaia/fauna_fish";
const oWalrus = "gaia/fauna_walrus";
const oArcticWolf = "gaia/fauna_arctic_wolf";

const aIceberg = "actor|props/special/eyecandy/iceberg.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor, tForestFloor];
const pForestS = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor, tForestFloor, tForestFloor];

const heightSeaGround = -5;
const heightLake = -4;
const heightLand = 3;
const heightHill = 25;

var g_Map = new RandomMap(heightLand, tSnowA);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clIsland = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

var startAngle = randomAngle();

placePlayerBases({
	"PlayerPlacement": [
		sortAllPlayers(),
		playerPlacementLine(0, new Vector2D(mapCenter.x, fractionToTiles(0.45)), fractionToTiles(0.2)).map(
			pos => pos.rotateAround(startAngle, mapCenter))
	],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	// No chicken, no berries
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oPine,
		"count": scaleByMapSize(12, 30),
	}
	// No decoratives
});
Engine.SetProgress(15);

paintRiver({
	"parallel": true,
	"start": new Vector2D(mapBounds.left, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapBounds.right, mapBounds.top).rotateAround(startAngle, mapCenter),
	"width": 2 * fractionToTiles(0.31),
	"fadeDist": 8,
	"deviation": 0,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightLand,
	"meanderShort": 0,
	"meanderLong": 0
});

paintTileClassBasedOnHeight(-Infinity, 0.5, Elevation_ExcludeMin_ExcludeMax, clWater);

g_Map.log("Creating shores");
for (let i = 0; i < scaleByMapSize(20, 120); ++i)
{
	let position = new Vector2D(fractionToTiles(randFloat(0.1, 0.9)), fractionToTiles(randFloat(0.67, 0.74))).rotateAround(startAngle, mapCenter).round();
	createArea(
		new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 30)), Infinity, position),
		[
			new TerrainPainter(tSnowA),
			new SmoothElevationPainter(ELEVATION_SET, heightLand, 3),
			new TileClassUnPainter(clWater)
		]);
}

g_Map.log("Creating islands");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new TerrainPainter(tSnowA),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 3),
		new TileClassPainter(clIsland),
		new TileClassUnPainter(clWater)
	],
	stayClasses(clWater, 7),
	scaleByMapSize(10, 80));

paintTerrainBasedOnHeight(-6, 1, 1, tWater);

g_Map.log("Creating lakes");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(5, 7)), Math.floor(scaleByMapSize(20, 50)), 0.1),
	[
		new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, heightLake, 3),
		new TileClassPainter(clWater)
	],
	avoidClasses(clPlayer, 20, clWater, 20),
	Math.round(scaleByMapSize(1, 4) * numPlayers));

paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);

Engine.SetProgress(45);

g_Map.log("Creating hills");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new LayeredPainter([tCliff, tSnowA], [3]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 3),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 2, clBaseResource, 2),
	scaleByMapSize(1, 4) * numPlayers
);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(100, 625, 0.7);
var types = [
	[[tSnowA, tSnowA, tSnowA, tSnowA, pForestD], [tSnowA, tSnowA, tSnowA, pForestD]],
	[[tSnowA, tSnowA, tSnowA, tSnowA, pForestS], [tSnowA, tSnowA, tSnowA, pForestS]]
];

var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);

var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 4))), Infinity),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(clPlayer, 20, clForest, 20, clHill, 0, clWater, 8),
		num);

g_Map.log("Creating iceberg");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aIceberg, 0, 2, 0, 4)], true, clRock),
	0,
	[avoidClasses(clRock, 6), stayClasses(clWater, 4)],
	scaleByMapSize(4, 16),
	100);
Engine.SetProgress(70);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tSnowD, tSnowB, tSnowC], [2, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clPlayer, 20,
			clDirt, 16),
		scaleByMapSize(20, 80));

for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new TerrainPainter(tSnowE),
			new TileClassPainter(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clPlayer, 20,
			clDirt, 16),
		scaleByMapSize(20, 80));

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(8,32), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(8,32), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(8,32), 100
);
Engine.SetProgress(80);

createStragglerTrees(
	[oPine],
	avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

g_Map.log("Creating deer");
group = new SimpleGroup(
	[new SimpleObject(oWalrus, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

Engine.SetProgress(90);

g_Map.log("Creating sheep");
group = new SimpleGroup(
	[new SimpleObject(oArcticWolf, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
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

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2, clIsland, 4));

setSunColor(0.6, 0.6, 0.6);
setSunElevation(Math.PI/ 6);

setWaterColor(0.02, 0.17, 0.52);
setWaterTint(0.494, 0.682, 0.808);
setWaterMurkiness(0.82);
setWaterWaviness(0.5);
setWaterType("ocean");

setFogFactor(0.95);
setFogThickness(0.09);
setPPSaturation(0.28);
setPPEffect("hdr");

setSkySet("fog");
g_Map.ExportMap();
