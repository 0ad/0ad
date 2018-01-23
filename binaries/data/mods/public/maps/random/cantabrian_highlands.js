Engine.LoadLibrary("rmgen");

const tPrimary = "temp_grass_long";
const tGrass = ["temp_grass", "temp_grass", "temp_grass_d"];
const tGrassPForest = "temp_plants_bog";
const tGrassDForest = "temp_plants_bog";
const tGrassA = "temp_grass_plants";
const tGrassB = "temp_plants_bog";
const tGrassC = "temp_mud_a";
const tHill = ["temp_highlands", "temp_grass_long_b"];
const tCliff = ["temp_cliff_a", "temp_cliff_b"];
const tRoad = "temp_road";
const tRoadWild = "temp_road_overgrown";
const tGrassPatchBlend = "temp_grass_long_b";
const tGrassPatch = ["temp_grass_d", "temp_grass_clovers"];
const tShoreBlend = "temp_grass_plants";
const tShore = "temp_plants_bog";
const tWater = "temp_mud_a";

const oOak = "gaia/flora_tree_oak";
const oOakLarge = "gaia/flora_tree_oak_large";
const oApple = "gaia/flora_tree_apple";
const oPine = "gaia/flora_tree_pine";
const oAleppoPine = "gaia/flora_tree_aleppo_pine";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";

const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/pond_lillies_large.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oOak, tGrassDForest + TERRAIN_SEPARATOR + oOakLarge, tGrassDForest];
const pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oPine, tGrassPForest + TERRAIN_SEPARATOR + oAleppoPine, tGrassPForest];

const heightSeaGround = -7;
const heightLand = 3;
const heightHill = 20;

InitMap(heightLand, tPrimary);

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var playerHillRadius = defaultPlayerBaseRadius() / (isNomad() ? 1.5 : 1);

var [playerIDs, playerPosition, playerAngle] = playerPlacementCircle(fractionToTiles(0.35));

log("Creating player hills and ramps...");
for (let i = 0; i < numPlayers; ++i)
{
	createArea(
		new ClumpPlacer(diskArea(playerHillRadius), 0.95, 0.6, 10, playerPosition[i]),
		[
			new LayeredPainter([tCliff, tHill], [2]),
			new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
			paintClass(clPlayer)
		]);

	let angle = playerAngle[i] + Math.PI * (1 + randFloat(-1, 1) / 8);
	createPassage({
		"start": Vector2D.add(playerPosition[i], new Vector2D(playerHillRadius + 15, 0).rotate(-angle)),
		"end": Vector2D.add(playerPosition[i], new Vector2D(playerHillRadius - 3, 0).rotate(-angle)),
		"startWidth": 10,
		"endWidth": 10,
		"smoothWidth": 2,
		"tileClass": clPlayer,
		"terrain": tHill,
		"edgeTerrain": tCliff
	});
}

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
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
		"template": oOak,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(10);

log("Creating lakes...");
var numLakes = Math.round(scaleByMapSize(1,4) * numPlayers);
var waterAreas = createAreas(
	new ClumpPlacer(scaleByMapSize(100, 250), 0.8, 0.1, 10),
	[
		new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 6),
		paintClass(clWater)
	],
	avoidClasses(clPlayer, 2, clWater, 20),
	numLakes
);
Engine.SetProgress(15);

log("Creating reeds...");
var group = new SimpleGroup(
	[new SimpleObject(aReeds, 5,10, 0,4), new SimpleObject(aLillies, 0,1, 0,4)], true
);
createObjectGroupsByAreasDeprecated(group, 0,
	[borderClasses(clWater, 3, 0), stayClasses(clWater, 1)],
	numLakes, 100,
	waterAreas
);
Engine.SetProgress(20);

log("Creating fish...");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup(
		[new SimpleObject(oFish, 1,1, 0,1)],
		true, clFood
	),
	0,
	[stayClasses(clWater, 4),  avoidClasses(clFood, 8)],
	numLakes / 4,
	50,
	waterAreas
);
Engine.SetProgress(25);

createBumps(avoidClasses(clWater, 2, clPlayer, 0));
Engine.SetProgress(30);

log("Creating hills...");
createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 2, clWater, 5, clHill, 15), clHill, scaleByMapSize(1, 4) * numPlayers);
Engine.SetProgress(35);

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
 [tGrass, tGrassDForest, tGrassPForest, pForestP, pForestD],
 avoidClasses(clPlayer, 1, clWater, 3, clForest, 17, clHill, 1),
 clForest,
 forestTrees);
Engine.SetProgress(40);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]],
 [1,1],
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(45);

log("Creating grass patches...");
createLayeredPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 [tGrassPatchBlend, tGrassPatch],
 [1],
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(50);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clRock, 10, clHill, 1),
 clRock);
Engine.SetProgress(55);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
);
Engine.SetProgress(60);

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
Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oSheep, 2, 3, 0, 2)],
		[new SimpleObject(oDeer, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 1, clHill, 1, clFood, 20),
	clFood);
Engine.SetProgress(80);

createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);
Engine.SetProgress(85);

createStragglerTrees(
	[oOak, oOakLarge, oPine, oApple],
	avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);
Engine.SetProgress(90);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("cirrus");
setWaterColor(0.447, 0.412, 0.322);			// muddy brown
setWaterTint(0.447, 0.412, 0.322);
setWaterMurkiness(1.0);
setWaterWaviness(3.0);
setWaterType("lake");

setFogThickness(0.25);
setFogFactor(0.4);

setPPEffect("hdr");
setPPSaturation(0.62);
setPPContrast(0.62);
setPPBloom(0.3);

ExportMap();
