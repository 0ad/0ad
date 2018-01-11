Engine.LoadLibrary("rmgen");

const tCity = "desert_city_tile";
const tCityPlaza = "desert_city_tile_plaza";
const tSand = "desert_dirt_rough";
const tDunes = "desert_sand_dunes_100";
const tFineSand = "desert_sand_smooth";
const tCliff = ["desert_cliff_badlands", "desert_cliff_badlands_2"];
const tForestFloor = "desert_forestfloor_palms";
const tGrass = "desert_grass_a";
const tGrassSand25 = "desert_grass_a_stones";
const tDirt = "desert_dirt_rough";
const tShore = "desert_shore_stones";
const tWaterDeep = "desert_shore_stones_wet";

const oBerryBush = "gaia/flora_bush_grapes";
const oCamel = "gaia/fauna_camel";
const oFish = "gaia/fauna_fish";
const oGazelle = "gaia/fauna_gazelle";
const oGiraffe = "gaia/fauna_giraffe";
const oGoat = "gaia/fauna_goat";
const oWildebeest = "gaia/fauna_wildebeest";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oDatePalm = "gaia/flora_tree_date_palm";
const oSDatePalm = "gaia/flora_tree_senegal_date_palm";

const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_dry_a.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

// terrain + entity (for painting)
const pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];
const pForestOasis = [tGrass + TERRAIN_SEPARATOR + oDatePalm, tGrass + TERRAIN_SEPARATOR + oSDatePalm, tGrass];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill1 = createTileClass();
var clForest = createTileClass();
var clPatch = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var [playerIDs, playerX, playerZ] = playerPlacementCircle(0.35);

for (let i = 0; i < numPlayers; ++i)
	createArea(
		new ClumpPlacer(
			diskArea(defaultPlayerBaseRadius()),
			0.9,
			0.5,
			10,
			Math.round(fractionToTiles(playerX[i])),
			Math.round(fractionToTiles(playerZ[i]))),
		paintClass(clPlayer));

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerX, playerZ],
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

log("Creating dune patches...");
placer = new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, 0);
painter = new TerrainPainter(tDunes);
createAreas(placer, [painter, paintClass(clPatch)],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(5, 20)
);
Engine.SetProgress(15);

log("Creating sand patches...");
var placer = new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0);
var painter = new TerrainPainter([tSand, tFineSand]);
createAreas(placer, [painter, paintClass(clPatch)],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(15, 50)
);
Engine.SetProgress(20);

log("Creating dirt patches...");
placer = new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0);
painter = new TerrainPainter([tDirt]);
createAreas(placer, [painter, paintClass(clPatch)],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(15, 50)
);
Engine.SetProgress(25);

log("Creating oasis...");
var oRadius = scaleByMapSize(14, 40);
createArea(
	new ClumpPlacer(diskArea(oRadius), 0.6, 0.15, 0, mapSize / 2, mapSize / 2),
	[
		new LayeredPainter([[tSand, pForest], [tGrassSand25, pForestOasis], tGrassSand25, tShore, tWaterDeep], [2, 3, 1, 1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, -11, 8),
		paintClass(clForest)
	],
	null);

Engine.SetProgress(30);

log("Creating oasis wildlife...");
var num = Math.round(PI * oRadius / 8);
var constraint = new AndConstraint([borderClasses(clForest, 0, 3), avoidClasses(clForest, 0)]);
var halfSize = mapSize/2;
for (var i = 0; i < num; ++i)
{
	var r = 0;
	var angle = 2 * Math.PI / num * i;
	do {
		// Work outward until constraint met
		var gx = Math.round(halfSize + r * cos(angle));
		var gz = Math.round(halfSize + r * sin(angle));
		++r;
	} while (!constraint.allows(gx,gz) && r < halfSize);

	createObjectGroup(
		new RandomGroup(
			[	new SimpleObject(oGiraffe, 2,4, 0,3),
				new SimpleObject(oWildebeest, 3,5, 0,3),
				new SimpleObject(oGazelle, 5,7, 0,3)
			],
			true,
			clFood,
			gx,
			gz),
		0);
}

constraint = new AndConstraint([borderClasses(clForest, 15, 0), avoidClasses(clFood, 5)]);
num = Math.round(PI * oRadius / 16);
for (var i = 0; i < num; ++i)
{
	var r = 0;
	var angle = 2 * Math.PI / num * i;
	do {
		// Work outward until constraint met
		var gx = Math.round(halfSize + r * cos(angle));
		var gz = Math.round(halfSize + r * sin(angle));
		++r;
	} while (!constraint.allows(gx,gz) && r < halfSize);

	group = new SimpleGroup(
		[new SimpleObject(oFish, 1,1, 0,1)],
		true, clFood, gx, gz
	);
	createObjectGroup(group, 0);
}
Engine.SetProgress(35);

log("Creating level 1 hills...");
var hillAreas = createAreas(
	new ClumpPlacer(scaleByMapSize(50,300), 0.25, 0.1, 0.5),
	[
		new LayeredPainter([tCliff, tSand], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1),
		paintClass(clHill1)
	],
	avoidClasses(clForest, 3, clPlayer, 0, clHill1, 10),
	scaleByMapSize(10,20), 100
);
Engine.SetProgress(40);

log("Creating small level 1 hills...");
hillAreas = hillAreas.concat(
	createAreas(
		new ClumpPlacer(scaleByMapSize(25,150), 0.25, 0.1, 0.5),
		[
			new LayeredPainter([tCliff, tSand], [1]),
			new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1),
			paintClass(clHill1)
		],
		avoidClasses(clForest, 3, clPlayer, 0, clHill1, 3),
		scaleByMapSize(15,25),
		100));

Engine.SetProgress(45);

log("Creating decorative rocks...");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup(
		[new RandomObject([aDecorativeRock, aBush2, aBush3], 3, 8, 0, 2)],
		true),
	0,
	borderClasses(clHill1, 0, 3),
	scaleByMapSize(40,200), 50,
	hillAreas);

Engine.SetProgress(50);

log("Creating level 2 hills...");
createAreasInAreas(
	new ClumpPlacer(scaleByMapSize(25, 150), 0.25, 0.1, 0),
	[
		new LayeredPainter([tCliff, tSand], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1)
	],
	[stayClasses(clHill1, 0)],
	scaleByMapSize(15, 25),
	50,
	hillAreas);

Engine.SetProgress(55);

log("Creating level 3 hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(12, 75), 0.25, 0.1, 0),
	[
		new LayeredPainter([tCliff, tSand], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1)
	],
	[stayClasses(clHill1, 0)],
	scaleByMapSize(15,25),
	50
);
Engine.SetProgress(60);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 0),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clForest, 0, clPlayer, 0, clHill1, 2),
	scaleByMapSize(100, 200)
);

Engine.SetProgress(65);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.5);
var num = scaleByMapSize(10,30);
placer = new ClumpPlacer(forestTrees / num, 0.15, 0.1, 0.5);
painter = new TerrainPainter([tSand, pForest]);
createAreas(placer, [painter, paintClass(clForest)],
	avoidClasses(clPlayer, 1, clForest, 10, clHill1, 1),
	num, 50
);

Engine.SetProgress(70);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4), new RandomObject(aBushes, 2, 4, 0, 2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill1, 1)],
	scaleByMapSize(4,16), 100
);

group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill1, 1)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill1, 1)],
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(80);

log("Creating gazelles...");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

log("Creating camels...");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);
Engine.SetProgress(85);

createStragglerTrees(
	[oDatePalm, oSDatePalm],
	avoidClasses(clForest, 0, clHill1, 1, clPlayer, 4, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);
Engine.SetProgress(90);

log("Creating bushes...");
group = new SimpleGroup([new RandomObject(aBushes, 2,3, 0,2)]);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill1, 1, clPlayer, 0, clForest, 0),
	scaleByMapSize(16, 262)
);

log("Creating more decorative rocks...");
group = new SimpleGroup([new SimpleObject(aDecorativeRock, 1,2, 0,2)]);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill1, 1, clPlayer, 0, clForest, 0),
	scaleByMapSize(16, 262)
);

setWaterColor(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1.0);
setWaterType("clap");
setWaterMurkiness(0.75);
setWaterHeight(20);

ExportMap();
