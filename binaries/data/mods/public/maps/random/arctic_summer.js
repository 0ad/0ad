Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

setFogThickness(0.46);
setFogFactor(0.5);

setPPEffect("hdr");
setPPSaturation(0.48);
setPPContrast(0.53);
setPPBloom(0.12);

var tPrimary = ["alpine_grass_rocky"];
var tForestFloor = "alpine_grass";
var tCliff = ["polar_cliff_a", "polar_cliff_b", "polar_cliff_snow"];
var tSecondary = "alpine_grass";
var tHalfSnow = ["polar_grass_snow", "ice_dirt"];
var tSnowLimited = ["polar_snow_rocks", "polar_ice"];
var tDirt = "ice_dirt";
var tShore = "alpine_shore_rocks";
var tWater = "polar_ice_b";
var tHill = "polar_ice_cracked";

var oBush = "gaia/flora_bush_badlands";
var oBush2 = "gaia/flora_bush_temperate";
var oBerryBush = "gaia/flora_bush_berry";
var oRabbit = "gaia/fauna_rabbit";
var oMuskox = "gaia/fauna_muskox";
var oDeer = "gaia/fauna_deer";
var oWolf = "gaia/fauna_wolf";
var oWhaleFin = "gaia/fauna_whale_fin";
var oWhaleHumpback = "gaia/fauna_whale_humpback";
var oFish = "gaia/fauna_fish";
var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_alpine_slabs";
var oWoodTreasure = "gaia/treasure/wood";

var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oBush, tForestFloor + TERRAIN_SEPARATOR + oBush2, tForestFloor];

var heightSeaGround = -5;
var heightLand = 2;

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
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
		"outerTerrain": tPrimary,
		"innerTerrain": tSecondary
	},
	"Chicken": {
		"template": oRabbit
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
	"Treasures": {
		"types": [
			{
				"template": oWoodTreasure,
				"count": 10
			}
		]
	},
	"Trees": {
		"template": oBush,
		"count": 20,
		"maxDistGroup": 3
	}
	// No decoratives
});
Engine.SetProgress(20);

createHills(
	[tPrimary, tCliff, tHill],
	avoidClasses(
		clPlayer, 35,
		clForest, 20,
		clHill, 50,
		clWater, 2),
	clHill,
	scaleByMapSize(1, 240));

Engine.SetProgress(30);

g_Map.log("Creating lakes");
createAreas(
	new ChainPlacer(
		1,
		Math.floor(scaleByMapSize(4, 8)),
		Math.floor(scaleByMapSize(40, 180)),
		Infinity),
	[
		new LayeredPainter([tShore, tWater], [1]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 5),
		new TileClassPainter(clWater)
	],
	avoidClasses(clPlayer, 15),
	scaleByMapSize(1, 20));

Engine.SetProgress(45);

createBumps(avoidClasses(clPlayer, 6, clWater, 2), scaleByMapSize(30, 300), 1, 8, 4, 0, 3);

paintTerrainBasedOnHeight(4, 15, 0, tCliff);
paintTerrainBasedOnHeight(15, 100, 3, tSnowLimited);

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
	[tSecondary, tForestFloor, tForestFloor, pForest, pForest],
	avoidClasses(
		clPlayer, 20,
		clForest, 14,
		clHill, 20,
		clWater, 2),
	clForest,
	forestTrees);
Engine.SetProgress(60);

g_Map.log("Creating dirt patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tDirt,tHalfSnow], [tHalfSnow,tSnowLimited]],
	[2],
	avoidClasses(
		clWater, 3,
		clForest, 0,
		clHill, 0,
		clDirt, 5,
		clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);

g_Map.log("Creating shrubs");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tSecondary,
	avoidClasses(
		clWater, 3,
		clForest, 0,
		clHill, 0,
		clDirt, 5,
		clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);

g_Map.log("Creating grass patches");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tSecondary,
	avoidClasses(
		clWater, 3,
		clForest, 0,
		clHill, 0,
		clDirt, 5,
		clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(65);

g_Map.log("Creating stone mines");
createMines(
	[
		[new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oStoneSmall, 2, 5, 1, 3)]
	],
	avoidClasses(
		clWater, 3,
		clForest, 1,
		clPlayer, 20,
		clRock, 18,
		clHill, 1),
	clRock);

g_Map.log("Creating metal mines");
createMines(
	[
		[new SimpleObject(oMetalLarge, 1, 1, 0, 4)]
	],
	avoidClasses(
		clWater, 3,
		clForest, 1,
		clPlayer, 20,
		clMetal, 18,
		clRock, 5,
		clHill, 1),
	clMetal);
Engine.SetProgress(70);

createDecoration(
	[
		[
			new SimpleObject(aRockMedium, 1, 3, 0, 1)
		],
		[
			new SimpleObject(aRockLarge, 1, 2, 0, 1),
			new SimpleObject(aRockMedium, 1, 3, 0, 1)
		]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
	],
	avoidClasses(
		clWater, 0,
		clForest, 0,
		clPlayer, 0,
		clHill, 0));
Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oWolf, 3, 5, 0, 3)],
		[new SimpleObject(oRabbit, 6, 8, 0, 6)],
		[new SimpleObject(oDeer, 3, 4, 0, 3)],
		[new SimpleObject(oMuskox, 3, 4, 0, 3)]
	],
	[
		3 * numPlayers,
		3 * numPlayers,
		3 * numPlayers,
		3 * numPlayers
	],
	avoidClasses(
		clFood, 20,
		clHill, 5,
		clWater, 5),
	clFood);

createFood(
	[
		[new SimpleObject(oWhaleFin, 1, 1, 0, 3)],
		[new SimpleObject(oWhaleHumpback, 1, 1, 0, 3)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	[
		avoidClasses(clFood, 20),
		stayClasses(clWater, 6)
	],
	clFood);

createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	avoidClasses(
		clWater, 3,
		clForest, 0,
		clPlayer, 20,
		clHill, 1,
		clFood, 10),
	clFood);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		15 * numPlayers
	],
	[
		avoidClasses(clFood, 20),
		stayClasses(clWater, 6)
	],
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oBush, oBush2],
	avoidClasses(
		clWater, 5,
		clForest, 3,
		clHill, 1,
		clPlayer, 12,
		clMetal, 4,
		clRock, 4),
	clForest,
	stragglerTrees);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clWater, 4, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("sunset 1");
setSunRotation(randomAngle());
setSunColor(0.8, 0.7, 0.6);
setTerrainAmbientColor(0.7, 0.6, 0.7);
setUnitsAmbientColor(0.6, 0.5, 0.6);
setSunElevation(Math.PI * randFloat(1/12, 1/7));
setWaterColor(0, 0.047, 0.286);
setWaterTint(0.462, 0.756, 0.866);
setWaterMurkiness(0.92);
setWaterWaviness(1);
setWaterType("clap");

g_Map.ExportMap();
