Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

const tGrassA = "savanna_shrubs_a_wetseason";
const tGrassB = "savanna_shrubs_a";
const tCliff = "savanna_cliff_a";
const tHill = "savanna_grass_a_wetseason";
const tMud = "savanna_mud_a";
const tShoreBlend = "savanna_grass_b_wetseason";
const tShore = "savanna_riparian_wet";
const tWater = "savanna_mud_a";
const tCityTile = "savanna_tile_a";

const oBush = "gaia/flora_bush_temperate";
const oBaobab = "gaia/flora_tree_baobab";
const oToona = "gaia/flora_tree_toona";
const oBerryBush = "gaia/flora_bush_berry";
const oGazelle = "gaia/fauna_gazelle";
const oZebra = "gaia/fauna_zebra";
const oWildebeest = "gaia/fauna_wildebeest";
const oLion = "gaia/fauna_lion";
const oRhino = "gaia/fauna_rhino";
const oCrocodile = "gaia/fauna_crocodile";
const oElephant = "gaia/fauna_elephant_north_african";
const oElephantInfant = "gaia/fauna_elephant_african_infant";
const oLioness = "gaia/fauna_lioness";
const oRabbit = "gaia/fauna_rabbit";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

const aGrass = "actor|props/flora/grass_field_lush_tall.xml";
const aGrass2 = "actor|props/flora/grass_tropic_field_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aReeds2 = "actor|props/flora/reeds_pond_lush_b.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aBushMedium = "actor|props/flora/bush_tropic_b.xml";
const aBushSmall = "actor|props/flora/bush_tropic_a.xml";
const aShrub = "actor|props/flora/shrub_tropic_plant_flower.xml";
const aFlower = "actor|props/flora/flower_bright.xml";
const aPalm = "actor|props/flora/shrub_fanpalm.xml";

const heightMarsh = -2;
const heightLand = 3;
const heightHill = 15;

const heightOffsetBump1 = 2;
const heightOffsetBump2 = 1;

var g_Map = new RandomMap(heightLand, tShoreBlend);

const numPlayers = getNumPlayers();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.35)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tCityTile,
		"innerTerrain": tCityTile
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
		"template": oBaobab
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(15);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.6, 0.1, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump1, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800));

g_Map.log("Creating hills");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 0),
	scaleByMapSize(1, 4) * numPlayers * 3);

g_Map.log("Creating marshes");
for (let i = 0; i < 2; ++i)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(6, 12)), Math.floor(scaleByMapSize(15, 60)), 0.8),
		[
			new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
			new SmoothElevationPainter(ELEVATION_SET, heightMarsh, 3),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 25, clWater, Math.round(scaleByMapSize(7, 16) * randFloat(0.8, 1.35))),
		scaleByMapSize(4, 20));

g_Map.log("Creating reeds");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aReeds, 20, 40, 0, 4),
			new SimpleObject(aReeds2, 20, 40, 0, 4),
			new SimpleObject(aLillies, 10, 30, 0, 4)
		],
		true),
	0,
	stayClasses(clWater, 1),
	scaleByMapSize(400, 1000),
	100);
Engine.SetProgress(40);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump2, 2),
	stayClasses(clWater, 2),
	scaleByMapSize(50, 100));

g_Map.log("Creating mud patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(2, Math.floor(scaleByMapSize(3, 6)), size, Infinity),
		[
			new LayeredPainter([tGrassA, tGrassB, tMud], [1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clWater, 1, clHill, 0, clDirt, 5, clPlayer, 8),
		scaleByMapSize(15, 45));
Engine.SetProgress(50);

g_Map.log("Creating stone mines");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oStoneSmall, 0, 2, 0, 4),
			new SimpleObject(oStoneLarge, 1, 1, 0, 4)
		],
		true,
		clRock),
	0,
	[avoidClasses(clWater, 0, clPlayer, 20, clRock, 10, clHill, 1)],
	scaleByMapSize(4, 16),
	100);

createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
	0,
	[avoidClasses(clWater, 0, clPlayer, 20, clRock, 10, clHill, 1)],
	scaleByMapSize(4, 16),
	100);

g_Map.log("Creating metal mines");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[avoidClasses(clWater, 0, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1)],
	scaleByMapSize(4, 16),
	100);
Engine.SetProgress(60);

g_Map.log("Creating small decorative rocks");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aRockMedium, 1, 3, 0, 1)], true),
	0,
	avoidClasses(clPlayer, 1),
	scaleByMapSize(16, 262),
	50);
Engine.SetProgress(65);

g_Map.log("Creating large decorative rocks");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aRockLarge, 1, 2, 0, 1),
			new SimpleObject(aRockMedium, 1, 3, 0, 2)
		],
		true),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131),
	50);
Engine.SetProgress(70);

g_Map.log("Creating lions");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oLion, 0, 1, 0, 4),
			new SimpleObject(oLioness, 2, 3, 0, 4)
		],
		true,
		clFood),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11, clHill, 1),
	scaleByMapSize(4, 12),
	50);

g_Map.log("Creating zebras");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oZebra, 4, 6, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

g_Map.log("Creating wildebeest");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oWildebeest, 2, 4, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

g_Map.log("Creating crocodiles");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oCrocodile, 2, 3, 0, 4)],
		true,
		clFood),
	0,
	[avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 13), stayClasses(clWater, 3)],
	5 * numPlayers,
	200);

g_Map.log("Creating gazelles");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oGazelle, 4, 6, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);
Engine.SetProgress(75);

g_Map.log("Creating rabbits");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oRabbit, 6, 8, 0, 2)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	6 * numPlayers,
	50);

g_Map.log("Creating rhinos");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oRhino, 1, 1, 0, 2)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

g_Map.log("Creating elephants");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oElephant, 2, 3, 0, 4), new SimpleObject(oElephantInfant, 1, 1, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

g_Map.log("Creating berry bushes");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2,
	50);
Engine.SetProgress(80);

createStragglerTrees(
	[oToona, oBaobab, oBush, oBush],
	avoidClasses(clForest, 1, clWater, 1, clHill, 1, clPlayer, 13, clMetal, 4, clRock, 4),
	clForest,
	scaleByMapSize(60, 500));
Engine.SetProgress(85);

g_Map.log("Creating small grass tufts");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aGrassShort, 1, 2, 0, 1, -Math.PI / 8, Math.PI / 8)]),
	0,
	avoidClasses(clWater, 2, clPlayer, 13, clDirt, 0),
	scaleByMapSize(13, 200));
Engine.SetProgress(90);

g_Map.log("Creating large grass tufts");
createObjectGroupsDeprecated(
	new SimpleGroup([
		new SimpleObject(aGrass, 2, 4, 0, 1.8, -Math.PI / 8, Math.PI / 8),
		new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5, -Math.PI / 8, Math.PI / 8)
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

g_Map.log("Creating flowering shrubs");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aShrub, 1, 1, 0, 2)]),
	0,
	avoidClasses(clWater, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

g_Map.log("Creating decorative palms");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aPalm, 1, 3, 0, 2)]),
	0,
	avoidClasses(clWater, 2, clPlayer, 12, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

g_Map.log("Creating shrubs,flowers and other decorations");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aFlower, 0, 6, 0, 2),
			new SimpleObject(aGrass2, 2, 5, 0, 2)
		]),
	0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("cirrus");
setWaterColor(0.553, 0.635, 0.345);
setWaterTint(0.161, 0.514, 0.635);
setWaterMurkiness(0.8);
setWaterWaviness(1.0);
setWaterType("clap");

setFogThickness(0.25);
setFogFactor(0.6);

setPPEffect("hdr");
setPPSaturation(0.44);
setPPBloom(0.3);

g_Map.ExportMap();
