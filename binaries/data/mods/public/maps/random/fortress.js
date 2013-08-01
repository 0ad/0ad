RMS.LoadLibrary("rmgen");

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
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
var painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 5),
	scaleByMapSize(100, 200)
);

RMS.SetProgress(30);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 17, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 5, clWater, 5, clHill, 15),
	scaleByMapSize(1, 4) * numPlayers
);

RMS.SetProgress(35);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tForestFloor, tGrass, pForestD], [tForestFloor, pForestD]],
	[[tForestFloor, tGrass, pForestO], [tForestFloor, pForestO]],
	[[tForestFloor, tGrass, pForestP], [tForestFloor, pForestP]]
];	// some variation
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 5, clWater, 3, clForest, 7, clHill, 1),
		num
	);
}

RMS.SetProgress(40);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 1),
		scaleByMapSize(15, 45)
	);
}

RMS.SetProgress(45);

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 1),
		scaleByMapSize(15, 45)
	);
}

RMS.SetProgress(50);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clRock, 10, clHill, 1)],
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clRock, 10, clHill, 1)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 5, clMetal, 10, clRock, 5, clHill, 1)],
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(60);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

RMS.SetProgress(65);

// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 6, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 6, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randInt(1, 4) * numPlayers + 2, 50
);

RMS.SetProgress(80);

// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oBeech, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 4, clMetal, 1, clRock, 1),
		num
	);
}

RMS.SetProgress(85);

//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	scaleByMapSize(13, 200), 50
);

setSkySet("sunny");
setWaterColour(0.157, 0.149, 0.443);
setWaterTint(0.443,0.42,0.824);
setWaterReflectionTint(0.863,0.667,0.608);
setWaterWaviness(2.5);
setWaterMurkiness(0.83);
setWaterReflectionTintStrength(0.35);

// Export map data
ExportMap();
