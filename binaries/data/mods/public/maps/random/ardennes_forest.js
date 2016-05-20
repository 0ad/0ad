RMS.LoadLibrary("rmgen");

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

const tGrass = ["new_alpine_grass_b", "new_alpine_grass_c", "new_alpine_grass_d"];
const tPineForestFloor = "temp_forestfloor_pine";
const tForestFloor = [tPineForestFloor, tPineForestFloor, "alpine_dirt_grass_50"];
const tCliff = ["alpine_cliff_c", "alpine_cliff_c", "alpine_grass_rocky"];
const tCity = ["new_alpine_citytile", "new_alpine_grass_dirt_a"];
const tGrassPatch = ["alpine_grass_a", "alpine_grass_b"];

const oBoar = "gaia/fauna_boar";
const oDeer = "gaia/fauna_deer";
const oBear = "gaia/fauna_bear";
const oPig = "gaia/fauna_pig";
const oBerryBush = "gaia/flora_bush_berry";
const oMetalSmall = "gaia/geology_metal_alpine";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";
const oStoneSmall = "gaia/geology_stone_alpine_a";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";

const oOak = "gaia/flora_tree_oak";
const oOakLarge = "gaia/flora_tree_oak_large";
const oPine = "gaia/flora_tree_pine";
const oAleppoPine = "gaia/flora_tree_aleppo_pine";

const aTreeA = "actor|flora/trees/oak.xml";
const aTreeB = "actor|flora/trees/oak_large.xml";
const aTreeC = "actor|flora/trees/pine.xml";
const aTreeD = "actor|flora/trees/aleppo_pine.xml";

const aTrees = [aTreeA, aTreeB, aTreeC, aTreeD];

const aGrassLarge = "actor|props/flora/grass_soft_large.xml";
const aWoodLarge = "actor|props/special/eyecandy/wood_pile_1_b.xml";
const aWoodA = "actor|props/special/eyecandy/wood_sm_pile_a.xml";
const aWoodB = "actor|props/special/eyecandy/wood_sm_pile_b.xml";
const aBarrel = "actor|props/special/eyecandy/barrel_a.xml";
const aWheel = "actor|props/special/eyecandy/wheel_laying.xml";
const aCeltHomestead = "actor|structures/celts/homestead.xml";
const aCeltHouse = "actor|structures/celts/house.xml";
const aCeltLongHouse = "actor|structures/celts/longhouse.xml";

var pForest = [
		tPineForestFloor+TERRAIN_SEPARATOR+oOak, tForestFloor, 
		tPineForestFloor+TERRAIN_SEPARATOR+oPine, tForestFloor, 
		tPineForestFloor+TERRAIN_SEPARATOR+oAleppoPine, tForestFloor,
		tForestFloor
		];

// create tile classes

var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clForestJoin = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clPlayable = createTileClass();
var clHillDeco = createTileClass();

// Create central dip
var centerX = fractionToTiles(0.5);
var centerZ = fractionToTiles(0.5);
	
var placer = new ClumpPlacer(scaleByMapSize(mapSize * 70, mapSize * 300), 0.94, 0.05, 0.1, centerX, centerZ);
var elevationPainter = new SmoothElevationPainter(
	    ELEVATION_SET,
	    30,
	    3
    );
var painter = new LayeredPainter(
		[tCliff, tGrass],		// terrains
		[3]		// widths
	);
createArea(placer, 
	[painter, elevationPainter], 
	null);
	
RMS.SetProgress(5);
	
// Find all hills
var noise0 = new Noise2D(20);
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var h = getHeight(ix,iz);
		if(h > 40){
			addToClass(ix,iz,clHill);
			
			// Add hill noise
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
			var n = (noise0.get(x,z) - 0.5) * 40;
			setHeight(ix, iz, h + n);
		}
	}
}

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

var startAngle = randFloat() * 2 * PI;
for (var i=0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*2*PI/numPlayers;
	playerX[i] = 0.5 + 0.3*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.3*sin(playerAngle[i]);
}

function distanceToPlayers(x, z)
{
	var r = 10000;
	for (var i = 0; i < numPlayers; i++)
	{
		var dx = x - playerX[i];
		var dz = z - playerZ[i];
		r = min(r, dx*dx + dz*dz);
	}
	return sqrt(r);
}

function playerNearness(x, z)
{
	var d = fractionToTiles(distanceToPlayers(x,z));
	
	if (d < 13)
	{
		return 0;
	}
	else if (d < 19)
	{
		return (d-13)/(19-13);
	}
	else
	{
		return 1;
	}
}

RMS.SetProgress(10);

for (var i=0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// create starting units
	placeCivDefaultEntities(fx, fz, id);
	
	var citySize = 250

	var placer = new ClumpPlacer(citySize, 0.95, 0.3, 0.1, ix, iz);
	createArea(placer, [paintClass(clPlayer)], null);
	
	// Create the city patch
	var placer = new ClumpPlacer(citySize * 0.4, 0.6, 0.05, 10, ix, iz);
	var painter = new TerrainPainter([tCity]);
	createArea(placer, painter, null);

	// Create starter animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(oPig, 5,5, 0,2)],
			true, clBaseResource, aX, aZ
		);
		createObjectGroup(group, 0);
	}	
	
	// Create starter berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 3,3, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// Create starter metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = bbDist + 4;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// Create starter stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create starting trees
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oOak, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
}

RMS.SetProgress(30);

log("Creating hills...");
var sizes = [scaleByMapSize(50, 800), scaleByMapSize(50, 400), scaleByMapSize(10, 30), scaleByMapSize(10, 30)];
for (var i = 0; i < sizes.length; i++)
{
	var placer = new ClumpPlacer(sizes[i], 0.1, 0.2, 0.1);
	var painter = new LayeredPainter(
		[tCliff, [tForestFloor, tForestFloor, tCliff]],		// terrains
		[2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
	    ELEVATION_SET,
	    50,
	    sizes[i] < 50 ? 2 : 4
    );
	 
	var mountains = createAreas(
		placer,
		[painter, paintClass(clHill), elevationPainter], 
		avoidClasses(clPlayer, 8, clBaseResource, 2, clHill, 5),
		scaleByMapSize(1, 4)
	);
	
	
	if(sizes[i] > 100 && mountains.length > 0)
	{
		var placer = new ClumpPlacer(sizes[i] * 0.3, 0.94, 0.05, 0.1);
		var elevationPainter = new SmoothElevationPainter(
				 ELEVATION_MODIFY,
				 10,
				 3
			 );
		var painter = new LayeredPainter(
			[tCliff, tForestFloor],		// terrains
			[2]		// widths
		);
		
		createAreasInAreas(
			placer,
			[painter, elevationPainter], 
			stayClasses(clHill, 4),
			mountains.length * 2,
			20,
			mountains
		);
	}
	
	
	var placer = new ClumpPlacer(sizes[i], 0.1, 0.2, 0.1);
	
	var elevationPainter = new SmoothElevationPainter(
	    ELEVATION_SET,
	    10,
	    2
    );
	
	var ravine = createAreas(
		placer,
		[painter, paintClass(clHill), elevationPainter], 
		avoidClasses(clPlayer, 6, clBaseResource, 2, clHill, 5),
		scaleByMapSize(1, 3)
	);
	
	if(sizes[i] > 150 && ravine.length > 0)
	{
		// Place huts in ravines
		var group = new RandomGroup(
		[
			new SimpleObject(aCeltHouse, 0,1, 4,5),
			new SimpleObject(aCeltLongHouse, 1,1, 4,5)
		], true, clHillDeco);
		createObjectGroupsByAreas(
			group, 0,
			[avoidClasses(clHillDeco, 3), stayClasses(clHill, 3)],
			ravine.length * 5, 20,
			ravine
		);
		
		var group = new RandomGroup(
		[
			new SimpleObject(aCeltHomestead, 1,1, 1,1)
		], true, clHillDeco);
		createObjectGroupsByAreas(
			group, 0,
			[avoidClasses(clHillDeco, 5), stayClasses(clHill, 4)],
			ravine.length * 2, 100,
			ravine
		);
		
		// Place noise
		var placer = new ClumpPlacer(sizes[i] * 0.3, 0.94, 0.05, 0.1);
		var elevationPainter = new SmoothElevationPainter(
				 ELEVATION_SET,
				 2,
				 2
			 );
		var painter = new LayeredPainter(
			[tCliff, tForestFloor],		// terrains
			[2]		// widths
		);
		
		createAreasInAreas(
			placer,
			[painter, elevationPainter], 
			[avoidClasses(clHillDeco, 2), stayClasses(clHill, 0)],
			ravine.length * 2,
			20,
			ravine
		);
		
		var placer = new ClumpPlacer(sizes[i] * 0.1, 0.3, 0.05, 0.1);
		var elevationPainter = new SmoothElevationPainter(
				 ELEVATION_SET,
				 40,
				 2
			 );
		var painter = new LayeredPainter(
			[tCliff, tForestFloor],		// terrains
			[2]		// widths
		);
		
		createAreasInAreas(
			placer,
			[painter, paintClass(clHill), elevationPainter], 
			[avoidClasses(clHillDeco, 2), borderClasses(clHill, 15, 1)],
			ravine.length * 2,
			50,
			ravine
		);
	}
	
	RMS.SetProgress(30 + (20 * (i / sizes.length)));
}

RMS.SetProgress(50);

var explorableArea = {};
explorableArea.points = [];

var playerClass = getTileClass(clPlayer);
var hillDecoClass = getTileClass(clHillDeco);

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var h = getHeight(ix,iz);
		
		if(h > 15 && h < 45 && playerClass.countMembersInRadius(ix, iz, 1) == 0)
		{
			// explorable area
			var pt = {};
			pt.x = ix;
			pt.z = iz;
			explorableArea.points.push(pt);
		}
		
		if(h > 35)
		{
			var rnd = randFloat();
			if(g_Map.validT(ix, iz) && rnd < 0.1)
			{
				var i = randInt(aTrees.length);
				placeObject(ix+randFloat(), iz+randFloat(), aTrees[i], 0, randFloat(0, TWO_PI));
			}
		}
		else if(h < 15 && hillDecoClass.countMembersInRadius(ix, iz, 1) == 0)
		{
			var rnd = randFloat();
			if(g_Map.validT(ix, iz) && rnd < 0.05)
			{
				var i = randInt(aTrees.length);
				placeObject(ix+randFloat(), iz+randFloat(), aTrees[i], 0, randFloat(0, TWO_PI));
			}
		}
	}
}

RMS.SetProgress(55);

// Add some general noise - after placing height dependant trees
for (var ix = 0; ix < mapSize; ix++)
{
	var x = ix / (mapSize + 1.0);
	for (var iz = 0; iz < mapSize; iz++)
	{
		var z = iz / (mapSize + 1.0);
		var h = getHeight(ix,iz);
		var pn = playerNearness(x,z);
		var n = (noise0.get(x,z) - 0.5) * 10;
		setHeight(ix, iz, h + (n * pn));
	}
}

RMS.SetProgress(60);

// Calculate desired number of trees for map (based on size)
const MIN_TREES = 400;
const MAX_TREES = 6000;
const P_FOREST = 0.8;
const P_FOREST_JOIN = 0.25;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST * (1.0 - P_FOREST_JOIN);
var numForestJoin = totalTrees * P_FOREST * P_FOREST_JOIN;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var num = numForest / (scaleByMapSize(6,16) * numPlayers);
placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
painter = new TerrainPainter(pForest);
createAreasInAreas(
	placer,
	[painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 5, clBaseResource, 4, clForest, 6, clHill, 4),
	num,
	100,
	[explorableArea]
);

var num = numForestJoin / (scaleByMapSize(4,6) * numPlayers);
placer = new ClumpPlacer(numForestJoin / num, 0.1, 0.1, 1);
painter = new TerrainPainter(pForest);
createAreasInAreas(
	placer,
	[painter, paintClass(clForest), paintClass(clForestJoin)], 
	[avoidClasses(clPlayer, 5, clBaseResource, 4, clForestJoin, 5, clHill, 4), borderClasses(clForest, 1, 4)], 
	num,
	100,
	[explorableArea]
);


RMS.SetProgress(70);

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassPatch],[tGrassPatch,tGrass], [tGrass,tGrassPatch]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		painter,
		avoidClasses(clForest, 0, clHill, 2, clPlayer, 5),
		scaleByMapSize(15, 45)
	);
}

// create chopped forest  patches
log("Creating chopped forest patches...");
var sizes = [scaleByMapSize(20, 120)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tForestFloor);
	createAreas(
		placer,
		painter,
		avoidClasses(clForest, 1, clHill, 2, clPlayer, 5),
		scaleByMapSize(4, 12)
	);
}


RMS.SetProgress(75);

log("Creating stone mines...");
// create stone quarries
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 1,2, 0,4), new SimpleObject(oStoneLarge, 0,1, 0,4)], true, clRock);
createObjectGroupsByAreas(group, 0,
	[avoidClasses(clHill, 4, clForest, 2, clPlayer, 20, clRock, 10)],
	scaleByMapSize(6,20), 100,
	[explorableArea]
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsByAreas(group, 0,
	[avoidClasses(clHill, 4, clForest, 2, clPlayer, 20, clRock, 10)],
	scaleByMapSize(6,20), 100,
	[explorableArea]
);

log("Creating metal mines...");
// create metal quarries
group = new SimpleGroup([new SimpleObject(oMetalSmall, 1,2, 0,4), new SimpleObject(oMetalLarge, 0,1, 0,4)], true, clMetal);
createObjectGroupsByAreas(group, 0,
	[avoidClasses(clHill, 4, clForest, 2, clPlayer, 20, clMetal, 10, clRock, 5)],
	scaleByMapSize(6,20), 100,
	[explorableArea]
);

RMS.SetProgress(80);

// create wildlife
log("Creating wildlife...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsByAreas(group, 0,
	avoidClasses(clHill, 2, clForest, 0, clPlayer, 0, clBaseResource, 20),
	3 * numPlayers, 100,
	[explorableArea]
);

group = new SimpleGroup(
	[new SimpleObject(oBoar, 2,3, 0,5)],
	true, clFood
);
createObjectGroupsByAreas(group, 0,
	avoidClasses(clHill, 2, clForest, 0, clPlayer, 0, clBaseResource, 15),
	numPlayers, 50,
	[explorableArea]
);

group = new SimpleGroup(
	[new SimpleObject(oBear, 1,1, 0,4)],
	false, clFood
);
createObjectGroupsByAreas(group, 0,
	avoidClasses(clHill, 2, clForest, 0, clPlayer, 20),
	scaleByMapSize(3, 12), 200,
	[explorableArea]
);

RMS.SetProgress(85);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	randInt(3, 12) * numPlayers + 2, 50
);

log("Creating decorative props...");
group = new SimpleGroup(
	[
		new SimpleObject(aWoodA, 1,2, 0,1),
		new SimpleObject(aWoodB, 1,3, 0,1),
		new SimpleObject(aWheel, 0,2, 0,1),
		new SimpleObject(aWoodLarge, 0,1, 0,1),
		new SimpleObject(aBarrel, 0,2, 0,1)
	],
	true
);
createObjectGroupsByAreas(
	group, 0,
	avoidClasses(clForest, 0),
	scaleByMapSize(5, 50), 50,
	[explorableArea]
);

RMS.SetProgress(90);

// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oOakLarge, oPine, oAleppoPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	
	createObjectGroupsByAreas(group, 0,
		avoidClasses(clForest, 4, clHill, 5, clPlayer, 10, clBaseResource, 2, clMetal, 5, clRock, 5),
		num, 20,
		[explorableArea]
	);
}

RMS.SetProgress(95);

// create grass tufts
log("Creating grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassLarge, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsByAreas(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2),
	scaleByMapSize(50, 300), 20,
	[explorableArea]
);


setTerrainAmbientColor(0.44,0.51,0.56);
setUnitsAmbientColor(0.44,0.51,0.56);

// Export map data
ExportMap();
