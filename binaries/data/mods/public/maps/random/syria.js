RMS.LoadLibrary("rmgen");

//terrain textures
const tMainDirt = ["desert_dirt_rocks_1", "desert_dirt_cracks"];
const tForestFloor1 = "forestfloor_dirty";
const tForestFloor2 = "desert_forestfloor_palms";
const tGrassSands = "desert_grass_a_sand";
const tGrass = "desert_grass_a";
const tSecondaryDirt = "medit_dirt_dry";
const tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_2"];
const tHill = ["desert_dirt_rocks_1", "desert_dirt_rocks_2", "desert_dirt_rocks_3"];
const tDirt = ["desert_dirt_rough", "desert_dirt_rough_2"];
const tRoad = "desert_shore_stones";;
const tRoadWild = "desert_grass_a_stones";;

const oTamarix = "gaia/flora_tree_tamarix";
const oPalm = "gaia/flora_tree_date_palm";
const oPine = "gaia/flora_tree_aleppo_pine";
const oBush = "gaia/flora_bush_grapes";
const oCamel = "gaia/fauna_camel";
const oGazelle = "gaia/fauna_gazelle";
const oLion = "gaia/fauna_lion";
const oStoneLarge = "gaia/geology_stonemine_desert_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";

const aRock = "actor|geology/stone_desert_med.xml";
const aBushA = "actor|props/flora/bush_desert_dry_a.xml";
const aBushB = "actor|props/flora/bush_desert_dry_a.xml";
const aBushes = [aBushA, aBushB];

const pForestP = [tForestFloor2 + TERRAIN_SEPARATOR + oPalm, tForestFloor2];
const pForestT = [tForestFloor1 + TERRAIN_SEPARATOR + oTamarix,tForestFloor2];

InitMap();

const numPlayers = getNumPlayers();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clGrass = createTileClass();

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

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
	var grassRadius = floor(scaleByMapSize(16 ,30));
	placer = new ChainPlacer(2, floor(scaleByMapSize(5, 12)), floor(scaleByMapSize(25, 60)), 1, ix, iz, 0, [grassRadius]);
	var painter = new LayeredPainter([tGrassSands, tGrass], [3]);
	createArea(placer, [painter, paintClass(clGrass)], null);

	// create the city patch
	var cityRadius = 10;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [3]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
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
		[new SimpleObject(pickRandom([oPalm, oTamarix]), num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

RMS.SetProgress(10);

log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter,
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800)
);

log("Creating hills...");
placer = new ChainPlacer(1, floor(scaleByMapSize(4, 6)), floor(scaleByMapSize(16, 40)), 0.5);
var terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 22, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)],
	avoidClasses(clPlayer, 3, clGrass, 1, clHill, 10),
	scaleByMapSize(1, 3) * numPlayers * 3
);

RMS.SetProgress(25);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 400;
const MAX_TREES = 2000;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

log("Creating forests...");
var types = [
	[[tMainDirt, tForestFloor2, pForestP], [tForestFloor2, pForestP]],
	[[tMainDirt, tForestFloor1, pForestT], [tForestFloor1, pForestT]]
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
		avoidClasses(clPlayer, 1, clGrass, 1, clForest, 10, clHill, 1),
		num
	);
}

RMS.SetProgress(40);

log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[tSecondaryDirt ,tDirt], 		// terrains
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

log("Creating big patches...");
var sizes = [scaleByMapSize(6, 30), scaleByMapSize(10, 50), scaleByMapSize(16, 70)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[tSecondaryDirt ,tDirt], 		// terrains
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
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clGrass, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clGrass, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1, clGrass, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushB, 1,2, 0,1), new SimpleObject(aBushA, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(50, 500), 50
);
RMS.SetProgress(80);

log("Creating gazelle...");
group = new SimpleGroup(
	[new SimpleObject(oGazelle, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clGrass, 2),
	3 * numPlayers, 50
);

log("Creating lions...");
group = new SimpleGroup(
	[new SimpleObject(oLion, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clGrass, 2),
	3 * numPlayers, 50
);

log("Creating camels...");
group = new SimpleGroup(
	[new SimpleObject(oCamel, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clGrass, 2),
	3 * numPlayers, 50
);
RMS.SetProgress(85);

log("Creating straggler trees...");
var types = [oPalm, oTamarix, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 1, clHill, 1, clPlayer, 1, clMetal, 6, clRock, 6),
		num
	);
}

log("Creating straggler trees...");
var types = [oPalm, oTamarix, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroupsDeprecated(group, 0,
		[avoidClasses(clForest, 1, clHill, 1, clPlayer, 1, clMetal, 6, clRock, 6), stayClasses(clGrass, 3)],
		num
	);
}

setSkySet("sunny");
setSunElevation(PI / 8);
setSunRotation(randFloat(0, TWO_PI));
setSunColor(0.746, 0.718, 0.539);
setWaterColor(0.292, 0.347, 0.691);
setWaterTint(0.550, 0.543, 0.437);
setWaterMurkiness(0.83);

setFogColor(0.8, 0.76, 0.61);
setFogThickness(0.2);
setFogFactor(0.4);

setPPEffect("hdr");
setPPContrast(0.65);
setPPSaturation(0.42);
setPPBloom(0.6);

ExportMap();
