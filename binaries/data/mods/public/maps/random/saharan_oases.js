Engine.LoadLibrary("rmgen");

const tPrimary = "desert_sand_dunes_100";
const tCity = "desert_city_tile";
const tCityPlaza = "desert_city_tile_plaza";
const tFineSand = "desert_sand_smooth";
const tDirt1 = "desert_dirt_rough_2";
const tSandDunes = "desert_sand_dunes_50";
const tDirt2 = "desert_dirt_rough";
const tDirtCracks = "desert_dirt_cracks";
const tShore = "desert_shore_stones";
const tWaterDeep = "desert_shore_stones_wet";
const tLush = "desert_grass_a";
const tSLush = "desert_grass_a_sand";

const oGrapeBush = "gaia/flora_bush_grapes";
const oCamel = "gaia/fauna_camel";
const oGazelle = "gaia/fauna_gazelle";
const oGoat = "gaia/fauna_goat";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oDatePalm = "gaia/flora_tree_date_palm";
const oSDatePalm = "gaia/flora_tree_cretan_date_palm_short";
const oWoodTreasure = "gaia/special_treasure_wood";
const oFoodTreasure = "gaia/special_treasure_food_bin";

const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_medit_sm_dry.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

const pForest = [tLush + TERRAIN_SEPARATOR + oDatePalm, tLush + TERRAIN_SEPARATOR + oSDatePalm, tLush];

const heightLand = 1;
const heightOffsetOasis = -3;

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clTreasure = g_Map.createTileClass();

var [playerIDs, playerPosition, playerAngle] = playerPlacementCircle(fractionToTiles(0.35));

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
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
		"template": oSDatePalm
	},
	"Decoratives": {
		"template": aBush1
	}
});
Engine.SetProgress(30);

g_Map.log("Creating oases");
var oasisRadius = fractionToTiles(scaleByMapSize(0.19, 0.22));
for (let i = 0; i < numPlayers; ++i)
{
	let position = Vector2D.add(mapCenter, new Vector2D(oasisRadius, 0).rotate(-playerAngle[i]));
	createArea(
		new ClumpPlacer(diskArea(scaleByMapSize(16, 60)) * 0.185, 0.6, 0.15, 0, position),
		[
			new LayeredPainter(
				[tSLush ,[tLush, pForest], [tLush, pForest], tShore, tShore, tWaterDeep],
				[2, 2, 1, 3, 1]),
			new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetOasis, 10),
			new TileClassPainter(clWater)
		]);
}
Engine.SetProgress(50);

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tDirt1, tSandDunes], [tSandDunes, tDirt2], [tDirt2, tDirt1]],
				[1, 1]
			),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clPlayer, 0, clWater, 1, clDirt, 5),
		scaleByMapSize(15, 45));
Engine.SetProgress(55);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tDirt2, tDirtCracks], [tDirt2, tFineSand], [tDirtCracks, tFineSand]],
				[1, 1]
			),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 0, clWater, 1),
		scaleByMapSize(15, 45));
Engine.SetProgress(60);

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 26, clRock, 10, clWater, 1),
	2*scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 26, clRock, 10, clWater, 1),
	2*scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 26, clMetal, 10, clRock, 5, clWater, 1),
	2*scaleByMapSize(4,16), 100
);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0),
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating shrubs");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2), new SimpleObject(aBush4, 1,2, 0,1), new SimpleObject(aBush3, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clPlayer, 0),
	scaleByMapSize(10, 100), 50
);

g_Map.log("Creating small decorative rocks on mines");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	stayClasses(clRock, 0),
	5*scaleByMapSize(16, 262), 50
);

group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	stayClasses(clMetal, 0),
	5*scaleByMapSize(16, 262), 50
);

g_Map.log("Creating gazelles");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	6*scaleByMapSize(5,20), 50
);

g_Map.log("Creating goats");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	5*scaleByMapSize(5,20), 50
);

g_Map.log("Creating treasures");
group = new SimpleGroup([new SimpleObject(oFoodTreasure, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	3*scaleByMapSize(5,20), 50
);

group = new SimpleGroup([new SimpleObject(oWoodTreasure, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	3*scaleByMapSize(5,20), 50
);

g_Map.log("Creating camels");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 14, 5),
	5*scaleByMapSize(5,20), 50
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2, clTreasure, 2));

setSkySet("sunny");
setSunColor(0.746, 0.718, 0.539);
setWaterColor(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1.0);
setWaterType("clap");
setWaterMurkiness(0.5);

g_Map.ExportMap();
