RMS.LoadLibrary("rmgen");

// terrain textures
const tGrass = ["temp_grass_aut", "temp_grass_aut", "temp_grass_d_aut"];
const tForestFloor = "temp_grass_aut";
const tGrassA = "temp_grass_plants_aut";
const tGrassB = "temp_grass_b_aut";
const tGrassC = "temp_grass_c_aut";
const tDirt = ["temp_plants_bog_aut", "temp_mud_a"];
const tHill = ["temp_highlands_aut", "temp_grass_long_b_aut"];
const tCliff = ["temp_cliff_a", "temp_cliff_b"];
const tRoad = "temp_road_aut";
const tRoadWild = "temp_road_overgrown_aut";
const tGrassPatch = "temp_grass_plants_aut";
const tShoreBlend = "temp_grass_plants_aut";
const tShore = "temp_plants_bog_aut";
const tWater = "temp_mud_a";

// gaia entities
const oBeech = "gaia/flora_tree_euro_beech_aut";
const oOak = "gaia/flora_tree_oak_aut";
const oPine = "gaia/flora_tree_pine";
const oChicken = "gaia/fauna_chicken";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_rabbit";
const oBerryBush = "gaia/flora_bush_berry";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";
const oWood = "gaia/special_treasure_wood";
const oFood = "gaia/special_treasure_food_bin";
const oMetal = "gaia/special_treasure_metal";
const oStone = "gaia/special_treasure_stone";

// decorative props
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

const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();


// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = sortPlayers(playerIDs);

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);
var baseRadius = 30;

var startAngle = randFloat(0, TWO_PI);
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*TWO_PI/numPlayers;
	playerX[i] = mapSize*(0.5 + 0.35*cos(playerAngle[i]));
	playerZ[i] = mapSize*(0.5 + 0.35*sin(playerAngle[i]));
}

for (var i=0; i < numPlayers; i++)
{
	var startEntities = getStartingEntities(i);
	// Place starting entities
	createStartingPlayerEntities(playerX[i], playerZ[i], i+1, startEntities, BUILDING_ANGlE)
	var uDist = 8;
	var uSpace = 2;
	for (var j = 1; j < startEntities.length - 1; ++j)
	{
		var uAngle = BUILDING_ANGlE - PI * (2-j) / 2;
		var count = (startEntities[j].Count !== undefined ? startEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = playerX[i] + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = playerZ[i] + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, startEntities[j].Template, i+1, uAngle);
		}
	}
	// create resources
	var bbAngle = BUILDING_ANGlE;
	var bbDist = 10;
	var bbX = round(playerX[i] + bbDist * cos(bbAngle));
	var bbZ = round(playerZ[i] + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oFood, 5,5, 0,2)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	bbAngle += PI/2;
	var bbX = round(playerX[i] + bbDist * cos(bbAngle));
	var bbZ = round(playerZ[i] + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oWood, 5,5, 0,2)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	bbAngle += PI/2;
	var bbX = round(playerX[i] + bbDist * cos(bbAngle));
	var bbZ = round(playerZ[i] + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetal, 3,3, 0,2)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	bbAngle += PI/2;
	var bbX = round(playerX[i] + bbDist * cos(bbAngle));
	var bbZ = round(playerZ[i] + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStone, 2,2, 0,2)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// Base texture
	var civ = g_MapSettings.PlayerData[i].Civ;
	var tilesSize = (civ == "cart" ? 27 : 22);
	
	const minBoundX = (playerX[i] > tilesSize ? playerX[i] - tilesSize : 0);
	const minBoundY = (playerZ[i] > tilesSize ? playerZ[i] - tilesSize : 0);
	const maxBoundX = (playerX[i] < mapSize - tilesSize ? playerX[i] + tilesSize : mapSize);
	const maxBoundY = (playerZ[i] < mapSize - tilesSize ? playerZ[i] + tilesSize : mapSize);
	
	for (var tx = minBoundX; tx < maxBoundX; ++tx)
	{
		for (var ty = minBoundY; ty < maxBoundY; ++ty)
		{
			var unboundSumOfXY = tx + ty - minBoundX - minBoundY;
			if ((unboundSumOfXY > tilesSize) && (unboundSumOfXY < 3 * tilesSize) && (tx - ty + minBoundY - minBoundX < tilesSize) && (ty - tx - minBoundY + minBoundX < tilesSize))
			{
				placeTerrain(floor(tx), floor(ty), tRoad);
				addToClass(floor(tx), floor(ty), clPlayer);
			}
		}
	}
	
	// Place custom fortress
	if (civ == "brit" || civ == "celt" || civ == "gaul" || civ == "iber")
	{
		var wall = ["gate", "tower", "wallLong",
			"cornerIn", "wallLong", "barracks", "tower", "wallLong", "tower", "house", "wallLong",
			"cornerIn", "wallLong", "house", "tower", "gate", "tower", "house", "wallLong",
			"cornerIn", "wallLong", "house", "tower", "wallLong", "tower", "house", "wallLong",
			"cornerIn", "wallLong", "house", "tower"];
	}
	else
	{
		var wall = ["gate", "tower", "wallLong",
			"cornerIn", "wallLong", "barracks", "tower", "wallLong", "tower", "wallLong",
			"cornerIn", "wallLong", "house", "tower", "gate", "tower", "wallLong",
			"cornerIn", "wallLong", "house", "tower", "wallLong", "tower", "wallLong",
			"cornerIn", "wallLong", "house", "tower"];
	}
	placeCustomFortress(playerX[i], playerZ[i], new Fortress("Spahbod", wall), civ, i+1, BUILDING_ANGlE);
}

// create lakes
log("Creating lakes...");
var numLakes = round(scaleByMapSize(1,4) * numPlayers);
var placer = new ClumpPlacer(scaleByMapSize(100,250), 0.8, 0.1, 10);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater],		// terrains
	[1,1]							// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 3);
var waterAreas = createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clWater)], 
	avoidClasses(clPlayer, 7, clWater, 20),
	numLakes
);

RMS.SetProgress(15);

// create reeds
log("Creating reeds...");
group = new SimpleGroup(
	[new SimpleObject(aReeds, 5,10, 0,4), new SimpleObject(aLillies, 0,1, 0,4)], true
);
createObjectGroupsByAreas(group, 0,
	[borderClasses(clWater, 3, 0), stayClasses(clWater, 1)],
	numLakes, 100,
	waterAreas
);

RMS.SetProgress(20);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 1,1, 0,1)],
	true, clFood
);
createObjectGroupsByAreas(group, 0,
	borderClasses(clWater, 2, 0),  avoidClasses(clFood, 8),
	numLakes, 50,
	waterAreas
);
waterAreas = [];

RMS.SetProgress(25);

// create bumps
createBumps(avoidClasses(clWater, 2, clPlayer, 5));

RMS.SetProgress(30);

// create hills
log("Creating hills...");
createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 5, clWater, 5, clHill, 15), clHill, scaleByMapSize(1, 4) * numPlayers);

RMS.SetProgress(35);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
g_numStragglerTrees = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tForestFloor, tGrass, pForestD], [tForestFloor, pForestD]],
	[[tForestFloor, tGrass, pForestO], [tForestFloor, pForestO]],
	[[tForestFloor, tGrass, pForestP], [tForestFloor, pForestP]]
];	// some variation
var size = numForest / (scaleByMapSize(3,6) * numPlayers);
var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), numForest / num, 0.5);
	var painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 5, clWater, 3, clForest, 15, clHill, 1),
		num
	);
}

RMS.SetProgress(50);

// create dirt patches
log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]],
 [1,1],
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 1)
);

RMS.SetProgress(55);

// create grass patches
log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tGrassPatch,
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 1)
);

RMS.SetProgress(60);

log("Creating stone mines...");
// create stone quarries
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clRock, 10, clHill, 1)
)

log("Creating metal mines...");
// create large metal quarries
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
)

RMS.SetProgress(70);

//create decoration
createDecoration
(
 [[new SimpleObject(aRockMedium, 1,3, 0,1)], 
  [new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
  [new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)],
  [new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)],
  [new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
 ],
 [
  scaleByMapSize(16, 262),
  scaleByMapSize(8, 131),
  scaleByMapSize(13, 200),
  scaleByMapSize(13, 200),
  scaleByMapSize(13, 200)
 ],
 avoidClasses(clWater, 0, clForest, 0, clPlayer, 1, clHill, 0)
);

RMS.SetProgress(80);

// create animals
createFood
(
 [
  [new SimpleObject(oSheep, 2,3, 0,2)],
  [new SimpleObject(oDeer, 5,7, 0,4)]
 ], 
 [
  3 * numPlayers,
  3 * numPlayers
 ],
 avoidClasses(clWater, 0, clForest, 0, clPlayer, 6, clHill, 1, clFood, 20)
);

// create fruits
createFood
(
 [
  [new SimpleObject(oBerryBush, 5,7, 0,4)]
 ], 
 [
  randInt(1, 4) * numPlayers + 2
 ],
 avoidClasses(clWater, 2, clForest, 0, clPlayer, 6, clHill, 1, clFood, 10)
);

RMS.SetProgress(90);

// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oBeech, oPine];	// some variation
createStragglerTrees(types, avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 1, clRock, 1));

setSkySet("sunny");
setWaterColour(0.157, 0.149, 0.443);
setWaterTint(0.443,0.42,0.824);
setWaterWaviness(2.5);
setWaterMurkiness(0.83);

setFogFactor(0.35);
setFogThickness(0.22);
setFogColor(0.82,0.82, 0.73);
setPPSaturation(0.56);
setPPContrast(0.56);
setPPBloom(0.38);
setPPEffect("hdr");

// Export map data
ExportMap();
