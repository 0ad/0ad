RMS.LoadLibrary("rmgen");

//terrain textures
var tGrass = ["desert_dirt_rocks_1", "desert_dirt_cracks"];
var tGrassPForest = "forestfloor_dirty";
var tForestFloor = "desert_forestfloor_palms";
var tGrassA = "desert_grass_a_sand";
var tGrassB = "desert_grass_a";
var tGrassC = "medit_dirt_dry";
var tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_2"];
var tHill = ["desert_dirt_rocks_1", "desert_dirt_rocks_2", "desert_dirt_rocks_3"];
var tDirt = ["desert_dirt_rough", "desert_dirt_rough_2"];
var tRoad = "desert_shore_stones";;
var tRoadWild = "desert_grass_a_stones";;

// gaia entities
var oTamarix = "gaia/flora_tree_tamarix";
var oPalm = "gaia/flora_tree_date_palm";
var oPine = "gaia/flora_tree_aleppo_pine";
var oBush = "gaia/flora_bush_grapes";
var oChicken = "gaia/fauna_chicken";
var oCamel = "gaia/fauna_camel";
var oGazelle = "gaia/fauna_gazelle";
var oLion = "gaia/fauna_lion";
var oLioness = "gaia/fauna_lioness";
var oStoneLarge = "gaia/geology_stonemine_desert_quarry";
var oStoneSmall = "gaia/geology_stone_desert_small";
var oMetalLarge = "gaia/geology_metal_desert_slabs";

// decorative props
var aFlower1 = "actor|props/flora/decals_flowers_daisies.xml";
var aWaterFlower = "actor|props/flora/water_lillies.xml";
var aReedsA = "actor|props/flora/reeds_pond_lush_a.xml";
var aReedsB = "actor|props/flora/reeds_pond_lush_b.xml";
var aRock = "actor|geology/stone_desert_med.xml";
var rba6 = "actor|geology/stone_desert_med";
var aBushA = "actor|props/flora/bush_desert_dry_a.xml";
var aBushB = "actor|props/flora/bush_desert_dry_a.xml";
var aBushes = [aBushA, aBushB];
var aSand = "actor|particle/blowing_sand.xml";

var pForestP = [tForestFloor + TERRAIN_SEPARATOR + oPalm, tForestFloor];
var pForestT = [tGrassPForest + TERRAIN_SEPARATOR + oTamarix,tForestFloor];
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
var clGrass = createTileClass();

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

var startAngle = randFloat(0, TWO_PI);
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*TWO_PI/numPlayers;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// scale radius of player area by map size
	var radius = scaleByMapSize(15,25);
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// calculate size based on the radius
	var size = PI * radius * radius;
	
	// create the player area
	var placer = new ClumpPlacer(size, 0.9, 0.5, 10, ix, iz);
	createArea(placer, paintClass(clPlayer), null);
	
	// create the grass patches
	var grassRadius = scaleByMapSize(13 ,30);
	placer = new ClumpPlacer(PI*grassRadius*grassRadius, 0.3, 0.2, 10, ix, iz);
	var painter = new LayeredPainter([tGrassA, tGrassB], [3]);
	createArea(placer, [painter, paintClass(clGrass)], null);
	
	// create the city patch
	var cityRadius = 10;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [3]);
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
		[new SimpleObject(oBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 11;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0), new RandomObject(aBushes, 2,4, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2), new RandomObject(aBushes, 2,4, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create starting trees
	var num = 3;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject([oPalm, oTamarix][randInt(0,1)], num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

RMS.SetProgress(10);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800)
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 22, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 3, clGrass, 1, clHill, 10),
	scaleByMapSize(1, 4) * numPlayers * 3
);

RMS.SetProgress(25);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 400;
const MAX_TREES = 2000;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrass, tForestFloor, pForestP], [tForestFloor, pForestP]],
	[[tGrass, tGrassPForest, pForestT], [tGrassPForest, pForestT]]
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
		avoidClasses(clPlayer, 1, clGrass, 1, clWater, 3, clForest, 10, clHill, 1),
		num
	);
}

RMS.SetProgress(40);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(5, 32), scaleByMapSize(6, 48), scaleByMapSize(7, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tGrassC ,tDirt], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		painter,
		avoidClasses(clHill, 0, clForest, 0, clPlayer, 8, clGrass, 1),
		scaleByMapSize(50, 90)
	);
}
RMS.SetProgress(60);

// create big patches
log("Creating big patches...");
var sizes = [scaleByMapSize(10, 60), scaleByMapSize(15, 90), scaleByMapSize(20, 120)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tGrassC ,tDirt], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		painter,
		avoidClasses(clHill, 0, clForest, 0, clPlayer, 8, clGrass, 1),
		scaleByMapSize(30, 90)
	);
}
RMS.SetProgress(70);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clGrass, 1)],
	scaleByMapSize(2,8), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clGrass, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1, clGrass, 1)],
	scaleByMapSize(2,8), 100
);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRock, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);


//create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushB, 1,2, 0,1), new SimpleObject(aBushA, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(50, 500), 50
);

RMS.SetProgress(80);

// create gazelle
log("Creating gazelle...");
group = new SimpleGroup(
	[new SimpleObject(oGazelle, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clGrass, 2),
	3 * numPlayers, 50
);

// create lions
log("Creating lions...");
group = new SimpleGroup(
	[new SimpleObject(oLion, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clGrass, 2),
	3 * numPlayers, 50
);

// create camels
log("Creating camels...");
group = new SimpleGroup(
	[new SimpleObject(oCamel, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clGrass, 2),
	3 * numPlayers, 50
);

RMS.SetProgress(85);

// create straggler trees
log("Creating straggler trees...");
var types = [oPalm, oTamarix, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 1, clRock, 1),
		num
	);
}

// create inner straggler trees
log("Creating straggler trees...");
var types = [oPalm, oTamarix, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 1, clRock, 1), stayClasses(clGrass, 3)],
		num
	);
}

// Set environment
setSkySet("sunny");
setSunColour(0.746, 0.718, 0.539);	
setWaterColour(0.292, 0.347, 0.691);		
setWaterTint(0.550, 0.543, 0.437);				
setWaterReflectionTint(0.562, 0.511, 0.425);	
setWaterMurkiness(0.83);
setWaterReflectionTintStrength(0.377);

// Export map data
ExportMap();