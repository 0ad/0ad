Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

if (g_MapSettings.Biome)
	setSelectedBiome();
else
	setBiome("generic/sahara");


const tPrimary = g_Terrains.mainTerrain;
const tCity = g_Terrains.roadWild;
const tCityPlaza = g_Terrains.road;
const tSand = g_Terrains.tier3Terrain;
const tDunes = g_Terrains.tier4Terrain;
const tFineSand = g_Terrains.tier2Terrain;
const tCliff = g_Terrains.cliff;
const tDirt = g_Terrains.dirt;
const tForestFloor = "desert_forestfloor_palms";
const tGrass = g_Terrains.forestFloor2;
const tGrassSand25 = g_Terrains.shoreBlend;
const tShore = g_Terrains.shore;
const tWaterDeep = g_Terrains.water;

const oBerryBush = "gaia/fruit/grapes";
const oCamel = "gaia/fauna_camel";
const oFish = g_Gaia.fish;
const oGazelle = "gaia/fauna_gazelle";
const oGiraffe = "gaia/fauna_giraffe";
const oGoat = "gaia/fauna_goat";
const oWildebeest = "gaia/fauna_wildebeest";
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oMetalSmall = g_Gaia.metalSmall;
const oOasisTree = "gaia/tree/senegal_date_palm";
const oDatePalm = g_Gaia.tree1;
const oSDatePalm = g_Gaia.tree2;

const aBush1 = g_Decoratives.bushMedium;
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = g_Decoratives.bushSmall;
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = g_Decoratives.rockMedium;

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];
const pForestOasis = [tGrass + TERRAIN_SEPARATOR + oOasisTree, tGrass + TERRAIN_SEPARATOR + oDatePalm, tGrass];

const heightLand = 10;

const heightOffsetOasis = -11;
const heightOffsetHill1 = 16;
const heightOffsetHill2 = 16;
const heightOffsetHill3 = 16;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill1 = g_Map.createTileClass();
var clOasis = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clPatch = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

var oasisRadius = scaleByMapSize(14, 40);

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

if (!isNomad())
	for (let i = 0; i < numPlayers; ++i)
		createArea(
			new ClumpPlacer(diskArea(defaultPlayerBaseRadius()), 0.9, 0.5, Infinity, playerPosition[i]),
			new TileClassPainter(clPlayer));

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tCity,
		"innerTerrain": tCityPlaza,
		"width": 3,
		"radius": 10
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
		"template": oDatePalm
	}
	// No decoratives
});
Engine.SetProgress(10);

g_Map.log("Creating dune patches");
createAreas(
	new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, 0),
	[
		new TerrainPainter(tDunes),
		new TileClassPainter(clPatch)
	],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(5, 20));
Engine.SetProgress(15);

g_Map.log("Creating sand patches");
createAreas(
	new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0),
	[
		new TerrainPainter([tSand, tFineSand]),
		new TileClassPainter(clPatch)
	],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(15, 50));
Engine.SetProgress(20);

g_Map.log("Creating dirt patches");
createAreas(
	new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0),
	[
		new TerrainPainter([tDirt]),
		new TileClassPainter(clPatch)
	],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(15, 50));
Engine.SetProgress(25);

g_Map.log("Creating oasis");
createArea(
	new ClumpPlacer(diskArea(oasisRadius), 0.6, 0.15, 0, mapCenter),
	[
		new LayeredPainter([[tSand, pForest], [tGrassSand25, pForestOasis], tGrassSand25, tShore, tWaterDeep], [2, 3, 1, 1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetOasis, 8),
		new TileClassPainter(clOasis)
	]);

Engine.SetProgress(30);

g_Map.log("Creating oasis wildlife");
var num = Math.round(Math.PI * oasisRadius / 8);
var constraint = new AndConstraint([borderClasses(clOasis, 0, 3), avoidClasses(clOasis, 0)]);
for (var i = 0; i < num; ++i)
{
	let animalPosition;
	let r = 0;
	let angle = 2 * Math.PI / num * i;
	do {
		// Work outward until constraint met
		animalPosition = Vector2D.add(mapCenter, new Vector2D(r, 0).rotate(-angle)).round();
		++r;
	} while (!constraint.allows(animalPosition) && r < mapSize / 2);

	createObjectGroup(
		new RandomGroup(
			[
				new SimpleObject(oGiraffe, 2, 4, 0, 3),
				new SimpleObject(oWildebeest, 3,5, 0,3),
				new SimpleObject(oGazelle, 5,7, 0,3)
			],
			true,
			clFood,
			animalPosition),
		0);
}

g_Map.log("Creating oasis fish");
constraint = new AndConstraint([borderClasses(clOasis, 15, 0), avoidClasses(clFood, 5)]);
num = Math.round(Math.PI * oasisRadius / 16);
for (var i = 0; i < num; ++i)
{
	let fishPosition;
	var r = 0;
	var angle = 2 * Math.PI / num * i;
	do {
		// Work outward until constraint met
		fishPosition = Vector2D.add(mapCenter, new Vector2D(r, 0).rotate(-angle));
		++r;
	} while (!constraint.allows(fishPosition) && r < mapSize / 2);

	createObjectGroup(new SimpleGroup([new SimpleObject(oFish, 1, 1, 0, 1)], true, clFood, fishPosition), 0);
}
Engine.SetProgress(35);

g_Map.log("Creating level 1 hills");
var hillAreas = createAreas(
	new ClumpPlacer(scaleByMapSize(50,300), 0.25, 0.1, 0.5),
	[
		new LayeredPainter([tCliff, tSand], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetHill1, 1),
		new TileClassPainter(clHill1)
	],
	avoidClasses(clOasis, 3, clPlayer, 0, clHill1, 10),
	scaleByMapSize(10,20), 100
);
Engine.SetProgress(40);

g_Map.log("Creating small level 1 hills");
hillAreas = hillAreas.concat(
	createAreas(
		new ClumpPlacer(scaleByMapSize(25,150), 0.25, 0.1, 0.5),
		[
			new LayeredPainter([tCliff, tSand], [1]),
			new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetHill2, 1),
			new TileClassPainter(clHill1)
		],
		avoidClasses(clOasis, 3, clPlayer, 0, clHill1, 3),
		scaleByMapSize(15,25),
		100));

Engine.SetProgress(45);

g_Map.log("Creating decorative rocks");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup(
		[new RandomObject([aDecorativeRock, aBush2, aBush3], 3, 8, 0, 2)],
		true),
	0,
	borderClasses(clHill1, 0, 3),
	scaleByMapSize(40,200), 50,
	hillAreas);

Engine.SetProgress(50);

g_Map.log("Creating level 2 hills");
createAreasInAreas(
	new ClumpPlacer(scaleByMapSize(25, 150), 0.25, 0.1, 0),
	[
		new LayeredPainter([tCliff, tSand], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetHill2, 1)
	],
	[stayClasses(clHill1, 0)],
	scaleByMapSize(15, 25),
	50,
	hillAreas);

Engine.SetProgress(55);

g_Map.log("Creating level 3 hills");
createAreas(
	new ClumpPlacer(scaleByMapSize(12, 75), 0.25, 0.1, 0),
	[
		new LayeredPainter([tCliff, tSand], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetHill3, 1)
	],
	[stayClasses(clHill1, 0)],
	scaleByMapSize(15,25),
	50
);
Engine.SetProgress(60);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 0),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	avoidClasses(clOasis, 0, clPlayer, 0, clHill1, 2),
	scaleByMapSize(100, 200)
);

Engine.SetProgress(65);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.5);
var num = g_DefaultNumberOfForests;
createAreas(
	new ClumpPlacer(forestTrees / num, 0.15, 0.1, 0.5),
	[
		new TerrainPainter([tSand, pForest]),
		new TileClassPainter(clForest)
	],
	avoidClasses(clPlayer, 1, clOasis, 10, clForest, 10, clHill1, 1),
	num,
	50);

Engine.SetProgress(70);
g_Map.log("Creating metal mines");
createBalancedMetalMines(
	oMetalSmall,
	oMetalLarge,
	clMetal,
	avoidClasses(clOasis, 2, clForest, 0, clPlayer, scaleByMapSize(15, 25), clHill1, 1)
);

g_Map.log("Creating stone mines");
createBalancedStoneMines(
	oStoneSmall,
	oStoneLarge,
	clRock,
	avoidClasses(clOasis, 2, clForest, 0, clPlayer, scaleByMapSize(15, 25), clHill1, 1, clMetal, 10)
);
Engine.SetProgress(80);

g_Map.log("Creating gazelles");
let group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 1, clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

g_Map.log("Creating goats");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 1, clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

g_Map.log("Creating camels");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 1, clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);
Engine.SetProgress(85);

createStragglerTrees(
	[oDatePalm, oSDatePalm],
	avoidClasses(clOasis, 1, clForest, 0, clHill1, 1, clPlayer, 4, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);
Engine.SetProgress(90);

g_Map.log("Creating bushes");
group = new SimpleGroup([new RandomObject(aBushes, 2,3, 0,2)]);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 1, clHill1, 1, clPlayer, 0, clForest, 0),
	scaleByMapSize(16, 262)
);

g_Map.log("Creating more decorative rocks");
group = new SimpleGroup([new SimpleObject(aDecorativeRock, 1,2, 0,2)]);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 1, clHill1, 1, clPlayer, 0, clForest, 0),
	scaleByMapSize(16, 262)
);

placePlayersNomad(clPlayer, avoidClasses(clOasis, 4, clForest, 1, clMetal, 4, clRock, 4, clHill1, 4, clFood, 2));

setWaterWaviness(1.0);
setWaterType("clap");
setWaterHeight(20);

g_Map.ExportMap();
