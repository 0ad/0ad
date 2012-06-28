RMS.LoadLibrary("rmgen");

var tGrass = ["temp_grass", "temp_grass", "temp_grass_d"];
var tGrassPForest = "temp_plants_bog";
var tGrassDForest = "temp_plants_bog";
var tGrassA = "temp_grass_plants";
var tGrassB = "temp_plants_bog";
var tGrassC = ["temp_grass_b", "temp_grass_c"];
var tDirt = ["temp_plants_bog", "temp_mud_a"];
var tHill = ["temp_highlands", "temp_grass_long_b"];
var tCliff = ["temp_cliff_a", "temp_cliff_b"];
var tRoad = "temp_road";
var tRoadWild = "temp_road_overgrown";
var tGrassPatch = "temp_grass_plants";
var tShoreBlend = "temp_grass_plants";
var tShore = "temp_plants_bog";
var tWater = "temp_mud_a";

// gaia entities
var oBeech = "gaia/flora_tree_euro_beech";
var oOak = "gaia/flora_tree_oak";
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_deer";
var oFish = "gaia/fauna_fish";
var oSheep = "gaia/fauna_rabbit";
var oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
var oStoneSmall = "gaia/geology_stone_temperate";
var oMetalLarge = "gaia/geology_metal_temperate_slabs";
var oWood = "gaia/special_treasure_wood";
var oFood = "gaia/special_treasure_food_bin";
var oMetal = "gaia/special_treasure_metal";
var oStone = "gaia/special_treasure_stone";

// decorative props
var aGrass = "actor|props/flora/grass_soft_small_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_large.xml";
var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
var aLillies = "actor|props/flora/water_lillies.xml";
var aBushMedium = "actor|props/flora/bush_medit_me.xml";
var aBushSmall = "actor|props/flora/bush_medit_sm.xml";

var pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oBeech, tGrassDForest];
var pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oOak, tGrassPForest];
const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

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
	var civ = g_MapSettings.PlayerData[i].Civ;
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
	var placer = new ClumpPlacer(PI*baseRadius*baseRadius/4, 1/2, 1/8, 10, playerX[i], playerZ[i]);
	createArea(placer,[new TerrainPainter(tRoad), paintClass(clPlayer)]);
	
	// Place custom fortress
	if (civ == "brit" || civ == "celt" || civ == "gaul" || civ == "iber")
	{
		var wall = ['entryTower', 'wall', 'wall',
			'cornerIn', 'wall', 'barracks', 'wall', 'gate', 'wall', 'house', 'wall',
			'cornerIn', 'wall', 'house', 'wall', 'entryTower', 'wall', 'house', 'wall',
			'cornerIn', 'wall', 'house', 'wall', 'gate', 'wall', 'house', 'wall',
			'cornerIn', 'wall', 'house', 'wall'];
	}
	else
	{
		var wall = ['entryTower', 'wall', 'wall',
			'cornerIn', 'wall', 'barracks', 'wall', 'gate', 'wall', 'wall',
			'cornerIn', 'wall', 'house', 'wall', 'entryTower', 'wall', 'wall',
			'cornerIn', 'wall', 'house', 'wall', 'gate', 'wall', 'wall',
			'cornerIn', 'wall', 'house', 'wall'];
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
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 12, 2);
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
	[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]],
	[[tGrassPForest, tGrass, pForestP], [tGrassPForest, pForestP]]
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

RMS.SetProgress(80);

// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oBeech];	// some variation
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

// Export map data
ExportMap();
