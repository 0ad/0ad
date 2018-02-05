Engine.LoadLibrary("rmgen");

const tGrass = ["temp_grass_aut", "temp_grass_aut", "temp_grass_d_aut"];
const tForestFloor = "temp_grass_aut";
const tGrassA = "temp_grass_plants_aut";
const tGrassB = "temp_grass_b_aut";
const tGrassC = "temp_grass_c_aut";
const tHill = ["temp_highlands_aut", "temp_grass_long_b_aut"];
const tCliff = ["temp_cliff_a", "temp_cliff_b"];
const tRoad = "temp_road_aut";
const tGrassPatch = "temp_grass_plants_aut";
const tShore = "temp_plants_bog_aut";
const tWater = "temp_mud_a";

const oBeech = "gaia/flora_tree_euro_beech_aut";
const oOak = "gaia/flora_tree_oak_aut";
const oPine = "gaia/flora_tree_pine";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_rabbit";
const oBerryBush = "gaia/flora_bush_berry";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";
const oFoodTreasure = "gaia/treasure/food_bin";
const oWoodTreasure = "gaia/treasure/wood";
const oStoneTreasure = "gaia/treasure/stone";
const oMetalTreasure = "gaia/treasure/metal";

const aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aReeds = "actor|props/flora/reeds_pond_dry.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oBeech, tForestFloor];
const pForestO = [tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor];
const pForestP = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];

var heightSeaGround = -4;
var heightLand = 3;

var g_Map = new RandomMap(heightLand, tGrass);

const numPlayers = getNumPlayers();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

var treasures = [
	{ "template": oFoodTreasure, "distance": 5 },
	{ "template": oWoodTreasure, "distance": 5 },
	{ "template": oMetalTreasure, "distance": 3 },
	{ "template": oStoneTreasure, "distance": 2 }
];

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

g_Map.log("Creating playerbases");
var playerAngle = BUILDING_ORIENTATION;
for (let i = 0; i < numPlayers; ++i)
{
	if (isNomad())
		break;

	// CC and units
	for (let dist of [6, 8])
	{
		let ents = getStartingEntities(playerIDs[i]);

		if (dist == 8)
			ents = ents.filter(ent => ent.Template.indexOf("female") != -1 || ent.Template.indexOf("infantry") != -1);

		placeStartingEntities(playerPosition[i], playerIDs[i], ents, dist);
	}

	// Treasure
	for (let j = 0; j < treasures.length; ++j)
		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(treasures[j].template, treasures[j].distance, treasures[j].distance, 0, 2)],
				false,
				clBaseResource,
				Vector2D.add(playerPosition[i], new Vector2D(10, 0).rotate(-j * Math.PI / 2 - playerAngle))),
			0);

	// Ground texture
	var civ = getCivCode(playerIDs[i]);
	var tilesSize = civ == "cart" ? 23 : 21;
	createArea(
		new ConvexPolygonPlacer(
			new Array(4).fill(0).map((zero, j) => new Vector2D(tilesSize, 0).rotate(j * Math.PI / 2 - playerAngle - Math.PI/4).add(playerPosition[i])),
			Infinity),
		[
			new TerrainPainter(tRoad),
			new TileClassPainter(clPlayer)
		]);

	// Fortress
	if (civ == "brit" || civ == "gaul" || civ == "iber")
	{
		var wall = ["gate", "tower", "long",
			"cornerIn", "long", "barracks", "tower", "long", "tower", "house", "long",
			"cornerIn", "long", "house", "tower", "gate", "tower", "house", "long",
			"cornerIn", "long", "house", "tower", "long", "tower", "house", "long",
			"cornerIn", "long", "house", "tower"];
	}
	else
	{
		var wall = ["gate", "tower", "long",
			"cornerIn", "long", "barracks", "tower", "long", "tower", "long",
			"cornerIn", "long", "house", "tower", "gate", "tower", "long",
			"cornerIn", "long", "house", "tower", "long", "tower", "long",
			"cornerIn", "long", "house", "tower"];
	}
	placeCustomFortress(playerPosition[i], new Fortress("Spahbod", wall), civ, playerIDs[i], playerAngle);
}

g_Map.log("Creating lakes");
var numLakes = Math.round(scaleByMapSize(1,4) * numPlayers);
var waterAreas = createAreas(
	new ClumpPlacer(scaleByMapSize(100,250), 0.8, 0.1, 10),
	[
		new LayeredPainter([tShore, tWater, tWater], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 3),
		new TileClassPainter(clWater)
	],
	avoidClasses(clPlayer, 7, clWater, 20),
	numLakes);

Engine.SetProgress(15);

g_Map.log("Creating reeds");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup([new SimpleObject(aReeds, 5,10, 0,4), new SimpleObject(aLillies, 0,1, 0,4)], true),
	0,
	[borderClasses(clWater, 3, 0), stayClasses(clWater, 1)],
	numLakes, 100,
	waterAreas);

Engine.SetProgress(25);

g_Map.log("Creating fish");
createObjectGroupsByAreasDeprecated(
	new SimpleGroup(
		[new SimpleObject(oFish, 1,1, 0,1)],
		true, clFood
	),
	0,
	[stayClasses(clWater, 4),  avoidClasses(clFood, 8)],
	numLakes / 4,
	50,
	waterAreas);
Engine.SetProgress(30);

createBumps(avoidClasses(clWater, 2, clPlayer, 5));
Engine.SetProgress(35);

createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 5, clWater, 5, clHill, 15), clHill, scaleByMapSize(1, 4) * numPlayers);
Engine.SetProgress(40);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.7);
var types = [
	[[tForestFloor, tGrass, pForestD], [tForestFloor, pForestD]],
	[[tForestFloor, tGrass, pForestO], [tForestFloor, pForestO]],
	[[tForestFloor, tGrass, pForestP], [tForestFloor, pForestP]]
];
var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / num, 0.5),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(clPlayer, 5, clWater, 3, clForest, 15, clHill, 1),
		num);
Engine.SetProgress(50);

g_Map.log("Creating dirt patches");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]],
 [1,1],
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 1),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(55);

g_Map.log("Creating grass patches");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tGrassPatch,
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 1),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(60);

g_Map.log("Creating stone mines");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clRock, 10, clHill, 1),
 clRock);
Engine.SetProgress(65);

g_Map.log("Creating metal mines");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clMetal, 10, clRock, 5, clHill, 1),
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
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 1, clHill, 0));
Engine.SetProgress(80);

createFood(
	[
		[new SimpleObject(oSheep, 2, 3, 0, 2)],
		[new SimpleObject(oDeer, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 6, clHill, 1, clFood, 20),
	clFood);
Engine.SetProgress(85);

createFood(
	[
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	avoidClasses(clWater, 2, clForest, 0, clPlayer, 6, clHill, 1, clFood, 10),
	clFood);

Engine.SetProgress(90);

createStragglerTrees(
	[oOak, oBeech, oPine],
	avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);
Engine.SetProgress(95);

placePlayersNomad(clPlayer, avoidClasses(clWater, 2, clHill, 2, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("sunny");
setWaterColor(0.157, 0.149, 0.443);
setWaterTint(0.443,0.42,0.824);
setWaterWaviness(2.0);
setWaterType("lake");
setWaterMurkiness(0.83);

setFogFactor(0.35);
setFogThickness(0.22);
setFogColor(0.82,0.82, 0.73);
setPPSaturation(0.56);
setPPContrast(0.56);
setPPBloom(0.38);
setPPEffect("hdr");

g_Map.ExportMap();
