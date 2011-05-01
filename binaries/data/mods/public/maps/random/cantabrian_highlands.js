RMS.LoadLibrary("rmgen");

// terrain textures
const tGrass = ["medit_grass_field_a", "medit_grass_field_b"];
const tGrassForest = "medit_grass_wild";
const tCliff = ["medit_cliff_italia", "medit_cliff_italia_grass"];
const tGrassDirt75 = "medit_rocks_shrubs";
const tGrassDirt50 = "medit_rocks_grass_shrubs";
const tGrassDirt25 = "medit_rocks_grass";
const tDirt = "medit_dirt_b";
const tCity = "medit_city_tile";
const tGrassPatch = "medit_grass_wild";
const tShoreBlend = "medit_grass_field_brown";
const tShore = "medit_riparian_mud";
const tWater = "medit_riparian_mud";

// gaia entities
const oOak = "gaia/flora_tree_oak";
const oOakLarge = "gaia/flora_tree_oak_large";
const oApple = "gaia/flora_tree_apple";
const oPine = "gaia/flora_tree_pine";
const oAleppoPine = "gaia/flora_tree_aleppo_pine";
const oBerryBush = "gaia/flora_bush_berry";
const oChicken = "gaia/fauna_chicken";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStone = "gaia/geology_stone_mediterranean";
const oMetal = "gaia/geology_metal_mediterranean_slabs";

// decorative props
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/pond_lillies_large.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

// terrain + entity (for painting)
const pForestD = [tGrassForest + TERRAIN_SEPARATOR + oOak, tGrassForest + TERRAIN_SEPARATOR + oOakLarge, tGrassForest];
const pForestP = [tGrassForest + TERRAIN_SEPARATOR + oPine, tGrassForest + TERRAIN_SEPARATOR + oAleppoPine, tGrassForest];

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

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat(0, TWO_PI);
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*TWO_PI/numPlayers;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	log("Creating base for player " + (i + 1) + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// calculate size based on the radius
	var size = PI * radius * radius;
	
	// create the hill
	var placer = new ClumpPlacer(size, 0.95, 0.6, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tCliff, tGrass],		// terrains
		[cliffRadius]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		elevation,				// elevation
		cliffRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clPlayer)], null);
	
	// create the ramp
	var rampAngle = playerAngle[i] + PI + randFloat(-PI/8, PI/8);
	var rampDist = radius;
	var rampX = round(fx + rampDist * cos(rampAngle));
	var rampZ = round(fz + rampDist * sin(rampAngle));
	placer = new ClumpPlacer(100, 0.9, 0.5, 1, rampX, rampZ);
	var painter = new SmoothElevationPainter(ELEVATION_SET, elevation-6, 5);
	createArea(placer, painter, null);
	placer = new ClumpPlacer(75, 0.9, 0.5, 1, rampX, rampZ);
	painter = new TerrainPainter(tGrass);
	createArea(placer, painter, null);
	
	// create the city patch
	var cityRadius = 5;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	painter = new TerrainPainter(tCity);
	createArea(placer, painter, null);
	
	// create the TC
	var civ = getCivCode(i);
	var group = new SimpleGroup(	// elements (type, min/max count, min/max distance)
		[new SimpleObject("structures/"+civ+"_civil_centre", 1,1, 0,0)],
		true, null, ix, iz
	);
	createObjectGroup(group, i+1);
	
	// create starting units
	var uDist = 7;
	var uAngle = playerAngle[i] + PI + randFloat(-PI/8, PI/8);
	var ux = round(fx + uDist * cos(uAngle));
	var uz = round(fz + uDist * sin(uAngle));
	group = new SimpleGroup(	// elements (type, min/max count, min/max distance)
		[new SimpleObject("units/"+civ+"_support_female_citizen", 4,4, 1,2)],
		true, null, ux, uz
	);
	createObjectGroup(group, i+1);
	
	uAngle += PI/4;
	ux = round(fx + uDist * cos(uAngle));
	uz = round(fz + uDist * sin(uAngle));
	group = new SimpleGroup(	// elements (type, min/max count, min/max distance)
		[new SimpleObject("units/"+civ+"_infantry_javelinist_a", 4,4, 1,2)],
		true, null, ux, uz
	);
	createObjectGroup(group, i+1);
	
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
		[new SimpleObject(oMetal, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStone, 5,5, 0,3)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create starting straggler trees
	group = new SimpleGroup(
		[new SimpleObject(oOak, 5,5, 8,12)],
		true, clBaseResource, ix, iz
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
	// create grass tufts
	for (var j = 0; j < 10; j++)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = radius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aGrassShort, 2,5, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gZ
		);
		createObjectGroup(group, 0);
	}
}

RMS.SetProgress(5);

// create lakes
log("Creating lakes...");
var numLakes = round(scaleByMapSize(1,4) * numPlayers);
placer = new ClumpPlacer(scaleByMapSize(100,250), 0.8, 0.1, 10);
terrainPainter = new LayeredPainter(
	[tShoreBlend, tShore, tWater],		// terrains
	[1,1]							// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -7, 3);
var waterAreas = createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clWater)], 
	avoidClasses(clPlayer, 2, clWater, 20),
	numLakes
);

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

RMS.SetProgress(22);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 0),
	scaleByMapSize(100, 200)
);

RMS.SetProgress(25);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, [tGrass,tGrass,tGrassDirt75]],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 12, 2);
var hillAreas = createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 2, clWater, 5, clHill, 15),
	scaleByMapSize(1, 4) * numPlayers
);

RMS.SetProgress(30);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [pForestD, pForestP];	// some variation
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		[[tGrassForest, tGrass, types[i]], [tGrassForest, types[i]]],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 1, clWater, 3, clForest, 10, clHill, 0),
		num
	);
}

RMS.SetProgress(53);

log("Creating stone mines...");
// create stone
group = new SimpleGroup([new SimpleObject(oStone, 3,5, 0,8)], true, clRock);
createObjectGroupsByAreas(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clRock, 10), 
	 borderClasses(clHill, 1, 4)],
	8 * numPlayers, 100,
	hillAreas
);

log("Creating metal mines...");
// create metal
group = new SimpleGroup([new SimpleObject(oMetal, 1,1, 0,8)], true, clMetal);
createObjectGroupsByAreas(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clMetal, 10, clRock, 5), 
	 borderClasses(clHill, 1, 4)],
	scaleByMapSize(8,32) * numPlayers, 100,
	hillAreas
);
hillAreas = [];

// create dirt patches
log("Creating dirt patches...");
var sizes = [0.000183, 0.000321, 0.000458];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(mapArea * sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassDirt75],[tGrassDirt75,tGrassDirt50], [tGrassDirt50,tGrassDirt25]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [0.000115, 0.000206, 0.000298];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(mapArea * sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		scaleByMapSize(15, 45)
	);
}

RMS.SetProgress(60);

// create small decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	mapArea/1000, 50
);

// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	mapArea/2000, 50
);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	3 * numPlayers, 50
);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	3 * numPlayers, 50
);

// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oOakLarge, oPine, oApple];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1),
		num
	);
}

//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	mapArea * 0.000763
);

RMS.SetProgress(80);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	mapArea * 0.000763
);

RMS.SetProgress(87);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	mapArea * 0.000763, 50
);

// Set environment
setSkySet("cirrus");
setWaterTint(0.447, 0.412, 0.322);				// muddy brown
setWaterReflectionTint(0.447, 0.412, 0.322);	// muddy brown
setWaterMurkiness(1.0);
setWaterReflectionTintStrength(0.677);

// Export map data
ExportMap();
