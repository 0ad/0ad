RMS.LoadLibrary("rmgen");

function rndRiver(f, seed)
{
	var rndRq = seed;
	var rndRw = rndRq;
	var rndRe = 0;
	var rndRr = f-floor(f);
	var rndRa = 0;
	for (var rndRx=0; rndRx<=floor(f); rndRx++)
	{
		rndRw = 10*(rndRw-floor(rndRw));
	}
	if (rndRx%2==0)
	{
		var rndRs = -1;
	}
	else
	{
		var rndRs = 1;
	}
	rndRe = (floor(rndRw))%5;
	if (rndRe==0)
	{
		rndRa = (rndRs)*2.3*(rndRr)*(rndRr-1)*(rndRr-0.5)*(rndRr-0.5);
	}
	else if (rndRe==1)
	{
		rndRa = (rndRs)*2.6*(rndRr)*(rndRr-1)*(rndRr-0.3)*(rndRr-0.7);
	}
	else if (rndRe==2)
	{
		rndRa = (rndRs)*22*(rndRr)*(rndRr-1)*(rndRr-0.2)*(rndRr-0.3)*(rndRr-0.3)*(rndRr-0.8);
	}
	else if (rndRe==3)
	{
		rndRa = (rndRs)*180*(rndRr)*(rndRr-1)*(rndRr-0.2)*(rndRr-0.2)*(rndRr-0.4)*(rndRr-0.6)*(rndRr-0.6)*(rndRr-0.8);
	}
	else if (rndRe==4)
	{
		rndRa = (rndRs)*2.6*(rndRr)*(rndRr-1)*(rndRr-0.5)*(rndRr-0.7);
	}
	return rndRa;
}

const tCity = "desert_city_tile";
const tCityPlaza = "desert_city_tile_plaza";
const tSand = ["desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_dunes_50", "desert_sand_smooth"];
const tDunes = "desert_sand_dunes_100";
const tFineSand = "desert_sand_smooth";
const tCliff = ["desert_cliff_badlands", "desert_cliff_badlands_2"];
const tForestFloor = "desert_forestfloor_palms";
const tGrass = "desert_dirt_rough_2";
const tGrassSand50 = "desert_sand_dunes_50";
const tGrassSand25 = "desert_dirt_rough";
const tDirt = "desert_dirt_rough";
const tDirtCracks = "desert_dirt_cracks";
const tShore = "desert_sand_wet";
const tLush = "desert_grass_a";
const tSLush = "desert_grass_a_sand";
const tSDry = "desert_plants_b";
// gaia entities
const oBerryBush = "gaia/flora_bush_berry";
const oChicken = "gaia/fauna_chicken";
const oCamel = "gaia/fauna_camel";
const oFish = "gaia/fauna_fish";
const oGazelle = "gaia/fauna_gazelle";
const oGiraffe = "gaia/fauna_giraffe";
const oGoat = "gaia/fauna_goat";
const oWildebeest = "gaia/fauna_wildebeest";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oDatePalm = "gaia/flora_tree_date_palm";
const oSDatePalm = "gaia/flora_tree_cretan_date_palm_short";
const eObelisk = "other/obelisk";
const ePyramid = "other/pyramid_minor";
const oWood = "gaia/special_treasure_wood";
const oFood = "gaia/special_treasure_food_bin";

// decorative props
const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_medit_sm_dry.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_desert_med.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";

// terrain + entity (for painting)
var pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];
var pForestOasis = [tGrass + TERRAIN_SEPARATOR + oDatePalm, tGrass + TERRAIN_SEPARATOR + oSDatePalm, tGrass];

const BUILDING_ANGlE = 0.75*PI;

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
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
playerIDs = shuffleArray(playerIDs);

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
	
	// get civ specific starting entities
	var civEntities = getStartingEntities(id-1);
	
	// create the TC
	var group = new SimpleGroup(	// elements (type, min/max count, min/max distance, min/max angle)
		[new SimpleObject(civEntities[0].Template, 1,1, 0,0, BUILDING_ANGlE, BUILDING_ANGlE)],
		true, null, ix, iz
	);
	createObjectGroup(group, id);
	
	// create starting units
	var uDist = 6;
	var uSpace = 2;
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = -BUILDING_ANGlE + PI * (j - 1) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, id, (j % 2 - 1) * PI + uAngle);
		}
	}
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		group = new SimpleGroup(
			[new SimpleObject(oChicken, 5,5, 0,3)],
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
	// create starting straggler trees
	var num = hillSize / 100;
	for (var j = 0; j < num; j++)
	{
		var tAngle = randFloat(0, TWO_PI);
		var tDist = randFloat(6, radius - 2);
		var tX = round(fx + tDist * cos(tAngle));
		var tZ = round(fz + tDist * sin(tAngle));
		group = new SimpleGroup(
			[new SimpleObject(oSDatePalm, 1,3, 0,2)],
			false, clBaseResource, tX, tZ
		);
		createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	}
	
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
for (ix = 0; ix < mapSize; ix++)
{
	for (iz = 0; iz < mapSize; iz++)
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
			
			}
			else if (xk > (cu+(0.95+WATER_WIDTH)/2))
			{
				h = -3 + 200.0*(xk-(cu+((0.95+WATER_WIDTH)/2)));
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
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -7, 3);
var waterAreas = createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clPond)], 
	avoidClasses(clPlayer, 25, clWater, 10),
	numLakes
);
waterAreas = [];

log("Creating reeds...");
group = new SimpleGroup(
	[new SimpleObject(aReeds, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	stayClasses(clWater, 1, clPond, 1),
	scaleByMapSize(300, 762), 50
);

log("Creating lillies...");
group = new SimpleGroup(
	[new SimpleObject(aLillies, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	stayClasses(clWater, 1, clPond, 1),
	scaleByMapSize(300, 762), 50
);

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
	avoidClasses(clPlayer, 8, clForest, 4, clWater, 1, clDesert, 5, clPond, 2),
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
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 0, clWater, 1, clDirt, 5, clShore, 1, clPond, 1),
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
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 0, clWater, 1, clGrass, 5, clShore, 1, clPond, 1),
		scaleByMapSize(15, 45)
	);
}



RMS.SetProgress(60);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 12, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 12, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 12, clMetal, 10, clRock, 5, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 12, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 12, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 12, clMetal, 10, clRock, 5, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
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
	scaleByMapSize(40, 360), 50
);

RMS.SetProgress(70);

// create gazelles
log("Creating gazelles...");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

// create goats
log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

// create treasures
log("Creating treasures...");
group = new SimpleGroup([new SimpleObject(oFood, 1,1, 0,2)], true, clTreasure);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

group = new SimpleGroup([new SimpleObject(oWood, 1,1, 0,2)], true, clTreasure);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

// create camels
log("Creating camels...");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 10, clDesert, 5, clTreasure, 2, clPond, 1),
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
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 8, clMetal, 1, clDesert, 1, clTreasure, 2, clPond, 1),
		num
	);
}

var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(0.1 * numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 8, clMetal, 1, clTreasure, 2),
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
		borderClasses(clPond, 0, 4),
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
	[avoidClasses(clWater, 4, clForest, 3, clPlayer, 5, clMetal, 2, clRock, 2, clPond, 4, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(5, 30), 50
);

log("Creating pyramids");
group = new SimpleGroup(
	[new SimpleObject(ePyramid, 1,1, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	[avoidClasses(clWater, 7, clForest, 6, clPlayer, 14, clMetal, 5, clRock, 5, clPond, 7, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(2, 6), 50
);

// Set environment
setSkySet("sunny");
setSunColour(0.873, 0.846, 0.674);	
setWaterColour(0.312, 0.562, 0.652);		
setWaterTint(0.412, 0.212, 0.212);				
setWaterReflectionTint(0.447, 0.202, 0.222);	
setWaterMurkiness(1.0);
setWaterReflectionTintStrength(0.677);

// Export map data
ExportMap();
