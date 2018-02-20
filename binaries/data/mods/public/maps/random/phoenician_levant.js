Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

//TILE_CENTERED_HEIGHT_MAP = true;

const tCity = "medit_city_pavement";
const tCityPlaza = "medit_city_pavement";
const tHill = ["medit_dirt", "medit_dirt_b", "medit_dirt_c", "medit_rocks_grass", "medit_rocks_grass"];
const tMainDirt = "medit_dirt";
const tCliff = "medit_cliff_aegean";
const tForestFloor = "medit_rocks_shrubs";
const tGrass = "medit_rocks_grass";
const tRocksShrubs = "medit_rocks_shrubs";
const tRocksGrass = "medit_rocks_grass";
const tDirt = "medit_dirt_b";
const tDirtB = "medit_dirt_c";
const tShore = "medit_sand";
const tWater = "medit_sand_wet";

const oGrapeBush = "gaia/flora_bush_grapes";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oGoat = "gaia/fauna_goat";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";
const oDatePalm = "gaia/flora_tree_cretan_date_palm_short";
const oSDatePalm = "gaia/flora_tree_cretan_date_palm_tall";
const oCarob = "gaia/flora_tree_carob";
const oFanPalm = "gaia/flora_tree_medit_fan_palm";
const oPoplar = "gaia/flora_tree_poplar_lombardy";
const oCypress = "gaia/flora_tree_cypress";

const aBush1 = "actor|props/flora/bush_medit_sm.xml";
const aBush2 = "actor|props/flora/bush_medit_me.xml";
const aBush3 = "actor|props/flora/bush_medit_la.xml";
const aBush4 = "actor|props/flora/bush_medit_me.xml";
const aDecorativeRock = "actor|geology/stone_granite_med.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor + TERRAIN_SEPARATOR + oCarob, tForestFloor, tForestFloor];

const heightSeaGround = -3;
const heightShore = -1.5;
const heightLand = 1;
const heightIsland = 6;
const heightHill = 15;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightLand, tHill);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clGrass = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clIsland = g_Map.createTileClass();

var startAngle = randIntInclusive(0, 3) * Math.PI / 2;

placePlayerBases({
	"PlayerPlacement": [
		sortAllPlayers(),
		playerPlacementLine(Math.PI / 2, new Vector2D(fractionToTiles(0.76), mapCenter.y), fractionToTiles(0.2)).map(pos =>
			pos.rotateAround(startAngle, mapCenter))
	],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tCityPlaza,
		"innerTerrain": tCity
	},
	"Chicken": {
	},
	"Berries": {
		"template": oGrapeBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oCarob,
		"count": 2
	},
	"Decoratives": {
		"template": aBush1
	}
});
Engine.SetProgress(30);

paintRiver({
	"parallel": true,
	"start": new Vector2D(mapBounds.left, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapBounds.left, mapBounds.bottom).rotateAround(startAngle, mapCenter),
	"width": mapSize,
	"fadeDist": scaleByMapSize(6, 25),
	"deviation": 0,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightLand,
	"meanderShort": 20,
	"meanderLong": 0
});
Engine.SetProgress(40);

paintTileClassBasedOnHeight(-Infinity, heightLand, Elevation_ExcludeMin_ExcludeMax, clWater);
paintTerrainBasedOnHeight(-Infinity, heightShore, Elevation_ExcludeMin_ExcludeMax, tWater);
paintTerrainBasedOnHeight(heightShore, heightLand, Elevation_ExcludeMin_ExcludeMax, tShore);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	avoidClasses(clWater, 2, clPlayer, 20),
	scaleByMapSize(100, 200));

g_Map.log("Creating hills");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 20, clForest, 1, clHill, 15, clWater, 0),
	scaleByMapSize(1, 4) * numPlayers * 3);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.5);
var num = scaleByMapSize(10,42);
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 5))), 0.5),
	[
		new TerrainPainter([tForestFloor, pForest]),
		new TileClassPainter(clForest)
	],
	avoidClasses(clPlayer, 20, clForest, 10, clWater, 1, clHill, 1, clBaseResource, 3),
	num,
	50);
Engine.SetProgress(50);

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter(
				[[tGrass, tRocksShrubs], [tRocksShrubs, tRocksGrass], [tRocksGrass, tGrass]],
				[1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 10, clWater, 4, clDirt, 5, clHill, 1),
		scaleByMapSize(15, 45));
Engine.SetProgress(55);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter(
				[[tDirt, tDirtB], [tDirt, tMainDirt], [tDirtB, tMainDirt]],
				[1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 10, clWater, 4, clGrass, 5, clHill, 1),
		scaleByMapSize(15, 45));
Engine.SetProgress(60);

g_Map.log("Creating cyprus");
createAreas(
	new ClumpPlacer(diskArea(fractionToTiles(0.08)), 0.2, 0.1, 0.01),
	[
		new LayeredPainter([tShore, tHill], [12]),
		new SmoothElevationPainter(ELEVATION_SET, heightIsland, 8),
		new TileClassPainter(clIsland),
		new TileClassUnPainter(clWater)
	],
	[stayClasses (clWater, 8)],
	1,
	100);

g_Map.log("Creating cyprus mines");
var mines = [
	new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock),
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock)
];
for (let mine of mines)
	createObjectGroups(
		mine,
		0,
		[
			stayClasses(clIsland, 9),
			avoidClasses(clForest, 1, clRock, 8, clMetal, 8)
		],
		scaleByMapSize(4, 16));

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(65);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 1),
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating shrubs");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2), new SimpleObject(aBush4, 1,2, 0,1), new SimpleObject(aBush3, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 3, clPlayer, 0, clHill, 1),
	scaleByMapSize(40, 360), 50
);
Engine.SetProgress(70);

g_Map.log("Creating fish");
group = new SimpleGroup([new SimpleObject(oFish, 1,3, 2,6)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clIsland, 2, clFood, 10), stayClasses(clWater, 5)],
	3*scaleByMapSize(5,20), 50
);

g_Map.log("Creating sheeps");
group = new SimpleGroup([new SimpleObject(oSheep, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

g_Map.log("Creating goats");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

g_Map.log("Creating deers");
group = new SimpleGroup([new SimpleObject(oDeer, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

g_Map.log("Creating grape bushes");
group = new SimpleGroup(
	[new SimpleObject(oGrapeBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 15, clHill, 1, clFood, 7),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);
Engine.SetProgress(90);

var stragglerTreeConfig = [
	[1, avoidClasses(clForest, 0, clWater, 4, clPlayer, 8, clMetal, 6, clHill, 1)],
	[3, [stayClasses(clIsland, 9), avoidClasses(clRock, 4, clMetal, 4)]]
];

for (let [amount, constraint] of stragglerTreeConfig)
	createStragglerTrees(
		[oDatePalm, oSDatePalm, oCarob, oFanPalm, oPoplar, oCypress],
		constraint,
		clForest,
		amount * stragglerTrees);

setSkySet("sunny");
setSunColor(0.917, 0.828, 0.734);
setWaterColor(0.263,0.314,0.631);
setWaterTint(0.133, 0.725,0.855);
setWaterWaviness(2.0);
setWaterType("ocean");
setWaterMurkiness(0.8);

setTerrainAmbientColor(0.57, 0.58, 0.55);
setUnitsAmbientColor(0.447059, 0.509804, 0.54902);

setSunElevation(0.671884);
setSunRotation(-0.582913);

setFogFactor(0.2);
setFogThickness(0.15);
setFogColor(0.8, 0.7, 0.6);

setPPEffect("hdr");
setPPContrast(0.53);
setPPSaturation(0.47);
setPPBloom(0.52);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

g_Map.ExportMap();
