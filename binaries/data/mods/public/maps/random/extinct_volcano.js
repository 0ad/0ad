Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

const tHillDark = "cliff volcanic light";
const tHillMedium1 = "ocean_rock_a";
const tHillMedium2 = "ocean_rock_b";
const tHillVeryDark = ["cliff volcanic coarse", "cave_walls"];
const tRoad = "road1";
const tRoadWild = "road1";
const tForestFloor1 = tHillMedium1;
const tForestFloor2 = tHillMedium2;
const tGrassA = "cliff volcanic light";
const tGrassB = "ocean_rock_a";
const tGrassPatchBlend = "temp_grass_long_b";
const tGrassPatch = ["temp_grass_d", "temp_grass_clovers"];
const tShoreBlend = "cliff volcanic light";
const tShore = "ocean_rock_a";
const tWater = "ocean_rock_b";

const oTree = "gaia/flora_tree_dead";
const oTree2 = "gaia/flora_tree_euro_beech";
const oTree3 = "gaia/flora_tree_oak";
const oTree4 = "gaia/flora_tree_oak_dead";
const oBush = "gaia/flora_bush_temperate";
const oFruitBush = "gaia/flora_bush_berry";
const oRabbit = "gaia/fauna_rabbit";
const oGoat = "gaia/fauna_goat";
const oBear = "gaia/fauna_bear";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";
const oTower = "other/palisades_rocks_fort";

const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_tempe_me.xml";
const aBushSmall = "actor|props/flora/bush_tempe_sm.xml";
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRain = "actor|particle/rain_shower.xml";

const pForestD = [
	tForestFloor1 + TERRAIN_SEPARATOR + oTree,
	tForestFloor2 + TERRAIN_SEPARATOR + oTree2,
	tForestFloor1
];

const pForestP = [
	tForestFloor1 + TERRAIN_SEPARATOR + oTree3,
	tForestFloor2 + TERRAIN_SEPARATOR + oTree4,
	tForestFloor1
];

var heightSeaGround = -4;
var heightLand = 1;
var heightHill = 18;
var heightPlayerHill = 25;

var g_Map = new RandomMap(heightLand, tHillMedium1);

var numPlayers = getNumPlayers();
var mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clGrass = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clBumps = g_Map.createTileClass();
var clTower = g_Map.createTileClass();
var clRain = g_Map.createTileClass();

var playerMountainSize = defaultPlayerBaseRadius();

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

g_Map.log("Creating CC mountains");
if (!isNomad())
	for (let i = 0; i < numPlayers; ++i)
	{
		// This one consists of many bumps, creating an omnidirectional ramp
		createMountain(
			heightPlayerHill,
			playerMountainSize,
			playerMountainSize,
			Math.floor(scaleByMapSize(4, 10)),
			undefined,
			playerPosition[i].x,
			playerPosition[i].y,
			tHillDark,
			clPlayer,
			14);

		// Flatten the initial CC area
		createArea(
			new ClumpPlacer(diskArea(playerMountainSize), 0.95, 0.6, Infinity, playerPosition[i]),
			[
				new LayeredPainter([tHillVeryDark, tHillMedium1], [playerMountainSize]),
				new SmoothElevationPainter(ELEVATION_SET, heightPlayerHill, playerMountainSize),
				new TileClassPainter(clPlayer)
			]);
	}

Engine.SetProgress(8);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	// PlayerTileClass already marked above
	"BaseResourceClass": clBaseResource,
	"Walls": "towers",
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
	},
	"Berries": {
		"template": oFruitBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTree2
	}
	// No decoratives
});
Engine.SetProgress(15);

createVolcano(mapCenter, clHill, tHillVeryDark, undefined, false, ELEVATION_SET);
Engine.SetProgress(20);

g_Map.log("Creating lakes");
createAreas(
	new ChainPlacer(5, 6, Math.floor(scaleByMapSize(10, 14)), 0.1),
	[
		new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 3),
		new TileClassPainter(clWater)
	],
	avoidClasses(clPlayer, 0, clHill, 2, clWater, 12),
	Math.round(scaleByMapSize(6, 12)));
Engine.SetProgress(25);

createBumps(avoidClasses(clPlayer, 0, clHill, 0), scaleByMapSize(50, 300), 1, 10, 3, 0, scaleByMapSize(4, 10));
paintTileClassBasedOnHeight(10, 100, 0, clBumps);
Engine.SetProgress(30);

g_Map.log("Creating hills");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, Infinity),
	[
		new LayeredPainter([tHillDark, tHillDark, tHillDark], [2, 2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 0, clHill, 15, clWater, 2, clBaseResource, 2),
	scaleByMapSize(2, 8) * numPlayers);
Engine.SetProgress(35);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(1200, 3000, 0.7);
var types = [
	[[tGrassB, tGrassA, pForestD], [tGrassB, pForestD]],
	[[tGrassB, tGrassA, pForestP], [tGrassB, pForestP]]
];
var size = forestTrees / (scaleByMapSize(4, 12) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(forestTrees / num, 0.1, 0.1, Infinity),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(
			clPlayer, 4,
			clForest, 10,
			clHill, 0,
			clWater, 2),
		num);
Engine.SetProgress(40);

g_Map.log("Creating hill patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	for (let type of [[tHillMedium1, tHillDark], [tHillDark, tHillMedium2], [tHillMedium1, tHillMedium2]])
		createAreas(
			new ClumpPlacer(size, 0.3, 0.06, 0.5),
			[
				new LayeredPainter(type, [1]),
				new TileClassPainter(clGrass)
			],
			avoidClasses(
				clWater, 3,
				clForest, 0,
				clHill, 0,
				clBumps, 0,
				clPlayer, 0),
			scaleByMapSize(20, 80));
Engine.SetProgress(45);

g_Map.log("Creating grass patches");
createLayeredPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	[tGrassPatchBlend, tGrassPatch],
	[1],
	avoidClasses(
		clWater, 1,
		clForest, 0,
		clHill, 0,
		clGrass, 5,
		clBumps, 0,
		clPlayer, 0),
	scaleByMapSize(3, 8),
	clDirt);
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
	[
		stayClasses(clBumps, 1),
		avoidClasses(
			clWater, 3,
			clForest, 1,
			clPlayer, 0,
			clRock, 15,
			clHill, 0)
	],
	100,
	100);
Engine.SetProgress(55);

g_Map.log("Creating small stone quarries");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
	0,
	[
		stayClasses(clBumps, 1),
		avoidClasses(
			clWater, 3,
			clForest, 1,
			clPlayer, 0,
			clRock, 15,
			clHill, 0)
	],
	100,
	100);
Engine.SetProgress(60);

g_Map.log("Creating metal mines");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[
		stayClasses(clBumps, 1),
		avoidClasses(
			clWater, 3,
			clForest, 1,
			clPlayer, 0,
			clMetal, 15,
			clRock, 10,
			clHill, 0)
	],
	100,
	100);
Engine.SetProgress(65);

if (!isNomad())
{
	g_Map.log("Creating towers");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oTower, 1, 1, 0, 4)], true, clTower),
		0,
		[
			stayClasses(clBumps, 3),
			avoidClasses(
				clMetal, 5,
				clRock, 5,
				clHill, 0,
				clTower, 60,
				clPlayer, 10,
				clForest, 2)
		],
		500,
		1);
}
Engine.SetProgress(67);

createDecoration(
		[
			[
				new SimpleObject(aGrassShort, 1, 2, 0, 1)
			],
			[
				new SimpleObject(aGrass, 2, 4, 0, 1.8),
				new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)
			],
			[
				new SimpleObject(aBushMedium, 1, 2, 0, 2),
				new SimpleObject(aBushSmall, 2, 4, 0, 2)
			]
		],
		[
			scaleByMapSize(15, 200),
			scaleByMapSize(15, 200),
			scaleByMapSize(15, 200)
		],
		[
			stayClasses(clGrass, 0),
			avoidClasses(
				clWater, 0,
				clForest, 0,
				clPlayer, 0,
				clHill, 0)
		]);
Engine.SetProgress(70);

createDecoration(
		[
			[
				new SimpleObject(aRockMedium, 1, 3, 0, 1)
			],
			[
				new SimpleObject(aRockLarge, 1, 2, 0, 1),
				new SimpleObject(aRockMedium, 1, 3, 0, 2)
			]
		],
		[
			scaleByMapSize(15, 250),
			scaleByMapSize(15, 150)
		],
		avoidClasses(
			clWater, 0,
			clForest, 0,
			clPlayer, 0,
			clHill, 0
		));
Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oRabbit, 5, 7, 2, 4)],
		[new SimpleObject(oGoat, 3, 5, 2, 4)]
	],
	[
		scaleByMapSize(1, 6) * numPlayers,
		scaleByMapSize(3, 10) * numPlayers
	],
	[
		avoidClasses(
			clWater, 1,
			clForest, 0,
			clPlayer, 0,
			clHill, 1,
			clFood, 20)
	],
	clFood);
Engine.SetProgress(78);

createFood(
	[
		[new SimpleObject(oBear, 1, 1, 0, 2)]
	],
	[
		3 * numPlayers
	],
	[
		avoidClasses(
			clWater, 1,
			clForest, 0,
			clPlayer, 0,
			clHill, 1,
			clFood, 20
		),
		stayClasses(clForest, 2)
	],
	clFood);
Engine.SetProgress(81);

createFood(
	[
		[new SimpleObject(oFruitBush, 1, 2, 0, 4)]
	],
	[
		3 * numPlayers
	],
	[stayClasses(clGrass, 1), avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 1, clFood, 10)],
	clFood);
Engine.SetProgress(85);

createStragglerTrees(
	[oTree, oTree2, oTree3, oTree4, oBush],
	[
		stayClasses(clGrass, 1),
		avoidClasses(
			clWater, 5,
			clForest, 1,
			clHill, 1,
			clPlayer, 0,
			clMetal, 4,
			clRock, 4)
	],
	clForest,
	stragglerTrees);

Engine.SetProgress(90);

g_Map.log("Creating straggler bushes");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBush, 1, 3, 0, 3)],
		true,
		clForest
	),
	0,
	[
		stayClasses(clGrass, 3),
		avoidClasses(
			clWater, 1,
			clForest, 1,
			clPlayer, 0,
			clMetal, 4,
			clRock, 4)
	],
	stragglerTrees);
Engine.SetProgress(95);

g_Map.log("Creating rain drops");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(aRain, 2, 2, 1, 4)],
		true,
		clRain),
	0,
	avoidClasses(clRain, 5),
	scaleByMapSize(80, 250));
Engine.SetProgress(98);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clHill, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("rain");
setWaterType("lake");
setWaterWaviness(2);
setWaterColor(0.1, 0.13, 0.15);
setWaterTint(0.058, 0.05, 0.035);
setWaterMurkiness(0.9);

setPPEffect("hdr");

g_Map.ExportMap();
