Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

if (g_MapSettings.Biome)
	setSelectedBiome();
else
	setBiome("generic/savanna");


// Pick some biome defaults and overload a few settings.
var tPrimary = g_Terrains.mainTerrain;
var tForestFloor = g_Terrains.forestFloor1;
var tCliff = ["savanna_cliff_a", "savanna_cliff_a_red", "savanna_cliff_b", "savanna_cliff_b_red"];
var tSecondary = g_Terrains.tier4Terrain;
var tGrassShrubs = g_Terrains.tier1Terrain;
var tDirt = g_Terrains.dirt;
var tDirt2 = "savanna_dirt_a_red";
var tDirt3 = g_Terrains.dirt;
var tDirt4 = g_Terrains.hill;
var tCitytiles = "savanna_tile_a";
var tShore = g_Terrains.shore;
var tWater = g_Terrains.water;

var oBaobab = g_Gaia.tree1;
var oPalm = g_Gaia.tree2;
var oPalm2 = g_Gaia.tree3;
var oBerryBush = "gaia/fruit/berry_01";
var oWildebeest = "gaia/fauna_wildebeest";
var oZebra = "gaia/fauna_zebra";
var oRhino = "gaia/fauna_rhinoceros_white";
var oLion = "gaia/fauna_lion";
var oLioness = "gaia/fauna_lioness";
var oHawk = "birds/buzzard";
var oGiraffe = "gaia/fauna_giraffe";
var oGiraffe2 = "gaia/fauna_giraffe_infant";
var oGazelle = "gaia/fauna_gazelle";
var oElephant = "gaia/fauna_elephant_african_bush";
var oElephant2 = "gaia/fauna_elephant_african_infant";
var oCrocodile = "gaia/fauna_crocodile_nile";
var oFish = g_Gaia.fish;
var oStoneLarge = g_Gaia.stoneLarge;
var oStoneSmall = g_Gaia.stoneSmall;
var oMetalLarge = g_Gaia.metalLarge;
var oMetalSmall = g_Gaia.metalSmall;

var aBush = g_Decoratives.bushMedium;
var aRock = g_Decoratives.rockMedium;

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPalm, tForestFloor + TERRAIN_SEPARATOR + oPalm2, tForestFloor];

var heightSeaGround = -5;
var heightLand = 2;
var heightCliff = 3;

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();

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
		"innerTerrain": tCitytiles
	},
	"StartingAnimal": {
	},
	"Berries": {
		"template": oBerryBush
	},
	"Mines": {
		"types": [
			{
				"template": oMetalLarge
			},
			{
				"type": "stone_formation",
				"template": oStoneSmall,
				"terrain": tDirt4
			}
		]
	},
	"Trees": {
		"template": oBaobab,
		"count": scaleByMapSize(3, 12),
		"minDistGroup": 2,
		"maxDistGroup": 6,
		"minDist": 15,
		"maxDist": 16
	}
	// No decoratives
});
Engine.SetProgress(20);

// The specificity of this map is a bunch of watering holes & hills making it somewhat cluttered.
const nbHills = scaleByMapSize(6, 16);
const nbWateringHoles = scaleByMapSize(4, 10);
{
	g_Map.log("Creating hills");
	createHills([tDirt2, tCliff, tGrassShrubs], avoidClasses(clPlayer, 30, clHill, 15), clHill, nbHills);
	Engine.SetProgress(30);

	g_Map.log("Creating water holes");
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), Math.floor(scaleByMapSize(60, 100)), Infinity),
		[
			new LayeredPainter([tShore, tWater], [1]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 7),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 22, clWater, 8, clHill, 2),
		nbWateringHoles);

	Engine.SetProgress(45);

	paintTerrainBasedOnHeight(heightCliff, Infinity, 0, tCliff);
}

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

g_Map.log("Creating forests");

createDefaultForests(
	[tPrimary, pForest, tForestFloor, pForest, pForest],
	avoidClasses(clPlayer, 20, clForest, 15, clHill, 0, clWater, 2),
	clForest,
	scaleByMapSize(500, 3000));
Engine.SetProgress(50);

g_Map.log("Creating dirt patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tDirt, tDirt3], [tDirt2, tDirt4]],
	[2],
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(55);

g_Map.log("Creating shrubs");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tGrassShrubs,
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(60);

g_Map.log("Creating grass patches");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tSecondary,
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(65);

g_Map.log("Creating metal mines");
createBalancedMetalMines(
	oMetalSmall,
	oMetalLarge,
	clMetal,
	avoidClasses(clWater, 4, clForest, 1, clPlayer, scaleByMapSize(20, 35), clHill, 1)
);

g_Map.log("Creating stone mines");
createBalancedStoneMines(
	oStoneSmall,
	oStoneLarge,
	clRock,
	avoidClasses(clWater, 4, clForest, 1, clPlayer, scaleByMapSize(20, 35), clHill, 1, clMetal, 10)
);

Engine.SetProgress(70);

createDecoration(
	[
		[new SimpleObject(aBush, 1,3, 0,1)],
		[new SimpleObject(aRock, 1,2, 0,1)]
	],
	[
		scaleByMapAreaAbsolute(8),
		scaleByMapAreaAbsolute(8)
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0));
Engine.SetProgress(75);

// Roaming animals
{
	var placeRoaming = function(name, objs)
	{
		g_Map.log("Creating roaming " + name);
		const group = new SimpleGroup(objs, true, clFood);
		createObjectGroups(group, 0,
			avoidClasses(clWater, 3, clPlayer, 20, clFood, 11, clHill, 4),
			scaleByMapSize(3, 9), 50
		);
	};

	placeRoaming("giraffes", [new SimpleObject(oGiraffe, 2, 4, 0, 4), new SimpleObject(oGiraffe2, 0, 2, 0, 4)]);
	placeRoaming("elephants", [new SimpleObject(oElephant, 2, 4, 0, 4), new SimpleObject(oElephant2, 0, 2, 0, 4)]);
	placeRoaming("lions", [new SimpleObject(oLion, 0, 1, 0, 4), new SimpleObject(oLioness, 2, 3, 0, 4)]);

	// Other roaming animals
	createFood(
		[
			[new SimpleObject(oHawk, 1, 1, 0, 3)],
			[new SimpleObject(oGazelle, 3, 5, 0, 3)],
			[new SimpleObject(oZebra, 3, 5, 0, 3)],
			[new SimpleObject(oWildebeest, 4, 6, 0, 3)],
			[new SimpleObject(oRhino, 1, 1, 0, 3)]
		],
		[
			3 * numPlayers,
			3 * numPlayers,
			3 * numPlayers,
			3 * numPlayers,
			3 * numPlayers,
		],
		avoidClasses(clFood, 20, clWater, 5, clHill, 2, clPlayer, 16),
		clFood);
}

// Animals that hang around watering holes
{
	// TODO: these have a high retry factor because our mapgen constraint logic is bad.
	var placeWateringHoleAnimals = function(name, objs, numberOfGroups)
	{
		g_Map.log("Creating " + name + " around watering holes");
		const group = new SimpleGroup(objs, true, clFood);
		createObjectGroups(group, 0,
			borderClasses(clWater, 6, 3),
			numberOfGroups, 50
		);
	};
	placeWateringHoleAnimals(
		"crocodiles",
		[new SimpleObject(oCrocodile, 2, 3, 0, 3)],
		nbWateringHoles * 0.8
	);
	placeWateringHoleAnimals(
		"zebras",
		[new SimpleObject(oZebra, 2, 5, 0, 3)],
		nbWateringHoles
	);
	placeWateringHoleAnimals(
		"gazelles",
		[new SimpleObject(oGazelle, 2, 5, 0, 3)],
		nbWateringHoles
	);
}


g_Map.log("Creating other food sources");
createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	avoidClasses(clWater, 3, clForest, 2, clPlayer, 20, clHill, 3, clFood, 10),
	clFood);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		15 * numPlayers
	],
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	clFood);
Engine.SetProgress(85);


g_Map.log("Creating straggler baobabs");
const group = new SimpleGroup([new SimpleObject(oBaobab, 1, 3, 0, 3)], true, clForest);
createObjectGroups(
	group,
	0,
	avoidClasses(clWater, 0, clForest, 2, clHill, 3, clPlayer, 12, clMetal, 4, clRock, 4),
	scaleByMapSize(15, 75)
);


placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

// Adjust some biome settings;

setSkySet("sunny");
setWaterType("clap");

g_Map.ExportMap();
