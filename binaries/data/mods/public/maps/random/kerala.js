RMS.LoadLibrary("rmgen");

const tGrass = ["tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_plants", "tropic_plants", "tropic_plants_b"];
const tGrassA = "tropic_plants_c";
const tGrassB = "tropic_plants_c";
const tGrassC = "tropic_grass_c";
const tForestFloor = "tropic_grass_plants";
const tCliff = ["tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a_plants"];
const tPlants = "tropic_plants";
const tRoad = "tropic_citytile_a";
const tRoadWild = "tropic_citytile_plants";
const tShoreBlend = "tropic_beach_dry_plants";
const tShore = "tropic_beach_dry";
const tWater = "tropic_beach_wet";

// gaia entities
const oTree = "gaia/flora_tree_toona";
const oPalm = "gaia/flora_tree_palm_tropic";
const oStoneLarge = "gaia/geology_stonemine_tropic_quarry";
const oStoneSmall = "gaia/geology_stone_tropic_a";
const oMetalLarge = "gaia/geology_metal_tropic_slabs";
const oFish = "gaia/fauna_fish";
const oDeer = "gaia/fauna_deer";
const oSheep = "gaia/fauna_tiger";
const oChicken = "gaia/fauna_chicken";
const oBush = "gaia/flora_bush_berry";

// decorative props
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBush1 = "actor|props/flora/plant_tropic_a.xml";
const aBush2 = "actor|props/flora/plant_lg.xml";
const aBush3 = "actor|props/flora/plant_tropic_large.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oTree, tForestFloor];
const pForestP = [tForestFloor + TERRAIN_SEPARATOR + oPalm, tForestFloor];
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
var clHill2 = createTileClass();
var clHill3 = createTileClass();
var clHill4 = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clMountains = createTileClass();

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
	playerX[i] = 0.45 + 0.2*(i%2)
	playerZ[i] = playerPos[i];
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
	var ix = round(fx);
	var iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
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
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	var group = new SimpleGroup(
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
	var num = floor(hillSize / 60);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
}

RMS.SetProgress(15);

// create western sea
var fadedistance = 8;

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		if (ix < 0.31 * mapSize)
		{
			if (ix > 0.31 * mapSize - fadedistance)
			{
				setHeight(ix, iz, 3 - 8 * (0.31 * mapSize - ix) / fadedistance);
				if (ix, iz, 3 - 8 * (0.31 * mapSize - ix) / fadedistance < 0.5)
					addToClass(ix, iz, clWater);
			}
			else
			{
				setHeight(ix, iz, -5);
				addToClass(ix, iz, clWater);
			}
		}
		else if (ix > 0.69 * mapSize)
		{
			addToClass(ix, iz, clMountains)
		}
	}
}

// create shore
log("Creating shores...");
for (var i = 0; i < scaleByMapSize(20,120); i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(4, 6)), floor(scaleByMapSize(16, 30)), 1, floor(randFloat(0.28,0.34)*mapSize), floor(randFloat(0.1,0.9)*mapSize));
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass],		// terrains
		[2]								// widths
	);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 3);
	createArea(
		placer,
		[terrainPainter, elevationPainter, unPaintClass(clWater)], 
		null
	);
}

paintTerrainBasedOnHeight(-6, 1, 1, tWater);
paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTerrainBasedOnHeight(0, 1, 1, tShore);
paintTileClassBasedOnHeight(-6, 0.5, 1, clWater)

RMS.SetProgress(45);

// create hills
log("Creating hills...");
placer = new ChainPlacer(1, floor(scaleByMapSize(4, 6)), floor(scaleByMapSize(16, 40)), 0.1);
var terrainPainter = new LayeredPainter(
	[tCliff, tGrass],		// terrains
	[3]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 25, 3);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	[avoidClasses(clPlayer, 20, clHill, 5, clWater, 2, clBaseResource, 2), stayClasses(clMountains, 0)],
	scaleByMapSize(5, 40) * numPlayers
);


// calculate desired number of trees for map (based on size)

var MIN_TREES = 1000;
var MAX_TREES = 6000;
var P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrass, tGrass, tGrass, tGrass, pForestD], [tGrass, tGrass, tGrass, pForestD]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP], [tGrass, tGrass, tGrass, pForestP]]
];	// some variation


var size = numForest / (scaleByMapSize(3,6) * numPlayers);

var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), numForest / (num * floor(scaleByMapSize(2,4))), 0.5);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 20, clForest, 10, clHill, 0, clWater, 8),
		num
	);
}

RMS.SetProgress(70);

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[tGrassC,tGrassA,tGrassB], 		// terrains
		[2,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 8, clForest, 0, clHill, 0, clPlayer, 12, clDirt, 16),
		scaleByMapSize(20, 80)
	);
}
var sizes = [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[tPlants,tPlants], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 8, clForest, 0, clHill, 0, clPlayer, 12, clDirt, 16),
		scaleByMapSize(20, 80)
	);
}

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	3*scaleByMapSize(16, 262), 50
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
	3*scaleByMapSize(8, 131), 50
);

//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aBush1, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	8 * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aBush1, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	8 * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBush3, 1,2, 0,2), new SimpleObject(aBush2, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	8 * scaleByMapSize(13, 200), 50
);

RMS.SetProgress(95);

// create straggler trees
log("Creating straggler trees...");
var types = [oTree, oPalm];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

RMS.SetProgress(75);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 6, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randInt(1, 4) * numPlayers + 2, 50
);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 22, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

setSunColour(0.6, 0.6, 0.6);	
setSunElevation(PI/ 3);

setWaterColour(0.524,0.734,0.839);
setWaterTint(0.369,0.765,0.745);
setWaterWaviness(1.5);
setWaterMurkiness(0.35);

setFogFactor(0.4);
setFogThickness(0.2);

setPPEffect("hdr");
setPPContrast(0.7);
setPPSaturation(0.65);
setPPBloom(0.6);

setSkySet("cirrus");
ExportMap();
