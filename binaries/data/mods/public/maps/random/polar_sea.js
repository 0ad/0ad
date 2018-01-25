Engine.LoadLibrary("rmgen");

var tPrimary = ["polar_snow_a"];
var tSecondary = "polar_snow_glacial";
var tHalfSnow = ["ice_01", "ice_dirt"];
var tSnowLimited = ["polar_snow_b", "polar_ice"];
var tDirt = "ice_dirt";
var tRoad = "polar_ice_b";
var tRoadWild = "polar_ice_cracked";
var tShore = "polar_ice_snow";
var tWater = "polar_ice_c";

var oArcticFox = "gaia/fauna_fox_arctic";
var oArcticWolf = "trigger/fauna_arctic_wolf_attack";
var oMuskox = "gaia/fauna_muskox";
var oWalrus = "gaia/fauna_walrus";
var oWhaleFin = "gaia/fauna_whale_fin";
var oWhaleHumpback = "gaia/fauna_whale_humpback";
var oFish = "gaia/fauna_fish";
var oStoneLarge = "gaia/geology_stonemine_medit_quarry";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_desert_badlands_slabs";
var oWoodTreasure = "gaia/special_treasure_wood";
var oMarket = "skirmish/structures/default_market";

var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aIceberg = "actor|props/special/eyecandy/iceberg.xml";

var heightSeaGround = -4;
var heightLand = 2;
var heightCliff = 3;

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clArcticWolf = g_Map.createTileClass();

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

var treasures = [{
	"template": oWoodTreasure,
	"count": isNomad() ? 16 : 14
}];

log("Creating player markets...");
if (!isNomad())
	for (let i = 0; i < numPlayers; ++i)
	{
		let marketPos = Vector2D.add(playerPosition[i], new Vector2D(12, 0).rotate(randomAngle())).round();
		placeObject(marketPos.x, marketPos.y, oMarket, playerIDs[i], BUILDING_ORIENTATION);
		addCivicCenterAreaToClass(marketPos, clBaseResource);
	}

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": "towers",
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
		"template": oMuskox
	},
	// No berries, no trees, no decoratives
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Treasures": {
		"types": treasures
	},
});
Engine.SetProgress(30);

log("Creating central lake...");
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(5, 16)),
		Math.floor(scaleByMapSize(35, 200)),
		1,
		mapCenter,
		0,
		[Math.floor(fractionToTiles(0.17))]),
	[
		new LayeredPainter([tShore, tWater, tWater, tWater], [1, 4, 2]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
		new TileClassPainter(clWater)
	],
	avoidClasses(clPlayer, 20));

Engine.SetProgress(40);

log("Creating small lakes...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(2, 4)), Math.floor(scaleByMapSize(20, 140)), 0.7),
	[
		new LayeredPainter([tShore, tWater, tWater], [1, 3]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 5),
		new TileClassPainter(clWater)
	],
	avoidClasses(clPlayer, 20),
	scaleByMapSize(10, 16),
	1);
Engine.SetProgress(50);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));
Engine.SetProgress(60);

log("Creating hills...");
createHills(
	[tPrimary, tPrimary, tSecondary],
	avoidClasses(clPlayer, 20, clHill, 35),
	clHill,
	scaleByMapSize(20, 240));
Engine.SetProgress(65);

log("Creating dirt patches...");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tDirt,tHalfSnow], [tHalfSnow,tSnowLimited]],
	[2],
	avoidClasses(clWater, 3, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);

log("Creating glacier patches...");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tSecondary,
	avoidClasses(clWater, 3, clDirt, 5, clPlayer, 12),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(70);

log("Creating stone mines...");
	createMines(
	[
		[new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
		[new SimpleObject(oStoneSmall, 2,5, 1,3)]
	],
	avoidClasses(clWater, 3, clPlayer, 20, clRock, 18, clHill, 2),
	clRock);

log("Creating metal mines...");
createMines(
	[
		[new SimpleObject(oMetalLarge, 1,1, 0,4)]
	],
	avoidClasses(clWater, 3, clPlayer, 20, clMetal, 18, clRock, 5, clHill, 2),
	clMetal);
Engine.SetProgress(75);

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
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
	],
	avoidClasses(clWater, 0, clPlayer, 0));

createDecoration(
	[
		[new SimpleObject(aIceberg, 1, 1, 1, 1)]
	],
	[
		scaleByMapSize(8, 131)
	],
	[stayClasses(clWater, 4), avoidClasses(clHill, 2)]);
Engine.SetProgress(80);

createFood(
	[
		[new SimpleObject(oArcticFox, 1, 2, 0, 3)],
		[new SimpleObject(isNomad() ? oArcticFox : oArcticWolf, 4, 6, 0, 4)],
		[new SimpleObject(oWalrus, 2, 3, 0, 2)],
		[new SimpleObject(oMuskox, 2, 3, 0, 2)]
	],
	[
		3 * numPlayers,
		5 * numPlayers,
		5 * numPlayers,
		12 * numPlayers
	],
	avoidClasses(clPlayer, 35, clFood, 16, clWater, 2, clMetal, 4, clRock, 4, clHill, 2),
	clFood);

createFood(
	[
		[new SimpleObject(oWhaleFin, 1, 2, 0, 2)],
		[new SimpleObject(oWhaleHumpback, 1, 2, 0, 2)]
	],
	[
		scaleByMapSize(1, 6) * 3,
		scaleByMapSize(1, 6) * 3,
	],
	[avoidClasses(clFood, 20, clHill, 5), stayClasses(clWater, 6)],
	clFood);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		100
	],
	[avoidClasses(clFood, 12, clHill, 5), stayClasses(clWater, 6)],
	clFood);
Engine.SetProgress(85);

// Create trigger points where wolves spawn
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject("trigger/trigger_point_A", 1, 1, 0, 0)], true, clArcticWolf),
	0,
	avoidClasses(clWater, 2, clMetal, 4, clRock, 4, clPlayer, 15, clHill, 2, clArcticWolf, 20),
	1000,
	100);
Engine.SetProgress(95);

if (randBool(1/3))
{
	setSkySet("sunset 1");
	setSunColor(0.8, 0.7, 0.6);
	setTerrainAmbientColor(0.7, 0.6, 0.7);
	setUnitsAmbientColor(0.6, 0.5, 0.6);
	setSunElevation(Math.PI * randFloat(1/24, 1/7));
}
else
{
	setSkySet(pickRandom(["cumulus", "rain", "mountainous", "overcast", "rain", "stratus"]));
	setSunElevation(Math.PI * randFloat(1/9, 1/7));
}

if (isNomad())
{
	let constraint = avoidClasses(clWater, 4, clMetal, 4, clRock, 4, clHill, 4, clFood, 2);
	[playerIDs, playerPosition] = placePlayersNomad(clPlayer, constraint);

	for (let i = 0; i < numPlayers; ++i)
		placePlayerBaseTreasures({
			"playerID": playerIDs[i],
			"playerPosition": playerPosition[i],
			"BaseResourceClass": clBaseResource,
			"baseResourceConstraint": constraint,
			"types": treasures
		});
}

setSunRotation(randomAngle());

setWaterColor(0.3, 0.3, 0.4);
setWaterTint(0.75, 0.75, 0.75);
setWaterMurkiness(0.92);
setWaterWaviness(0.5);
setWaterType("clap");

setFogThickness(0.76);
setFogFactor(0.7);

setPPEffect("hdr");
setPPContrast(0.6);
setPPSaturation(0.45);
setPPBloom(0.4);

g_Map.ExportMap();
