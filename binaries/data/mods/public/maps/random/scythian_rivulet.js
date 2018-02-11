Engine.LoadLibrary("rmgen");

const tMainTerrain = "alpine_snow_a";
const tTier1Terrain = "snow rough";
const tTier2Terrain = "snow_01";
const tTier3Terrain = "snow rocks";
const tForestFloor1 = "alpine_forrestfloor_snow";
const tForestFloor2 = "polar_snow_rocks";
const tCliff = ["alpine_cliff_a", "alpine_cliff_b"];
const tHill = "alpine_snow_glacial";
const tRoad = "new_alpine_citytile";
const tRoadWild = "alpine_snow_rocky";
const tShore = "alpine_shore_rocks_icy";
const tWater = "polar_ice_b";

const oTreeDead = "gaia/flora_tree_dead";
const oOak = "gaia/flora_tree_oak_dead";
const oPine = "gaia/flora_tree_pine";
const oGrapes = "gaia/flora_bush_grapes";
const oBush = "gaia/flora_bush_badlands";
const oDeer = "gaia/fauna_deer";
const oRabbit = "gaia/fauna_rabbit";
const oWolf1 = "gaia/fauna_wolf";
const oWolf2 = "gaia/fauna_arctic_wolf";
const oFox = "gaia/fauna_fox_arctic";
const oHawk = "gaia/fauna_hawk";
const oFish = "gaia/fauna_fish";
const oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
const oStoneSmall = "gaia/geology_stone_alpine_a";
const oMetalLarge = "gaia/geology_metal_alpine_slabs";

const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/plant_desert_a.xml";
const aBushSmall = "actor|props/flora/bush_desert_a.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aOutpostPalisade = "actor|props/structures/britons/outpost_palisade.xml";
const aWorkshopChariot= "actor|props/structures/britons/workshop_chariot_01.xml";

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTreeDead, tForestFloor2 + TERRAIN_SEPARATOR + oOak, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTreeDead, tForestFloor1];

var heightSeaGround = -2;
var heightShoreLower = 0.7;
var heightShoreUpper = 1;
var heightLand = 2;
var heightSnowline = 12;
var heightOffsetLargeBumps = 4;

var g_Map = new RandomMap(heightShoreUpper, tMainTerrain);

const numPlayers = getNumPlayers();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRiver = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clShallowsFlora = g_Map.createTileClass();

var riverWidth = fractionToTiles(0.1);

var startAngle = randomAngle();

var [playerIDs, playerPosition] = playerPlacementRiver(startAngle, fractionToTiles(0.6));

if (!isNomad())
	for (let position of playerPosition)
		addCivicCenterAreaToClass(position, clPlayer);

paintRiver({
	"parallel": false,
	"start": new Vector2D(mapCenter.x, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapCenter.x, mapBounds.bottom).rotateAround(startAngle, mapCenter),
	"width": riverWidth,
	"fadeDist": scaleByMapSize(3, 14),
	"deviation": 6,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightLand,
	"meanderShort": 40,
	"meanderLong": 20
});
Engine.SetProgress(10);

paintTileClassBasedOnHeight(-Infinity, heightShoreUpper, Elevation_ExcludeMin_ExcludeMax, clRiver);
Engine.SetProgress(15);

createTributaryRivers(
	startAngle + Math.PI / 2,
	4,
	10,
	heightSeaGround,
	[-Infinity, heightSeaGround],
	Math.PI / 5,
	clWater,
	undefined,
	avoidClasses(clPlayer, 4));

Engine.SetProgress(25);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": avoidClasses(clWater, 4),
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
		"template": oDeer,
		"distance": 18,
		"minGroupDistance": 2,
		"maxGroupDistance": 4,
		"minGroupCount": 2,
		"maxGroupCount": 3
	},
	"Berries": {
		"template": oGrapes,
		"minCount": 3,
		"maxCount": 3
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTreeDead,
		"count": 10
	},
	"Decoratives": {
		"template": aBushSmall,
		"minDist": 10,
		"maxDist": 12
	}
});
Engine.SetProgress(30);

g_Map.log("Creating pools");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(2, 5)), Math.floor(scaleByMapSize(15, 60)), 0.8),
	new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 3),
	avoidClasses(clPlayer, 20),
	scaleByMapSize(6, 20));

Engine.SetProgress(40);

createBumps(avoidClasses(clPlayer, 2));

if (randBool())
	createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 20, clWater, 1, clHill, 15, clRiver, 10), clHill, scaleByMapSize(3, 15));
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clWater, 1, clHill, 15, clRiver, 10), clHill, scaleByMapSize(3, 15));

g_Map.log("Creating large bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetLargeBumps, 3),
	avoidClasses(clPlayer, 2),
	scaleByMapSize(100, 800));

createBumps(avoidClasses(clPlayer, 20));

paintTileClassBasedOnHeight(-Infinity, heightShoreUpper, Elevation_ExcludeMin_ExcludeMax, clWater);
paintTerrainBasedOnHeight(-Infinity, heightShoreUpper, Elevation_ExcludeMin_ExcludeMax, tWater);
paintTerrainBasedOnHeight(heightShoreUpper, heightShoreLower, Elevation_ExcludeMin_ExcludeMax, tShore);
paintTerrainBasedOnHeight(heightSnowline, Infinity, Elevation_ExcludeMin_ExcludeMax, tMainTerrain);

Engine.SetProgress(50);

g_Map.log("Creating dirt patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]],
	[1, 1],
	avoidClasses(clHill, 2, clDirt, 5, clPlayer, 12, clWater, 5, clForest, 4),
	scaleByMapSize(25, 55),
	clDirt);

var [forestTrees, stragglerTrees] = getTreeCounts(200, 1200, 0.7);
createForests(
	[tForestFloor1, tForestFloor2, tForestFloor1, pForest1, pForest2],
	avoidClasses(clPlayer, 20, clWater, 2, clHill, 2, clForest, 12),
	clForest,
	forestTrees);

createStragglerTrees(
	[oTreeDead, oOak, oPine, oBush],
	avoidClasses(clPlayer, 17, clWater, 2, clHill, 2, clForest, 1, clRiver, 4),
	clForest,
	stragglerTrees);

Engine.SetProgress(55);

g_Map.log("Creating stone mines");
// Allow mines on the bumps at the river
createMines(
	[
		[new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)],
		[new SimpleObject(oStoneSmall, 2, 5, 1, 3)]
	],
	avoidClasses(clForest, 4, clWater, 1, clPlayer, 20, clRock, 15, clHill, 1),
	clRock);

g_Map.log("Creating metal mines");
createMines(
	[
		[new SimpleObject(oMetalLarge, 1, 1, 0, 4)]
	],
	avoidClasses(clForest, 4, clWater, 1, clPlayer, 20, clMetal, 15, clRock, 5, clHill, 1),
	clMetal);

Engine.SetProgress(65);

createDecoration(
	[
		[
			new SimpleObject(aRockMedium, 1, 3, 0, 1)
		],
		[
			new SimpleObject(aBushSmall, 1, 2, 0, 1),
			new SimpleObject(aBushMedium, 1, 3, 0, 2),
			new SimpleObject(aRockLarge, 1, 2, 0, 1)
		]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(40, 360)
	],
	avoidClasses(clWater, 2, clForest, 0, clPlayer, 20, clHill, 1));

Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oHawk, 1, 1, 0, 3)],
		[new SimpleObject(oWolf1, 4, 6, 0, 4)],
		[new SimpleObject(oWolf2, 4, 8, 0, 4)],
		[new SimpleObject(oFox, 2, 3, 0, 4)],
		[new SimpleObject(oDeer, 4, 6, 0, 2)],
		[new SimpleObject(oRabbit, 1, 3, 4, 6)]
	],
	[
		scaleByMapSize(3, 10),
		scaleByMapSize(3, 10),
		scaleByMapSize(3, 10),
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20)
	],
	avoidClasses(clWater, 3, clPlayer, 20, clHill, 1, clFood, 10));

Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oFish, 1, 2, 0, 2)]
	],
	[
		3 * numPlayers
	],
	[avoidClasses(clPlayer, 8, clForest, 1, clHill, 4), stayClasses (clWater, 6)],
	clFood);

g_Map.log("Creating shallow flora");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aReeds, 6, 14, 1, 5)], false, clShallowsFlora),
	0,
	[
		new HeightConstraint(-1, 0),
		avoidClasses(clShallowsFlora, 25),
	],
	20 * scaleByMapSize(13, 200),
	80);

g_Map.log("Creating gallic decoratives");
createDecoration(
	[
		[new SimpleObject(aOutpostPalisade, 1, 1, 0, 1)],
		[new SimpleObject(aWorkshopChariot, 1, 1, 0, 1)],
	],
	[
		scaleByMapSize(2, 7),
		scaleByMapSize(2, 7)
	],
	avoidClasses(clForest, 1, clPlayer, 20, clBaseResource, 5, clHill, 4, clFood, 4, clWater, 5, clRock, 9, clMetal, 9));


placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 10, clWater, 5));

setSkySet(pickRandom(["fog", "stormy", "sunset"]));
setSunElevation(0.27);
setSunRotation(randomAngle());
setSunColor(0.746, 0.718, 0.539);
setWaterColor(0.292, 0.347, 0.691);
setWaterTint(0.550, 0.543, 0.437);
setWaterMurkiness(0.83);
setWaterType("clap");

setWindAngle(startAngle);

setFogColor(0.8, 0.76, 0.61);
setFogThickness(2);
setFogFactor(1.2);

setPPEffect("hdr");
setPPContrast(0.65);
setPPSaturation(0.42);
setPPBloom(0.6);

g_Map.ExportMap();
