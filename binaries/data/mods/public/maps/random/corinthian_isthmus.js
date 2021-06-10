Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

TILE_CENTERED_HEIGHT_MAP = true;

const tCity = g_Terrains.road;
const tCityPlaza = g_Terrains.roadWild;
const tHill = g_Terrains.hill;
const tMainDirt = g_Terrains.dirt;
const tCliff = g_Terrains.cliff;
const tForestFloor = g_Terrains.forestFloor1;
const tGrass = g_Terrains.mainTerrain;
const tGrassSand50 = g_Terrains.tier1Terrain;
const tGrassSand25 = g_Terrains.tier3Terrain;
const tDirt = g_Terrains.dirt;
const tDirt2 = g_Terrains.dirt;
const tDirt3 = g_Terrains.tier2Terrain;
const tDirtCracks = g_Terrains.dirt;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oBerryBush = g_Gaia.fruitBush;
const oDeer = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oSheep = g_Gaia.secondaryHuntableAnimal;
const oGoat = "gaia/fauna_goat";
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oDatePalm = g_Gaia.tree1;
const oSDatePalm = g_Gaia.tree2;
const oCarob = g_Gaia.tree3;
const oFanPalm = g_Gaia.tree4;
const oPoplar = g_Gaia.tree5;
const oCypress = pickRandom([g_Gaia.tree1, g_Gaia.tree2, g_Gaia.tree3, g_Gaia.tree4, g_Gaia.tree5]);

const aBush1 = g_Decoratives.bushSmall;
const aBush2 = g_Decoratives.bushMedium;
const aBush3 = g_Decoratives.grassShort;
const aBush4 = g_Decoratives.tree;
const aDecorativeRock = g_Decoratives.rockMedium;

const aLillies = g_Decoratives.lillies;
const aReeds = g_Decoratives.reeds;

const pForest = [
	tForestFloor, tForestFloor + TERRAIN_SEPARATOR + oCarob,
	tForestFloor + TERRAIN_SEPARATOR + oDatePalm,
	tForestFloor + TERRAIN_SEPARATOR + oSDatePalm,
	tForestFloor
];

var heightSeaGround = -7;
var heightShallow = -0.8;
var heightLand = 3;

var g_Map = new RandomMap(heightLand, tHill);

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
var clGrass = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clPassageway = g_Map.createTileClass();
var clShallow = g_Map.createTileClass();

g_Map.log("Creating the main river");
var riverAngle = randomAngle();
var riverWidth = scaleByMapSize(20, 90);
var riverStart = new Vector2D(mapCenter.x, 0).rotateAround(riverAngle, mapCenter);
var riverEnd = new Vector2D(mapCenter.x, mapSize).rotateAround(riverAngle, mapCenter);

createArea(
	new PathPlacer(riverStart, riverEnd, riverWidth, 0.2, 15 * scaleByMapSize(1, 3), 0.04, 0.01),
	new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4));

g_Map.log("Creating small puddles at the map border to ensure players being separated");
for (const point of [riverStart, riverEnd])
	createArea(
		new ClumpPlacer(diskArea(riverWidth / 2), 0.95, 0.6, Infinity, point),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4));

// Paint water so we now where to lay passageway.
paintTileClassBasedOnHeight(heightSeaGround - 1, 0.5, 1, clWater);

g_Map.log("Creating passage connecting the two riversides");
// Add some leeway to make sure things are connected.
const fraction = randFloat(0.38, 0.62);
const middlePoint = riverStart.clone().mult(fraction).add(riverEnd.clone().mult(1 - fraction));
let passageStart = new Vector2D(middlePoint.x, middlePoint.y - riverWidth * 1.1).rotateAround(riverAngle, middlePoint);
let passageEnd = new Vector2D(middlePoint.x, middlePoint.y + riverWidth * 1.1).rotateAround(riverAngle, middlePoint);
passageStart = passageStart.rotateAround(Math.PI / 2, middlePoint);
passageEnd = passageEnd.rotateAround(Math.PI / 2, middlePoint);
// First create a shallow (walkable) area.
const passageWidth = scaleByMapSize(15, 40);
createArea(
	new PathPlacer(
		passageStart,
		passageEnd,
		passageWidth * 2,
		0.2,
		3 * scaleByMapSize(1, 4),
		0.1,
		0.01,
		100.0),
	new MultiPainter([
		new SmoothElevationPainter(ELEVATION_SET, heightShallow, 3),
		new TileClassPainter(clShallow)]),
	new OrConstraint([
		stayClasses(clWater, 0),
		borderClasses(clWater, 0, 3),
	])
);

// Then create the proper passageway.
createArea(
	new PathPlacer(
		passageStart,
		passageEnd,
		passageWidth,
		0.5,
		3 * scaleByMapSize(1, 4),
		0.1,
		0.01),
	new MultiPainter([
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 3),
		new TileClassPainter(clPassageway)])
);

paintTerrainBasedOnHeight(heightSeaGround - 1, 1, 1, tWater);
paintTerrainBasedOnHeight(1, 2, 1, tShore);
paintTerrainBasedOnHeight(2, 5, 1, tGrass);

// Reset water class.
clWater = g_Map.createTileClass();
paintTileClassBasedOnHeight(heightSeaGround - 1, 0.5, 1, clWater);

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(riverAngle, fractionToTiles(0.6)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": "towers",
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
			{ "template": oStoneSmall },
			{ "template": oStoneSmall },
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
Engine.SetProgress(40);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

const [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createDefaultForests(
	[tForestFloor, tForestFloor, pForest, pForest, pForest],
	avoidClasses(clPlayer, 20, clPassageway, 5, clForest, 17, clWater, 2, clBaseResource, 3),
	clForest,
	forestTrees);

Engine.SetProgress(50);

if (randBool())
	createHills([tGrass, tCliff, tHill], avoidClasses(clPlayer, 20, clPassageway, 10, clForest, 1, clHill, 15, clWater, 3), clHill, scaleByMapSize(3, 15));
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clPassageway, 10, clForest, 1, clHill, 15, clWater, 3), clHill, scaleByMapSize(3, 15));

g_Map.log("Creating grass patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tGrass, tGrassSand50], [tGrassSand50, tGrassSand25], [tGrassSand25, tGrass]],
	[1, 1],
	avoidClasses(clForest, 0, clGrass, 2, clPlayer, 10, clWater, 2, clDirt, 2, clHill, 1),
	scaleByMapSize(15, 45),
	clDirt);

Engine.SetProgress(55);

g_Map.log("Creating dirt patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[tDirt3, tDirt2, [tDirt, tMainDirt], [tDirtCracks, tMainDirt]],
	[1, 1, 1],
	avoidClasses(clForest, 0, clDirt, 2, clPlayer, 10, clWater, 2, clGrass, 2, clHill, 1),
	scaleByMapSize(15, 45),
	clDirt);

Engine.SetProgress(60);

g_Map.log("Creating stone mines in the middle");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oStoneSmall, 1, 4, 0, 4)], true, clRock),
	0,
	[
		avoidClasses(clForest, 4, clPlayer, 15, clRock, 6, clWater, 0, clHill, 4),
		stayClasses(clPassageway, 2)
	],
	scaleByMapSize(4, 15),
	80
);

g_Map.log("Creating metal mines");
createMines(
	[
		[new SimpleObject(oMetalLarge, 1, 1, 0, 4)]
	],
	avoidClasses(clForest, 4, clPassageway, 10, clPlayer, 15, clMetal, 10, clRock, 5, clWater, 4, clHill, 4),
	clMetal,
	scaleByMapSize(5, 20)
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
	avoidClasses(clWater, 2, clForest, 0, clPlayer, 5, clBaseResource, 6, clHill, 1, clRock, 6, clMetal, 6));

Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		3 * scaleByMapSize(5, 20)
	],
	[avoidClasses(clFood, 8, clShallow, 2), stayClasses(clWater, 4)],
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
	avoidClasses(clForest, 0, clPlayer, 10, clPassageway, 0, clBaseResource, 6, clWater, 1, clFood, 10, clHill, 1, clRock, 6, clMetal, 6),
	clFood);

createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPassageway, 0, clPlayer, 20, clHill, 1, clFood, 10, clRock, 6, clMetal, 6),
	clFood);

Engine.SetProgress(90);

createStragglerTrees(
	[oDatePalm, oSDatePalm, oCarob, oFanPalm, oPoplar, oCypress],
	avoidClasses(clForest, 1, clWater, 2, clPlayer, 8, clBaseResource, 6, clMetal, 6, clRock, 6, clHill, 1),
	clForest,
	stragglerTrees);


g_Map.log("Placing decorations on shallows");

createObjectGroups(
	new SimpleGroup([
		new SimpleObject(aDecorativeRock, 1, 2, 0, 1),
		new SimpleObject(aReeds, 0, 4, 0, 1),
	], true, clRock),
	0,
	[stayClasses(clShallow, 2), avoidClasses(clPassageway, 0)],
	scaleByMapSize(30, 100),
	50
);

createObjectGroups(
	new SimpleGroup([new SimpleObject(aLillies, 1, 2, 0, 1)], true, clRock),
	0,
	[stayClasses(clShallow, 2), avoidClasses(clPassageway, 0)],
	scaleByMapSize(6, 36),
	50
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setWaterWaviness(2.5);
setWaterType("ocean");
setWaterMurkiness(0.49);

g_Map.ExportMap();
