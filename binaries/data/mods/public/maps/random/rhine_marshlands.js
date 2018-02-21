Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

const tGrass = ["temp_grass", "temp_grass", "temp_grass_d"];
const tForestFloor = "temp_plants_bog";
const tGrassA = "temp_grass_plants";
const tGrassB = "temp_plants_bog";
const tMud = "temp_mud_a";
const tRoad = "temp_road";
const tRoadWild = "temp_road_overgrown";
const tShoreBlend = "temp_grass_plants";
const tShore = "temp_plants_bog";
const tWater = "temp_mud_a";

const oBeech = "gaia/flora_tree_euro_beech";
const oOak = "gaia/flora_tree_oak";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oHorse = "gaia/fauna_horse";
const oWolf = "gaia/fauna_wolf";
const oRabbit = "gaia/fauna_rabbit";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";

const aGrass = "actor|props/flora/grass_soft_small_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oBeech, tForestFloor];
const pForestP = [tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor];

const heightMarsh = -2
const heightLand = 1;
const heightOffsetBumpWater = 1;
const heightOffsetBumpLand = 2;

var g_Map = new RandomMap(heightLand, tGrass);

const numPlayers = getNumPlayers();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.35)),
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
		"template": oBeech
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(15);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBumpLand, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800));

g_Map.log("Creating marshes");
for (let i = 0; i < 7; ++i)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(6, 12)), Math.floor(scaleByMapSize(15, 60)), 0.8),
		[
			new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
			new SmoothElevationPainter(ELEVATION_SET, heightMarsh, 3),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 20, clWater, Math.round(scaleByMapSize(7,16)*randFloat(0.8,1.35))),
		scaleByMapSize(4,20));

g_Map.log("Creating reeds");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aReeds, 5, 10, 0, 4), new SimpleObject(aLillies, 5, 10, 0, 4)], true),
	0,
	stayClasses(clWater, 1),
	scaleByMapSize(400,2000), 100);
Engine.SetProgress(40);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBumpWater, 2),
	stayClasses(clWater, 2),
	scaleByMapSize(50, 100));

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.7);
var types = [
	[[tForestFloor, tGrass, pForestD], [tForestFloor, pForestD]],
	[[tForestFloor, tGrass, pForestP], [tForestFloor, pForestP]]
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
		avoidClasses(clPlayer, 20, clWater, 0, clForest, 10),
		num);
Engine.SetProgress(50);

g_Map.log("Creating mud patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, Infinity),
		[
			new LayeredPainter([tGrassA, tGrassB, tMud], [1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clWater, 1, clForest, 0, clDirt, 5, clPlayer, 8),
		scaleByMapSize(15, 45));

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10)],
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10)],
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5)],
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(60);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0),
	scaleByMapSize(16, 262), 50
);

Engine.SetProgress(65);

g_Map.log("Creating large decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0),
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

g_Map.log("Creating deer");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clFood, 13),
	6 * numPlayers, 50
);

g_Map.log("Creating horse");
group = new SimpleGroup(
	[new SimpleObject(oHorse, 1,3, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clFood, 13),
	3 * numPlayers, 50
);

Engine.SetProgress(75);

g_Map.log("Creating rabbit");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 5,7, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clFood, 13),
	6 * numPlayers, 50
);

g_Map.log("Creating wolf");
group = new SimpleGroup(
	[new SimpleObject(oWolf, 1,3, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clFood, 13),
	3 * numPlayers, 50
);

g_Map.log("Creating berry bush");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

Engine.SetProgress(80);

createStragglerTrees(
	[oOak, oBeech],
	avoidClasses(clForest, 1, clPlayer, 13, clMetal, 6, clRock, 6, clWater, 0),
	clForest,
	stragglerTrees);

Engine.SetProgress(85);

g_Map.log("Creating small grass tufts");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aGrassShort, 1, 2, 0, 1)]),
	0,
	avoidClasses(clWater, 2, clPlayer, 13, clDirt, 0),
	scaleByMapSize(13, 200));

Engine.SetProgress(90);

g_Map.log("Creating large grass tufts");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aGrass, 2, 4, 0, 1.8),
			new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)
		]),
	0,
	avoidClasses(clWater, 3, clPlayer, 13, clDirt, 1, clForest, 0),
	scaleByMapSize(13, 200));

Engine.SetProgress(95);

g_Map.log("Creating bushes");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aBushMedium, 1, 2, 0, 2),
			new SimpleObject(aBushSmall, 2, 4, 0, 2)
		]),
	0,
	avoidClasses(clWater, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSkySet("cirrus");
setWaterColor(0.753,0.635,0.345);				// muddy brown
setWaterTint(0.161,0.514,0.635);				// clear blue for blueness
setWaterMurkiness(0.8);
setWaterWaviness(1.0);
setWaterType("clap");

setFogThickness(0.25);
setFogFactor(0.6);

setPPEffect("hdr");
setPPSaturation(0.44);
setPPBloom(0.3);

g_Map.ExportMap();
