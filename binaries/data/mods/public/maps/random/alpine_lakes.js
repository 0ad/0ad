Engine.LoadLibrary("rmgen");

if (randBool())
{
	log("Late spring biome...");

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
	var tDirt = "alpine_dirt";
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
else
{
	log("Winter biome...");

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
	var tDirt = "alpine_dirt";
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

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

initTerrain(tPrimary);

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(0.35),
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
		"count": scaleByMapSize(3, 12)
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

log("Creating hills...");
createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 8), clHill, scaleByMapSize(10, 40) * numPlayers, Math.floor(scaleByMapSize(40, 60)), Math.floor(scaleByMapSize(4, 5)), Math.floor(scaleByMapSize(7, 15)), Math.floor(scaleByMapSize(5, 15)));

Engine.SetProgress(30);

var lakeAreas = [];
var playerConstraint = new AvoidTileClassConstraint(clPlayer, 20);
var waterConstraint = new AvoidTileClassConstraint(clWater, 8);

for (var x = 0; x < mapSize; ++x)
	for (var z = 0; z < mapSize; ++z)
		if (playerConstraint.allows(x, z) && waterConstraint.allows(x, z))
			lakeAreas.push([x, z]);

var lakeAreaLen;

log("Creating lakes...");

var numLakes = scaleByMapSize(5, 16);
for (var i = 0; i < numLakes; ++i)
{
	lakeAreaLen = lakeAreas.length;
	if (!lakeAreaLen)
		break;

	let chosenPoint = pickRandom(lakeAreas);

	let newLake = createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(4, 8)), Math.floor(scaleByMapSize(40, 180)), 0.7, chosenPoint[0], chosenPoint[1]),
		[
			new LayeredPainter([tShore, tWater, tWater], [1, 3]),
			new SmoothElevationPainter(ELEVATION_SET, -5, 5),
			paintClass(clWater)
		],
		avoidClasses(clPlayer, 20, clWater, 8),
		1, 1
	);

	if (newLake && newLake.length)
	{
		var n = 0;
		for (var j = 0; j < lakeAreaLen; ++j)
		{
			var x = lakeAreas[j][0], z = lakeAreas[j][1];
			if (playerConstraint.allows(x, z) && waterConstraint.allows(x, z))
				lakeAreas[n++] = lakeAreas[j];
		}
		lakeAreas.length = n;
	}

}
paintTerrainBasedOnHeight(3, Math.floor(scaleByMapSize(20, 40)), 0, tCliff);
paintTerrainBasedOnHeight(Math.floor(scaleByMapSize(20, 40)), 100, 3, tSnowLimited);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
 [tPrimary, tForestFloor, tForestFloor, pForest, pForest],
 avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 2),
 clForest,
 forestTrees);

Engine.SetProgress(60);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tDirt,tHalfSnow], [tHalfSnow,tSnowLimited]],
 [2],
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tSecondary,
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
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
		scaleByMapSize(13, 200),
		scaleByMapSize(13, 200),
		scaleByMapSize(13, 200)
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0));

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
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
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

createStragglerTrees(
	[oPine],
	avoidClasses(clWater, 5, clForest, 3, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randFloat(0, 2 * Math.PI));
setSunElevation(Math.PI * randFloat(1/5, 1/3));
setWaterColor(0.0, 0.047, 0.286);				// dark majestic blue
setWaterTint(0.471, 0.776, 0.863);				// light blue
setWaterMurkiness(0.82);
setWaterWaviness(3.0);
setWaterType("clap");

ExportMap();
