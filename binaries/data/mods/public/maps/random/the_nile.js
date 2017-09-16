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

var oBerryBush = "gaia/flora_bush_berry";
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

InitMap();

var mapSize = getMapSize();
var aPlants = mapSize < 256 ?
	"actor|props/flora/grass_tropical.xml" :
	"actor|props/flora/grass_tropic_field_tall.xml";

var numPlayers = getNumPlayers();
var mapArea = mapSize*mapSize;

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

var playerIDs = primeSortAllPlayers();
var playerPos = placePlayersRiver();
var playerX = [];
var playerZ = [];

for (var i = 0; i < numPlayers; i++)
{
	playerZ[i] = playerPos[i];
	playerX[i] = 0.30 + 0.4*(i%2);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

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

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);
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

	placeDefaultDecoratives(fx, fz, aBush1, clBaseResource, radius);
}

RMS.SetProgress(30);

const riverTextures = [
	{
		"left": 0,
		"right": 0.04,
		"tileClass": tLush
	},
	{
		"left": 0.04,
		"right": 0.06,
		"tileClass": tSLush
	},
	{
		"left": 0.06,
		"right": 0.09,
		"tileClass": tSDry
	}
];

const plantFrequency = 2;

var plantID = 0;

paintRiver({
	"horizontal": false,
	"parallel": true,
	"position": 0.5,
	"width": 0.1,
	"fadeDist": 0.025,
	"deviation": 0.005,
	"waterHeight": -3,
	"landHeight": 2,
	"meanderShort": 12,
	"meanderLong": 50,
	"waterFunc": (ix, iz, height) => {

		addToClass(ix, iz, clWater);
		placeTerrain(ix, iz, tShore);

		// Place river bushes
		if (height <= -0.2 || height >= 0.1)
			return;

		if (plantID % plantFrequency == 0)
		{
			plantID = 0;
			placeObject(ix, iz, aPlants, 0, randFloat(0, TWO_PI));
		}
		++plantID;
	},
	"landFunc": (ix, iz, shoreDist1, shoreDist2) => {

		let x = ix / (mapSize + 1.0);
		if (x < 0.25 || x > 0.75)
			addToClass(ix, iz, clDesert);

		for (let riv of riverTextures)
			if (-shoreDist1 > -riv.right && -shoreDist1 < -riv.left ||
				-shoreDist2 > riv.left && -shoreDist2 < riv.right)
			{
				placeTerrain(ix, iz, riv.tileClass);
				addToClass(ix, iz, clShore);
			}
	}
});

RMS.SetProgress(40);

log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter,
	avoidClasses(clWater, 2, clPlayer, 6),
	scaleByMapSize(100, 200)
);

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
createObjectGroupsByAreasDeprecated(group, 0,
	stayClasses(clPond, 1),
	numLakes, 100,
	waterAreas
);

log("Creating lillies...");
group = new SimpleGroup(
	[new SimpleObject(aLillies, 1,3, 0,1)],
	true
);
createObjectGroupsByAreasDeprecated(group, 0,
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

log("Creating forests...");
var num = scaleByMapSize(10,30);
placer = new ClumpPlacer(numForest / num, 0.15, 0.1, 0.5);
painter = new TerrainPainter([pForest, tForestFloor]);
createAreas(placer, [painter, paintClass(clForest)],
	avoidClasses(clPlayer, 19, clForest, 4, clWater, 1, clDesert, 5, clPond, 2, clBaseResource, 3),
	num, 50
);

RMS.SetProgress(50);

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
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clPond, 1),
	scaleByMapSize(4,16), 100
);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 1, clPond, 1), stayClasses(clDesert, 3)],
	scaleByMapSize(6,20), 100
);

RMS.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clPond, 1),
	scaleByMapSize(16, 262), 50
);

log("Creating shrubs...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2), new SimpleObject(aBush4, 1,2, 0,1), new SimpleObject(aBush3, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clPlayer, 0, clPond, 1),
	scaleByMapSize(20, 180), 50
);
RMS.SetProgress(70);

log("Creating gazelles...");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

log("Creating treasures...");
group = new SimpleGroup([new SimpleObject(oFood, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

group = new SimpleGroup([new SimpleObject(oWood, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 2, clDesert, 5, clTreasure, 6, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

log("Creating camels...");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clWater, 1, clFood, 10, clDesert, 5, clTreasure, 2, clPond, 1),
	3*scaleByMapSize(5,20), 50
);

RMS.SetProgress(90);

log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(0.5 * numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 20, clMetal, 6, clDesert, 1, clTreasure, 2, clPond, 1),
		num
	);
}

var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(0.1 * numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 0, clWater, 1, clPlayer, 20, clMetal, 6, clTreasure, 2),
		num
	);
}

log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroupsDeprecated(group, 0,
		borderClasses(clPond, 1, 4),
		num
	);
}

log("Creating obelisks");
group = new SimpleGroup(
	[new SimpleObject(eObelisk, 1,1, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clWater, 4, clForest, 3, clPlayer, 20, clMetal, 6, clRock, 2, clPond, 4, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(5, 30), 50
);

log("Creating pyramids");
group = new SimpleGroup(
	[new SimpleObject(ePyramid, 1,1, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clWater, 7, clForest, 6, clPlayer, 20, clMetal, 5, clRock, 5, clPond, 7, clTreasure, 2), stayClasses(clDesert, 3)],
	scaleByMapSize(2, 6), 50
);

setSkySet("sunny");
setSunColor(0.711, 0.746, 0.574);
setWaterColor(0.541,0.506,0.416);
setWaterTint(0.694,0.592,0.522);
setWaterMurkiness(1);
setWaterWaviness(3.0);
setWaterType("lake");

ExportMap();
