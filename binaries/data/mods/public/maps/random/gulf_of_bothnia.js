Engine.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;

var biome = pickRandom(["late_spring", "winter", "frozen_lake"]);

if (biome == "late_spring")
{
	log("Late spring biome...");
	var heightSeaGround = -3;
	var heightShore = 1;
	var heightLand = 3;

	var fishCount = { "min": 20, "max": 80 };
	var bushCount = { "min": 13, "max": 200 };

	setFogThickness(0.26);
	setFogFactor(0.4);

	setPPEffect("hdr");
	setPPSaturation(0.48);
	setPPContrast(0.53);
	setPPBloom(0.12);

	var tPrimary = ["alpine_dirt_grass_50"];
	var tForestFloor = "alpine_forrestfloor";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
	var tSecondary = "alpine_grass_rocky";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_rocky"];
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_grass_50";
	var tWater = "alpine_shore_rocks";

	var oPine = "gaia/flora_tree_pine";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

	var aGrass = "actor|props/flora/grass_soft_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm.xml";
}
else if (biome == "winter")
{
	log("Winter biome...");
	var heightSeaGround = -3;
	var heightShore = 1;
	var heightLand = 3;

	var fishCount = { "min": 20, "max": 80 };
	var bushCount = { "min": 13, "max": 200 };

	setFogFactor(0.35);
	setFogThickness(0.19);
	setPPSaturation(0.37);
	setPPEffect("hdr");

	var tPrimary = ["alpine_snow_a", "alpine_snow_b"];
	var tForestFloor = "alpine_forrestfloor_snow";
	var tCliff = ["alpine_cliff_snow"];
	var tSecondary = "alpine_grass_snow_50";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_a", "alpine_snow_b"];
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_icy";
	var tWater = "alpine_shore_rocks";

	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}
else if (biome == "frozen_lake")
{
	log("Frozen lake biome...");
	var heightSeaGround = 0;
	var heightShore = 2;
	var heightLand = 3;

	var fishCount = { "min": 0, "max": 0 };
	var bushCount = { "min": 0, "max": 0 };

	setFogFactor(0.41);
	setFogThickness(0.23);
	setPPSaturation(0.34);
	setPPEffect("hdr");

	var tPrimary = ["alpine_snow_a", "alpine_snow_b"];
	var tForestFloor = "alpine_snow_a";
	var tCliff = ["alpine_cliff_snow"];
	var tSecondary = "polar_ice_snow";
	var tHalfSnow = ["polar_ice_cracked"];
	var tSnowLimited = ["alpine_snow_a", "alpine_snow_b"];
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "polar_ice_snow";
	var tWater = ["polar_ice_snow", "polar_ice"];

	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oFish = "gaia/fauna_fish";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];

InitMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var startAngle = randomAngle();

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), ...playerPlacementCustomAngle(
			fractionToTiles(0.35),
			mapCenter,
			i => startAngle + 1/3 * Math.PI * (1 + 2 * (numPlayers == 1 ? 1 : 2 * i / (numPlayers - 1))))],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
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
		"template": oPine,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

log("Creating the gulf...");
var gulfLakePositions = [
	{ "numCircles": 200, "x": fractionToTiles(0), "radius": fractionToTiles(0.175) },
	{ "numCircles": 120, "x": fractionToTiles(0.3), "radius": fractionToTiles(0.2) },
	{ "numCircles": 100, "x": fractionToTiles(0.5), "radius": fractionToTiles(0.225) }
];

for (let gulfLake of gulfLakePositions)
{
	let position = Vector2D.add(mapCenter, new Vector2D(gulfLake.x, 0).rotate(-startAngle)).round();

	createArea(
		new ChainPlacer(
			2,
			Math.floor(scaleByMapSize(5, 16)),
			Math.floor(scaleByMapSize(35, gulfLake.numCircles)),
			1,
			position,
			0,
			[Math.floor(gulfLake.radius)]),
		[
			new LayeredPainter([tPrimary, tPrimary, tPrimary, tPrimary], [1, 4, 2]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			paintClass(clWater)
		],
		avoidClasses(clPlayer,scaleByMapSize(20, 28)));
}

paintTerrainBasedOnHeight(heightShore, heightLand, Elevation_ExcludeMin_ExcludeMax, tShore);
paintTerrainBasedOnHeight(-Infinity, heightShore, Elevation_ExcludeMin_IncludeMax, tWater);

createBumps(avoidClasses(clWater, 2, clPlayer, 10));

if (randBool())
	createHills([tPrimary, tCliff, tPrimary], avoidClasses(clPlayer, 20, clHill, 15, clWater, 0), clHill, scaleByMapSize(1, 4) * numPlayers);
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 0), clHill, scaleByMapSize(1, 4) * numPlayers);

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
 [tPrimary, tForestFloor, tForestFloor, pForest, pForest],
 avoidClasses(clPlayer, 20, clForest, 16, clHill, 0, clWater, 2),
 clForest,
 forestTrees);

Engine.SetProgress(60);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tPrimary,tSecondary],[tSecondary,tHalfSnow], [tHalfSnow,tSnowLimited]],
 [1,1],
 avoidClasses(clWater, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tHalfSnow,
 avoidClasses(clWater, 6, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(65);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
);
Engine.SetProgress(70);

createDecoration(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
		[new SimpleObject(aGrass, 2, 4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
		[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		scaleByMapSize(bushCount.min, bushCount.max),
		scaleByMapSize(bushCount.min, bushCount.max),
		scaleByMapSize(bushCount.min, bushCount.max)
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 5, clHill, 0, clBaseResource, 5));
Engine.SetProgress(75);

createFood(
	[
		[new SimpleObject(oDeer, 5, 7, 0, 4)],
		[new SimpleObject(oRabbit, 2, 3, 0, 2)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	clFood);

createFood(
	[[new SimpleObject(oBerryBush, 5, 7, 0, 4)]],
	[randIntInclusive(1, 4) * numPlayers + 2],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);

createFood(
	[[new SimpleObject(oFish, 2, 3, 0, 2)]],
	[scaleByMapSize(fishCount.min, fishCount.max)],
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oPine],
	avoidClasses(clWater, 3, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

// Avoid the lake, even if frozen
placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("stormy");
setSunRotation(randomAngle());
setSunElevation(Math.PI * randFloat(1/6, 1/4));

setWaterColor(0.035,0.098,0.314);
setWaterTint(0.28, 0.3, 0.59);
setWaterWaviness(5.0);
setWaterType("lake");
setWaterMurkiness(0.88);

ExportMap();
