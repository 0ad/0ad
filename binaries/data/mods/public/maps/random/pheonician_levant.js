RMS.LoadLibrary("rmgen");

//TILE_CENTERED_HEIGHT_MAP = true;

const tCity = "medit_city_pavement";
const tCityPlaza = "medit_city_pavement";
const tHill = ["medit_dirt", "medit_dirt_b", "medit_dirt_c", "medit_rocks_grass", "medit_rocks_grass"];
const tMainDirt = "medit_dirt";
const tCliff = "medit_cliff_aegean";
const tForestFloor = "medit_rocks_shrubs";
const tGrass = "medit_rocks_grass";
const tRocksShrubs = "medit_rocks_shrubs";
const tRocksGrass = "medit_rocks_grass";
const tDirt = "medit_dirt_b";
const tDirtB = "medit_dirt_c";
const tShore = "medit_sand";
const tWater = "medit_sand_wet";

// gaia entities
const oGrapeBush = "gaia/flora_bush_grapes";
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
const pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor + TERRAIN_SEPARATOR + oCarob, tForestFloor, tForestFloor];

const BUILDING_ANGlE = -PI/4;

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

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
playerIDs = sortPlayers(playerIDs);

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
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
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
		[new SimpleObject(oGrapeBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 12;
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
		[new SimpleObject(oCarob, num, num, 0,5)],
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


log("Creating sea");
var theta = randFloat(0, 1);
var seed = randFloat(2,3);
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
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
	avoidClasses(clWater, 2, clPlayer, 20),
	scaleByMapSize(100, 200)
);

// create hills
log("Creating hills...");
placer = new ChainPlacer(1, floor(scaleByMapSize(4, 6)), floor(scaleByMapSize(16, 40)), 0.5);
var terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 15, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 20, clForest, 1, clHill, 15, clWater, 0),
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
var num = scaleByMapSize(10,42);
placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), numForest / (num * floor(scaleByMapSize(2,5))), 0.5);
painter = new TerrainPainter([tForestFloor, pForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 20, clForest, 10, clWater, 1, clHill, 1, clBaseResource, 3),
	num, 50
);

RMS.SetProgress(50);






// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[[tGrass,tRocksShrubs],[tRocksShrubs,tRocksGrass], [tRocksGrass,tGrass]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 10, clWater, 4, clDirt, 5, clHill, 1),
		scaleByMapSize(15, 45)
	);
}

RMS.SetProgress(55);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[[tDirt,tDirtB],[tDirt,tMainDirt], [tDirtB,tMainDirt]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 10, clWater, 4, clGrass, 5, clHill, 1),
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

log("Creating cyprus stone mines...");
// create cyprus large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

// create cyprus small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating cyprus metal mines...");
// create cyprus large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 3, clHill, 1),
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
	avoidClasses(clWater, 3, clPlayer, 0, clHill, 1),
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
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

// create goats
log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

// create deers
log("Creating deers...");
group = new SimpleGroup([new SimpleObject(oDeer, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

// create grape bushes
log("Creating grape bushes...");
group = new SimpleGroup(
	[new SimpleObject(oGrapeBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 15, clHill, 1, clFood, 7),
	randInt(1, 4) * numPlayers + 2, 50
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
setWaterColour(0.263,0.314,0.631);
setWaterTint(0.133, 0.725,0.855);
setWaterWaviness(2.0);
setWaterType("ocean");
setWaterMurkiness(0.8);

setTerrainAmbientColour(0.57, 0.58, 0.55);
setUnitsAmbientColour(0.447059, 0.509804, 0.54902);

setSunElevation(0.671884);
setSunRotation(-0.582913);

setFogFactor(0.2);
setFogThickness(0.15);
setFogColor(0.8, 0.7, 0.6);

setPPEffect("hdr");
setPPContrast(0.53);
setPPSaturation(0.47);
setPPBloom(0.52);

// Export map data
ExportMap();
