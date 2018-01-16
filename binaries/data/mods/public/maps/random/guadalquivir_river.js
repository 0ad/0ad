Engine.LoadLibrary("rmgen");

const tGrass = ["medit_grass_field_a", "medit_grass_field_b"];
const tForestFloorC = "medit_plants_dirt";
const tForestFloorP = "medit_grass_shrubs";
const tGrassA = "medit_grass_field_b";
const tGrassB = "medit_grass_field_brown";
const tGrassC = "medit_grass_field_dry";
const tRoad = "medit_city_tile";
const tRoadWild = "medit_city_tile";
const tGrassPatch = "medit_grass_shrubs";
const tShore = "sand_grass_25";
const tWater = "medit_sand_wet";

const oPoplar = "gaia/flora_tree_poplar";
const oApple = "gaia/flora_tree_apple";
const oCarob = "gaia/flora_tree_carob";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForestP = [tForestFloorP + TERRAIN_SEPARATOR + oPoplar, tForestFloorP];
const pForestC = [tForestFloorC + TERRAIN_SEPARATOR + oCarob, tForestFloorC];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();
const mapBounds = getMapBounds();

var clPlayer = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clLand = createTileClass();
var clRiver = createTileClass();
var clShallow = createTileClass();

var landHeight = 3;
var shoreHeight = 2;
var shallowHeight = -1.5;
var waterHeight = -3;

log("Create the continent body");
var continentCenter = new Vector2D(fractionToTiles(0.5), fractionToTiles(0.7)).round();
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(5, 12)),
		Math.floor(scaleByMapSize(60, 700)),
		1,
		continentCenter.x,
		continentCenter.y,
		0,
		[Math.floor(fractionToTiles(0.49))]),
	[
		new LayeredPainter([tGrass, tGrass, tGrass], [4, 2]),
		new SmoothElevationPainter(ELEVATION_SET, landHeight, 4),
		paintClass(clLand)
	],
	null);

placePlayerBases({
	"PlayerPlacement": [primeSortAllPlayers(), ...playerPlacementCustomAngle(
			fractionToTiles(0.35),
			continentCenter,
			i => Math.PI * (-0.46 / numPlayers * (i + i % 2) - (i % 2) / 2))],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": false,
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
		"template": oPoplar,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

paintRiver({
	"parallel": true,
	"constraint": stayClasses(clLand, 0),
	"start": new Vector2D(mapCenter.x, mapBounds.top),
	"end": new Vector2D(mapCenter.x, mapBounds.bottom),
	"width": fractionToTiles(0.07),
	"fadeDist": scaleByMapSize(3, 12),
	"deviation": 1,
	"waterHeight": waterHeight,
	"landHeight": shoreHeight,
	"meanderShort": 12,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height, z) => {
		addToClass(ix, iz, clRiver);
		placeTerrain(ix, iz, tWater);

		if (height < shallowHeight && (
		    z > 0.3 && z < 0.4 ||
		    z > 0.5 && z < 0.6 ||
		    z > 0.7 && z < 0.8))
		{
			setHeight(ix, iz, shallowHeight);
			addToClass(ix, iz, clShallow);
		}
	}
});

paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

createBumps([avoidClasses(clPlayer, 20, clRiver, 1), stayClasses(clLand, 3)]);

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
 [tGrass, tForestFloorP, tForestFloorC, pForestC, pForestP],
 [avoidClasses(clPlayer, 20, clForest, 17, clRiver, 1), stayClasses(clLand, 7)],
 clForest,
 forestTrees);

Engine.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]],
 [1,1],
 [avoidClasses(clForest, 0, clDirt, 3, clPlayer, 8, clRiver, 1), stayClasses(clLand, 7)],
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tGrassPatch,
 [avoidClasses(clForest, 0, clDirt, 3, clPlayer, 8, clRiver, 1), stayClasses(clLand, 7)],
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clRiver, 1), stayClasses(clLand, 5)],
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clRiver, 1), stayClasses(clLand, 5)],
 clMetal
);
Engine.SetProgress(65);

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
	[avoidClasses(clPlayer, 1, clDirt, 1, clRiver, 1), stayClasses(clLand, 6)]);

log("Create water decoration in the shallow parts");
createDecoration(
	[
		[new SimpleObject(aReeds, 1, 3, 0, 1)],
		[new SimpleObject(aLillies, 1, 2, 0, 1)]
	],
	[
		scaleByMapSize(800, 12800),
		scaleByMapSize(800, 12800)
	],
	stayClasses(clShallow, 0));

Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oDeer, 5, 7, 0, 4)],
		[new SimpleObject(oSheep, 2, 3, 0, 2)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	[avoidClasses(clForest, 0, clPlayer, 20, clFood, 20, clRiver, 1), stayClasses(clLand, 3)],
	clFood);

createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	[avoidClasses(clForest, 0, clPlayer, 20, clFood, 10, clRiver, 1), stayClasses(clLand, 3)],
	clFood);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		25 * numPlayers
	],
	avoidClasses(clLand, 2, clRiver, 1),
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oPoplar, oCarob, oApple],
	[avoidClasses(clForest, 1, clPlayer, 9, clMetal, 6, clRock, 6, clRiver, 1), stayClasses(clLand, 7)],
	clForest,
	stragglerTrees);

placePlayersNomad(
	clPlayer,
	new AndConstraint([
		stayClasses(clLand, 4),
		avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clFood, 2)]));

setSkySet("cumulus");
setWaterColor(0.2,0.312,0.522);
setWaterTint(0.1,0.1,0.8);
setWaterWaviness(4.0);
setWaterType("lake");
setWaterMurkiness(0.73);

setFogFactor(0.3);
setFogThickness(0.25);

setPPEffect("hdr");
setPPContrast(0.62);
setPPSaturation(0.51);
setPPBloom(0.12);

ExportMap();
