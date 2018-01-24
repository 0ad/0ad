Engine.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;

const tCity = "medit_city_pavement";
const tCityPlaza = "medit_city_pavement";
const tHill = ["medit_grass_shrubs", "medit_rocks_grass_shrubs", "medit_rocks_shrubs", "medit_rocks_grass", "medit_shrubs"];
const tMainDirt = "medit_dirt";
const tCliff = "medit_cliff_aegean";
const tForestFloor = "medit_grass_shrubs";
const tGrass = "medit_grass_field";
const tGrassSand50 = "medit_grass_field_a";
const tGrassSand25 = "medit_grass_field_b";
const tDirt = "medit_dirt_b";
const tDirt2 = "medit_rocks_grass";
const tDirt3 = "medit_rocks_shrubs";
const tDirtCracks = "medit_dirt_c";
const tShoreUpper = "medit_sand";
const tShoreLower = "medit_sand_wet";
const tCoralsUpper = "medit_sea_coral_plants";
const tCoralsLower = "medit_sea_coral_deep";
const tSeaDepths = "medit_sea_depths";

const oBerryBush = "gaia/flora_bush_berry";
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

const pForest = [tForestFloor, tForestFloor + TERRAIN_SEPARATOR + oCarob, tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];

const heightSeaGround = -3;
const heightSeaBump = -2.5;
const heightCorralsLower = -2;
const heightCorralsUpper = -1.5;
const heightShore = 1;
const heightLand = 2;
const heightIsland = 6;

var g_Map = new RandomMap(heightShore, tHill);

const numPlayers = getNumPlayers();
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

var startAngle = randomAngle();

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(startAngle , fractionToTiles(0.6)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tCityPlaza,
		"innerTerrain": tCity
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
		"template": oCarob,
		"count": 2
	},
	"Decoratives": {
		"template": aBush1
	}
});
Engine.SetProgress(30);

paintRiver({
	"parallel": false,
	"start": new Vector2D(mapCenter.x, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapCenter.x, mapBounds.bottom).rotateAround(startAngle, mapCenter),
	"width": fractionToTiles(0.35),
	"fadeDist": scaleByMapSize(6, 25),
	"deviation": 0,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightLand,
	"meanderShort": 20,
	"meanderLong": 0
});

paintTileClassBasedOnHeight(-Infinity, 0.7, Elevation_ExcludeMin_ExcludeMax, clWater);

paintTerrainBasedOnHeight(-Infinity, heightShore, Elevation_ExcludeMin_ExcludeMax, tShoreLower);
paintTerrainBasedOnHeight(heightShore, heightLand, Elevation_ExcludeMin_ExcludeMax, tShoreUpper);
Engine.SetProgress(40);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
 [tForestFloor, tForestFloor, tForestFloor, pForest, pForest],
 avoidClasses(clPlayer, 20, clForest, 17, clWater, 2, clBaseResource, 3),
 clForest,
 forestTrees);

Engine.SetProgress(50);

if (randBool())
	createHills([tGrass, tCliff, tHill], avoidClasses(clPlayer, 20, clForest, 1, clHill, 15, clWater, 3), clHill, scaleByMapSize(3, 15));
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clForest, 1, clHill, 15, clWater, 3), clHill, scaleByMapSize(3, 15));

log("Creating grass patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassSand50],[tGrassSand50,tGrassSand25], [tGrassSand25,tGrass]],
 [1,1],
 avoidClasses(clForest, 0, clGrass, 2, clPlayer, 10, clWater, 2, clDirt, 2, clHill, 1),
 scaleByMapSize(15, 45),
 clDirt);

Engine.SetProgress(55);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [tDirt3, tDirt2,[tDirt,tMainDirt], [tDirtCracks,tMainDirt]],
 [1,1,1],
 avoidClasses(clForest, 0, clDirt, 2, clPlayer, 10, clWater, 2, clGrass, 2, clHill, 1),
 scaleByMapSize(15, 45),
 clDirt);

Engine.SetProgress(60);

log("Creating undersea bumps...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	new SmoothElevationPainter(ELEVATION_SET, heightSeaBump, 3),
	stayClasses(clWater, 6),
	scaleByMapSize(10, 50));

log("Creating islands...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(30, 80)), 0.5),
	[
		new LayeredPainter([tShoreLower, tShoreUpper, tHill], [2 ,1]),
		new SmoothElevationPainter(ELEVATION_SET, heightIsland, 4),
		paintClass(clIsland)
	],
	[avoidClasses(clPlayer, 8, clForest, 1, clIsland, 15), stayClasses (clWater, 6)],
	scaleByMapSize(1, 4) * numPlayers
);

paintTerrainBasedOnHeight(-Infinity, heightSeaGround, Elevation_IncludeMin_IncludeMax, tSeaDepths);
paintTerrainBasedOnHeight(heightSeaGround, heightCorralsLower, Elevation_ExcludeMin_IncludeMax, tCoralsLower);
paintTerrainBasedOnHeight(heightCorralsLower, heightCorralsUpper, Elevation_ExcludeMin_IncludeMax, tCoralsUpper);

log("Creating island stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 stayClasses(clIsland, 4),
 clRock);

log("Creating island metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 stayClasses(clIsland, 4),
 clMetal
);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clHill, 1),
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clHill, 1),
 clMetal
);

Engine.SetProgress(65);

createDecoration(
	[
		[
			new SimpleObject(aDecorativeRock, 1, 3, 0, 1)
		],
		[
			new SimpleObject(aBush2, 1, 2, 0, 1),
			new SimpleObject(aBush1, 1, 3, 0, 2),
			new SimpleObject(aBush4, 1, 2, 0, 1),
			new SimpleObject(aBush3, 1, 3, 0, 2)
		]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(40, 360)
	],
	avoidClasses(clWater, 2, clForest, 0, clPlayer, 0, clHill, 1));

Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		3 * scaleByMapSize(5, 20)
	],
	[avoidClasses(clIsland, 2, clFood, 10), stayClasses(clWater, 5)],
	clFood);

createFood(
	[
		[new SimpleObject(oSheep, 5, 7, 0, 4)],
		[new SimpleObject(oGoat, 2, 4, 0, 3)],
		[new SimpleObject(oDeer, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20)
	],
	avoidClasses(clForest, 0, clPlayer, 8, clBaseResource, 4, clWater, 1, clFood, 10, clHill, 1),
	clFood);

createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);

Engine.SetProgress(90);

var types = [oDatePalm, oSDatePalm, oCarob, oFanPalm, oPoplar, oCypress];
createStragglerTrees(
	types,
	avoidClasses(clForest, 1, clWater, 2, clPlayer, 12, clMetal, 6, clHill, 1),
	clForest,
	stragglerTrees);

createStragglerTrees(
	types,
	stayClasses(clIsland, 4),
	clForest,
	stragglerTrees * 10);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clIsland, 10, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("cumulus");
setSunColor(0.866667, 0.776471, 0.486275);
setWaterColor(0, 0.501961, 1);
setWaterTint(0.501961, 1, 1);
setWaterWaviness(4.0);
setWaterType("ocean");
setWaterMurkiness(0.49);

setFogFactor(0.3);
setFogThickness(0.25);

setPPEffect("hdr");
setPPContrast(0.62);
setPPSaturation(0.51);
setPPBloom(0.12);

g_Map.ExportMap();
