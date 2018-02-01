Engine.LoadLibrary("rmgen");

var tPrimary = "desert_sand_dunes_100";
var tCity = "desert_city_tile";
var tCityPlaza = "desert_city_tile_plaza";
var tFineSand = "desert_sand_smooth";
var tForestFloor = "desert_forestfloor_palms";
var tGrass = "desert_dirt_rough_2";
var tGrassSand50 = "desert_sand_dunes_50";
var tGrassSand25 = "desert_dirt_rough";
var tDirt = "desert_dirt_rough";
var tDirtCracks = "desert_dirt_cracks";
var tShore = "desert_sand_wet";
var tLush = "desert_grass_a";
var tSLush = "desert_grass_a_sand";
var tSDry = "desert_plants_b";

var oBerryBush = "gaia/flora_bush_berry";
var oCamel = "gaia/fauna_camel";
var oGazelle = "gaia/fauna_gazelle";
var oGoat = "gaia/fauna_goat";
var oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
var oStoneSmall = "gaia/geology_stone_desert_small";
var oMetalLarge = "gaia/geology_metal_desert_slabs";
var oDatePalm = "gaia/flora_tree_date_palm";
var oSDatePalm = "gaia/flora_tree_cretan_date_palm_short";
var eObelisk = "other/obelisk";
var ePyramid = "gaia/ruins/pyramid_minor";
var oWoodTreasure = "gaia/treasure/wood";
var oFoodTreasure = "gaia/treasure/food_bin";

var aBush1 = "actor|props/flora/bush_desert_a.xml";
var aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
var aBush3 = "actor|props/flora/bush_medit_sm_dry.xml";
var aBush4 = "actor|props/flora/plant_desert_a.xml";
var aDecorativeRock = "actor|geology/stone_desert_med.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
var aLillies = "actor|props/flora/water_lillies.xml";

var pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];

const heightLand = 1;
const heightShore = 2;
const heightPonds = -7;
const heightSeaGround = -3;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightLand, tPrimary);

const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var aPlants = mapSize < 256 ?
	"actor|props/flora/grass_tropical.xml" :
	"actor|props/flora/grass_tropic_field_tall.xml";

var numPlayers = getNumPlayers();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clGrass = g_Map.createTileClass();
var clDesert = g_Map.createTileClass();
var clPond = g_Map.createTileClass();
var clShore = g_Map.createTileClass();
var clTreasure = g_Map.createTileClass();

var desertWidth = fractionToTiles(0.25);
var startAngle = randomAngle();

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(startAngle, fractionToTiles(0.4)),
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
		"template": oDatePalm,
		"count": 2
	},
	"Decoratives": {
		"template": aBush1
	}
});
Engine.SetProgress(30);

const riverTextures = [
	{
		"left": fractionToTiles(0),
		"right": fractionToTiles(0.04),
		"terrain": tLush,
		"tileClass": clShore
	},
	{
		"left": fractionToTiles(0.04),
		"right": fractionToTiles(0.06),
		"terrain": tSLush,
		"tileClass": clShore
	},
	{
		"left": fractionToTiles(0.06),
		"right": fractionToTiles(0.09),
		"terrain": tSDry,
		"tileClass": clShore
	},
	{
		"left": fractionToTiles(0.25),
		"right": fractionToTiles(0.5),
		"tileClass": clDesert
	}
];

const plantFrequency = 2;
var plantID = 0;

paintRiver({
	"parallel": true,
	"start": new Vector2D(mapCenter.x, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapCenter.x, mapBounds.bottom).rotateAround(startAngle, mapCenter),
	"width": fractionToTiles(0.1),
	"fadeDist": scaleByMapSize(3, 12),
	"deviation": 0.5,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightShore,
	"meanderShort": 12,
	"meanderLong": 50,
	"waterFunc": (position, height, riverFraction) => {

		clWater.add(position);
		createTerrain(tShore).place(position);

		// Place river bushes
		if (height <= -0.2 || height >= 0.1)
			return;

		if (plantID % plantFrequency == 0)
		{
			plantID = 0;
			g_Map.placeEntityAnywhere(aPlants, 0, position, randomAngle());
		}
		++plantID;
	},
	"landFunc": (position, shoreDist1, shoreDist2) => {

		for (let riv of riverTextures)
			if (riv.left < +shoreDist1 && +shoreDist1 < riv.right ||
			    riv.left < -shoreDist2 && -shoreDist2 < riv.right)
			{
				riv.tileClass.add(position);

				if (riv.terrain)
					createTerrain(riv.terrain).place(position);
			}
	}
});
Engine.SetProgress(40);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	avoidClasses(clWater, 2, clPlayer, 6),
	scaleByMapSize(100, 200));

g_Map.log("Creating ponds");
var numLakes = Math.round(scaleByMapSize(1, 4) * numPlayers / 2);
var waterAreas = createAreas(
	new ClumpPlacer(scaleByMapSize(2, 5) * 50, 0.8, 0.1, 10),
	[
		new LayeredPainter([tShore, tShore, tShore], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, heightPonds, 4),
		new TileClassPainter(clPond)
	],
	avoidClasses(clPlayer, 25, clWater, 20, clPond, 10),
	numLakes);

g_Map.log("Creating reeds");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup([new SimpleObject(aReeds, 1, 3, 0, 1)], true),
	0,
	stayClasses(clPond, 1),
	numLakes,
	100,
	waterAreas);

g_Map.log("Creating lillies");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup([new SimpleObject(aLillies, 1, 3, 0, 1)], true),
	0,
	stayClasses(clPond, 1),
	numLakes,
	100,
	waterAreas);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(700, 3500, 0.5);
var num = scaleByMapSize(10,30);
createAreas(
	new ClumpPlacer(forestTrees / num, 0.15, 0.1, 0.5),
	[
		new TerrainPainter([pForest, tForestFloor]),
		new TileClassPainter(clForest)
	],
	avoidClasses(clPlayer, 19, clForest, 4, clWater, 1, clDesert, 5, clPond, 2, clBaseResource, 3),
	num,
	50);

Engine.SetProgress(50);

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tGrass, tGrassSand50], [tGrassSand50, tGrassSand25], [tGrassSand25, tGrass]],
				[1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 10, clWater, 1, clDirt, 5, clShore, 1, clPond, 1),
		scaleByMapSize(15, 45));
Engine.SetProgress(55);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tDirt, tDirtCracks], [tDirt, tFineSand], [tDirtCracks, tFineSand]],
				[1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 10, clWater, 1, clGrass, 5, clShore, 1, clPond, 1),
		scaleByMapSize(15, 45));

Engine.SetProgress(60);

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating stone mines");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

Engine.SetProgress(65);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clPond, 1),
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating shrubs");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2), new SimpleObject(aBush4, 1,2, 0,1), new SimpleObject(aBush3, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clPlayer, 0, clPond, 1),
	scaleByMapSize(20, 180), 50
);
Engine.SetProgress(70);

g_Map.log("Creating gazelles");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

g_Map.log("Creating goats");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

g_Map.log("Creating treasures");
group = new SimpleGroup([new SimpleObject(oFoodTreasure, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

group = new SimpleGroup([new SimpleObject(oWoodTreasure, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

g_Map.log("Creating camels");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clTreasure, 2, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

Engine.SetProgress(90);

createStragglerTrees(
	[oDatePalm, oSDatePalm],
	avoidClasses(clForest, 0, clWater, 1, clPlayer, 20, clMetal, 6, clDesert, 1, clTreasure, 2, clPond, 1),
	clForest,
	stragglerTrees / 2);

createStragglerTrees(
	[oDatePalm, oSDatePalm],
	avoidClasses(clForest, 0, clWater, 1, clPlayer, 20, clMetal, 6, clTreasure, 2),
	clForest,
	stragglerTrees / 10);

createStragglerTrees(
	[oDatePalm, oSDatePalm],
	borderClasses(clPond, 1, 4),
	clForest,
	stragglerTrees);

g_Map.log("Creating obelisks");
group = new SimpleGroup(
	[new SimpleObject(eObelisk, 1,1, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clWater, 4, clForest, 3, clPlayer, 20, clMetal, 6, clRock, 2, clPond, 4, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(5, 30), 50
);

g_Map.log("Creating pyramids");
group = new SimpleGroup(
	[new SimpleObject(ePyramid, 1,1, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clWater, 7, clForest, 6, clPlayer, 20, clMetal, 5, clRock, 5, clPond, 7, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(2, 6), 50
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSkySet("sunny");
setSunColor(0.711, 0.746, 0.574);
setWaterColor(0.541,0.506,0.416);
setWaterTint(0.694,0.592,0.522);
setWaterMurkiness(1);
setWaterWaviness(3.0);
setWaterType("lake");

g_Map.ExportMap();
