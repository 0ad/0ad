RMS.LoadLibrary("rmgen");

var tCity = "desert_city_tile";
var tCityPlaza = "desert_city_tile_plaza";
var tSand = ["desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_dunes_50", "desert_sand_smooth"];
var tDunes = "desert_sand_dunes_100";
var tFineSand = "desert_sand_smooth";
var tCliff = ["desert_cliff_badlands", "desert_cliff_badlands_2"];
var tForestFloor = "desert_forestfloor_palms";
var tGrass = "desert_dirt_rough_2";
var tGrassSand50 = "desert_sand_dunes_50";
var tGrassSand25 = "desert_dirt_rough";
var tDirt = "desert_dirt_rough";
var tDirtCracks = "desert_dirt_cracks";
var tShore = "desert_sand_wet";
var tLush = "desert_grass_a";
var tSLush = "desert_grass_a_sand";
var tSDry = "desert_plants_b";
// gaia entities
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oCamel = "gaia/fauna_camel";
var oFish = "gaia/fauna_fish";
var oGazelle = "gaia/fauna_gazelle";
var oGiraffe = "gaia/fauna_giraffe";
var oGoat = "gaia/fauna_goat";
var oWildebeest = "gaia/fauna_wildebeest";
var oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
var oStoneSmall = "gaia/geology_stone_desert_small";
var oMetalLarge = "gaia/geology_metal_desert_slabs";
var oDatePalm = "gaia/flora_tree_date_palm";
var oSDatePalm = "gaia/flora_tree_cretan_date_palm_short";
var eObelisk = "other/obelisk";
var ePyramid = "other/pyramid_minor";
var oWood = "gaia/special_treasure_wood";
var oFood = "gaia/special_treasure_food_bin";

// decorative props
var aBush1 = "actor|props/flora/bush_desert_a.xml";
var aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
var aBush3 = "actor|props/flora/bush_medit_sm_dry.xml";
var aBush4 = "actor|props/flora/plant_desert_a.xml";
var aBushes = [aBush1, aBush2, aBush3, aBush4];
var aDecorativeRock = "actor|geology/stone_desert_med.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
var aLillies = "actor|props/flora/water_lillies.xml";

// terrain + entity (for painting)
var pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];
var pForestOasis = [tGrass + TERRAIN_SEPARATOR + oDatePalm, tGrass + TERRAIN_SEPARATOR + oSDatePalm, tGrass];

const BUILDING_ANGlE = -PI/4;

log("Initializing map...");

InitMap();

var mapSize = getMapSize();
if (mapSize < 256)
{
	var aPlants = "actor|props/flora/grass_tropical.xml";
}
else
{
	var aPlants = "actor|props/flora/grass_tropic_field_tall.xml";
}
var numPlayers = getNumPlayers();
var mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clGrass = createTileClass();
var clDesert = createTileClass();
var clPond = createTileClass();
var clShore = createTileClass();
var clTreasure = createTileClass();

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = primeSortPlayers(sortPlayers(playerIDs));

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);
var playerPos = new Array(numPlayers);
var iop = 0;
for (var i = 0; i < numPlayers; i++)
{
	iop = i - 1;
	if (!(numPlayers%2)){
		playerPos[i] = ((iop + abs(iop%2))/2 + 1) / ((numPlayers / 2) + 1);
	}
	else
	{
		if (iop%2)
		{
			playerPos[i] = ((iop + abs(iop%2))/2 + 1) / (((numPlayers + 1) / 2) + 1);
		}
		else
		{
			playerPos[i] = ((iop)/2 + 1) / ((((numPlayers - 1)) / 2) + 1);
		}
	}
	playerZ[i] = playerPos[i];
	playerX[i] = 0.30 + 0.4*(i%2);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = floor(fx);
	var iz = floor(fz);
	addToClass(ix, iz, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tCityPlaza, tCity], [1]);
	createArea(placer, painter, null);
	
	// create starting units
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE);
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(oChicken, 5,5, 0,2)],
			true, clBaseResource, aX, aZ
		);
		createObjectGroup(group, 0);
	}
	
	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = radius - 4;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oDatePalm, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
	// create grass tufts
	var num = hillSize / 250;
	for (var j = 0; j < num; j++)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = radius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aBush1, 2,5, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gZ
		);
		createObjectGroup(group, 0);
	}
}

RMS.SetProgress(30);

const WATER_WIDTH = 0.1;
log("Creating river");
var theta = randFloat(0, 1);
var seed = randFloat(2,3);
var theta2 = randFloat(0, 1);
var seed2 = randFloat(2,3);
var rifp = 0;
var rifp2 = 0;
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		
		var h = 0;
		var distToWater = 0;
		
		h = 32 * (z - 0.5);
		if ((x < 0.25)||(x > 0.75))
		{
			addToClass(ix, iz, clDesert);
		}
		// add the rough shape of the water
		var km = 12/scaleByMapSize(35, 160);
		var cu = km*rndRiver(theta+z*0.5*(mapSize/64),seed)+(50/scaleByMapSize(35, 100))*rndRiver(theta2+z*0.5*(mapSize/128),seed2);
		var zk = z*randFloat(0.995,1.005);
		var xk = x*randFloat(0.995,1.005);
		if (-3.0 < getHeight(ix, iz)){
		if ((xk > cu+((1.0-WATER_WIDTH)/2))&&(xk < cu+((1.0+WATER_WIDTH)/2)))
		{
			if (xk < cu+((1.05-WATER_WIDTH)/2))
			{
				h = -3 + 200.0* abs(cu+((1.05-WATER_WIDTH)/2-xk));
				if ((h < 0.1)&&(h>-0.2))
				{
					if (rifp%2 == 0)
					{
						rifp = 0;
						placeObject(ix, iz, aPlants, 0, randFloat(0,TWO_PI));
					}
					++rifp;
				}
			}
			else if (xk > (cu+(0.95+WATER_WIDTH)/2))
			{
				h = -3 + 200.0*(xk-(cu+((0.95+WATER_WIDTH)/2)));
				if ((h < 0.1)&&(h>-0.2))
				{
					if (rifp2%2 == 0)
					{
						rifp2 = 0;
						placeObject(ix, iz, aPlants, 0, randFloat(0,TWO_PI));
					}
					++rifp2;
				}
			}
			else
			{
					h = -3.0;
			}
			setHeight(ix, iz, h);
			addToClass(ix, iz, clWater);
			placeTerrain(ix, iz, tShore);
		}
		}
		if (((xk > cu+((1.0-WATER_WIDTH)/2)-0.04)&&(xk < cu+((1.0-WATER_WIDTH)/2)))||((xk > cu+((1.0+WATER_WIDTH)/2))&&(xk < cu+((1.0+WATER_WIDTH)/2) + 0.04)))
		{
			placeTerrain(ix, iz, tLush);
			addToClass(ix, iz, clShore);
		}
		else if (((xk > cu+((1.0-WATER_WIDTH)/2)-0.06)&&(xk < cu+((1.0-WATER_WIDTH)/2)-0.04))||((xk > cu+((1.0+WATER_WIDTH)/2)+0.04)&&(xk < cu+((1.0+WATER_WIDTH)/2) + 0.06)))
		{
			placeTerrain(ix, iz, tSLush);
			addToClass(ix, iz, clShore);
		}
		else if (((xk > cu+((1.0-WATER_WIDTH)/2)-0.09)&&(xk < cu+((1.0-WATER_WIDTH)/2)-0.06))||((xk > cu+((1.0+WATER_WIDTH)/2)+0.06)&&(xk < cu+((1.0+WATER_WIDTH)/2) + 0.09)))
		{
			placeTerrain(ix, iz, tSDry);
			addToClass(ix, iz, clShore);
		}
	}
}

RMS.SetProgress(40);
// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 6),
	scaleByMapSize(100, 200)
);

// create ponds
log("Creating ponds...");
var numLakes = round(scaleByMapSize(1,4) * numPlayers / 2);
placer = new ClumpPlacer(scaleByMapSize(100,250), 0.8, 0.1, 10);
var terrainPainter = new LayeredPainter(
	[tShore, tShore, tShore],		// terrains
	[1,1]							// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -7, 4);
var waterAreas = createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clPond)], 
	avoidClasses(clPlayer, 25, clWater, 20, clPond, 10),
	numLakes
);

log("Creating reeds...");
group = new SimpleGroup(
	[new SimpleObject(aReeds, 1,3, 0,1)],
	true
);
createObjectGroupsByAreas(group, 0,
	stayClasses(clPond, 1),
	numLakes, 100,
	waterAreas
);

log("Creating lillies...");
group = new SimpleGroup(
	[new SimpleObject(aLillies, 1,3, 0,1)],
	true
);
createObjectGroupsByAreas(group, 0,
	stayClasses(clPond, 1),
	numLakes, 100,
	waterAreas
);

waterAreas = [];

// calculate desired number of trees for map (based on size)
const MIN_TREES = 700;
const MAX_TREES = 3500;
const P_FOREST = 0.5;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var num = scaleByMapSize(10,30);
placer = new ClumpPlacer(numForest / num, 0.15, 0.1, 0.5);
painter = new TerrainPainter([pForest, tForestFloor]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 19, clForest, 4, clWater, 1, clDesert, 5, clPond, 2, clBaseResource, 3),
	num, 50
);

RMS.SetProgress(50);

// create grass patches

log("Creating grass patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassSand50],[tGrassSand50,tGrassSand25], [tGrassSand25,tGrass]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 10, clWater, 1, clDirt, 5, clShore, 1, clPond, 1),
		scaleByMapSize(15, 45)
	);
}

RMS.SetProgress(55);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tDirt,tDirtCracks],[tDirt,tFineSand], [tDirtCracks,tFineSand]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 10, clWater, 1, clGrass, 5, clShore, 1, clPond, 1),
		scaleByMapSize(15, 45)
	);
}



RMS.SetProgress(60);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

RMS.SetProgress(65);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clPond, 1),
	scaleByMapSize(16, 262), 50
);


// create shrubs
log("Creating shrubs...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2), new SimpleObject(aBush4, 1,2, 0,1), new SimpleObject(aBush3, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 1, clPlayer, 0, clPond, 1),
	scaleByMapSize(20, 180), 50
);

RMS.SetProgress(70);

// create gazelles
log("Creating gazelles...");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

// create goats
log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

// create treasures
log("Creating treasures...");
group = new SimpleGroup([new SimpleObject(oFood, 1,1, 0,2)], true, clTreasure);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

group = new SimpleGroup([new SimpleObject(oWood, 1,1, 0,2)], true, clTreasure);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

// create camels
log("Creating camels...");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clTreasure, 2, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

RMS.SetProgress(90);

// create straggler trees
log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(0.5 * numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 20, clMetal, 1, clDesert, 1, clTreasure, 2, clPond, 1),
		num
	);
}

var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(0.1 * numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 20, clMetal, 1, clTreasure, 2),
		num
	);
}

// create pond trees
log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		borderClasses(clPond, 1, 4),
		num
	);
}

//create eyecandy
log("Creating obelisks");
group = new SimpleGroup(
	[new SimpleObject(eObelisk, 1,1, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	[avoidClasses(clWater, 4, clForest, 3, clPlayer, 20, clMetal, 2, clRock, 2, clPond, 4, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(5, 30), 50
);

log("Creating pyramids");
group = new SimpleGroup(
	[new SimpleObject(ePyramid, 1,1, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	[avoidClasses(clWater, 7, clForest, 6, clPlayer, 20, clMetal, 5, clRock, 5, clPond, 7, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(2, 6), 50
);

// Set environment
setSkySet("sunny");
setSunColour(0.711, 0.746, 0.574);	
setWaterColour(0.541,0.506,0.416);
setWaterTint(0.694,0.592,0.522);
setWaterMurkiness(1);
setWaterWaviness(2.5);

// Export map data
ExportMap();
