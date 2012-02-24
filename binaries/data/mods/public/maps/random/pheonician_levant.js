RMS.LoadLibrary("rmgen");

//TILE_CENTERED_HEIGHT_MAP = true;
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

const tCity = "medit_city_pavement";
const tCityPlaza = "medit_city_pavement";
const tHill = ["medit_dirt", "medit_dirt_b", "medit_dirt_c", "medit_rocks_grass", "medit_rocks_grass"];
const tMainDirt = "medit_dirt";
const tCliff = "medit_cliff_aegean";
const tForestFloor = "medit_rocks_shrubs";
const tGrass = "medit_rocks_grass";
const tGrassSand50 = "medit_rocks_shrubs";
const tGrassSand25 = "medit_rocks_grass";
const tDirt = "medit_dirt_b";
const tDirtCracks = "medit_dirt_c";
const tShore = "medit_sand";
const tWater = "medit_sand_wet";

// gaia entities
const oBerryBush = "gaia/flora_bush_berry";
const oChicken = "gaia/fauna_chicken";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oGoat = "gaia/fauna_goat";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";
const oDatePalm = "gaia/flora_tree_cretan_date_palm_short";
const oSDatePalm = "gaia/flora_tree_cretan_date_palm_tall";
const oCarob = "gaia/flora_tree_carob";
const oFanPalm = "gaia/flora_tree_medit_fan_palm";
const oPoplar = "gaia/flora_tree_poplar_lombardy";
const oCypress = "gaia/flora_tree_cypress";

// decorative props
const aBush1 = "actor|props/flora/bush_medit_sm.xml";
const aBush2 = "actor|props/flora/bush_medit_me.xml";
const aBush3 = "actor|props/flora/bush_medit_la.xml";
const aBush4 = "actor|props/flora/bush_medit_me.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_granite_med.xml";

// terrain + entity (for painting)
var pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];

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
var clHill = createTileClass();
var clIsland = createTileClass();

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
var playerPos = new Array(numPlayers);

for (var i = 0; i < numPlayers; i++)
{
	playerPos[i] = (i + 1) / (numPlayers + 1);
	playerZ[i] = playerPos[i];
	playerX[i] = 0.66 + 0.2*(i%2)
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


log("Creating sea");
var theta = randFloat(0, 1);
var seed = randFloat(2,3);
for (ix = 0; ix < mapSize; ix++)
{
	for (iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		
		// add the rough shape of the water
		var km = 20/scaleByMapSize(35, 160);
		var cu = km*rndRiver(theta+z*0.5*(mapSize/64),seed);
		
		var fadeDist = 0.05;
		
		if (x < cu + 0.5)
		{
			var h;
			if (x < (cu + 0.5 + fadeDist))
			{
				h = 1 + 4.0 * (1 - ((cu + 0.5 + fadeDist) - x)/fadeDist);
			}
			else
			{
				h = -3.0;
			}
			
			if (h < -1.5)
			{
				placeTerrain(ix, iz, tWater);
			}
			else
			{
				placeTerrain(ix, iz, tShore);
			}
			
			setHeight(ix, iz, h);
			if (h < 0){
				addToClass(ix, iz, clWater);
			}
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

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 15, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 16, clForest, 1, clHill, 15, clWater, 0),
	scaleByMapSize(1, 4) * numPlayers * 3
);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.5;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var num = scaleByMapSize(10,30);
placer = new ClumpPlacer(numForest / num, 0.15, 0.1, 0.5);
painter = new TerrainPainter([tForestFloor, pForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 8, clForest, 10, clWater, 1, clHill, 1),
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
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 10, clWater, 1, clDirt, 5, clHill, 1),
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
		[[tDirt,tDirtCracks],[tDirt,tMainDirt], [tDirtCracks,tMainDirt]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 10, clWater, 1, clGrass, 5, clHill, 1),
		scaleByMapSize(15, 45)
	);
}



RMS.SetProgress(60);

// create cyprus
log("Creating cyprus...");
placer = new ClumpPlacer(4.5 * scaleByMapSize(60, 540), 0.2, 0.1, 0.01);
var terrainPainter = new LayeredPainter(
	[tShore, tHill],		// terrains
	[12]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 6, 8);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clIsland)], 
	[stayClasses (clWater, 5)],
	1
);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 12, clRock, 10, clWater, 1, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 12, clRock, 10, clWater, 1, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 12, clMetal, 10, clRock, 5, clWater, 1, clHill, 1),
	scaleByMapSize(4,16), 100
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
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 1),
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
	avoidClasses(clWater, 1, clPlayer, 0, clHill, 1),
	scaleByMapSize(40, 360), 50
);

RMS.SetProgress(70);

// create fish
log("Creating fish...");
group = new SimpleGroup([new SimpleObject(oFish, 1,3, 2,6)], true, clFood);
createObjectGroups(group, 0,
	[avoidClasses(clIsland, 2, clFood, 10), stayClasses(clWater, 5)],
	3*scaleByMapSize(5,20), 50
);

// create sheeps
log("Creating sheeps...");
group = new SimpleGroup([new SimpleObject(oSheep, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

// create goats
log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

// create deers
log("Creating deers...");
group = new SimpleGroup([new SimpleObject(oDeer, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 1, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

RMS.SetProgress(90);

// create straggler trees
log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm, oCarob, oFanPalm, oPoplar, oCypress];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 8, clMetal, 1, clHill, 1),
		num
	);
}

log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm, oCarob, oFanPalm, oPoplar, oCypress];	// some variation
var num = 3*floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		stayClasses(clIsland, 9),
		num
	);
}

// Set environment
setSkySet("sunny");
setSunColour(0.917, 0.828, 0.734);	
setWaterColour(0.292, 0.347, 0.691);		

// Export map data
ExportMap();

// Export map data
ExportMap();

