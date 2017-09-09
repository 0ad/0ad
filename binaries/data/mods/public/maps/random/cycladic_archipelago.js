RMS.LoadLibrary("rmgen");

const tOceanDepths = "medit_sea_depths";
const tOceanRockDeep = "medit_sea_coral_deep";
const tOceanRockShallow = "medit_rocks_wet";
const tOceanCoral = "medit_sea_coral_plants";
const tBeachWet = "medit_sand_wet";
const tBeachDry = "medit_sand";
const tBeachGrass = "medit_rocks_grass";
const tBeach = ["medit_rocks_grass","medit_sand", "medit_rocks_grass_shrubs"];
const tBeachBlend = ["medit_rocks_grass", "medit_rocks_grass_shrubs"];
const tBeachCliff = "medit_dirt";
const tCity = "medit_city_tile";
const tGrassDry = ["medit_grass_field_dry", "medit_grass_field_b"];
const tGrass = ["medit_rocks_grass", "medit_rocks_grass","medit_dirt","medit_rocks_grass_shrubs"];
const tGrassLush = ["grass_temperate_dry_tufts", "medit_grass_flowers"];
const tGrassShrubs = "medit_shrubs";
const tCliffShrubs = ["medit_cliff_aegean_shrubs", "medit_cliff_italia_grass","medit_cliff_italia"];
const tGrassRock = ["medit_rocks_grass"];
const tDirt = "medit_dirt";
const tDirtGrass = "medit_dirt_b";
const tDirtCliff = "medit_cliff_italia";
const tGrassCliff = "medit_cliff_italia_grass";
const tCliff = ["medit_cliff_italia", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tForestFloor = "medit_forestfloor_a";

const oBeech = "gaia/flora_tree_euro_beech";
const oBerryBush = "gaia/flora_bush_berry";
const oCarob = "gaia/flora_tree_carob";
const oCypress1 = "gaia/flora_tree_cypress";
const oCypress2 = "gaia/flora_tree_cypress";
const oLombardyPoplar = "gaia/flora_tree_poplar_lombardy";
const oOak = "gaia/flora_tree_oak";
const oPalm = "gaia/flora_tree_medit_fan_palm";
const oPine = "gaia/flora_tree_aleppo_pine";
const oPoplar = "gaia/flora_tree_poplar";
const oDateT = "gaia/flora_tree_cretan_date_palm_tall";
const oDateS = "gaia/flora_tree_cretan_date_palm_short";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oWhale = "gaia/fauna_whale_humpback";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";
const oShipwreck = "other/special_treasure_shipwreck";
const oShipDebris = "other/special_treasure_shipwreck_debris";

const aBushLargeDry = "actor|props/flora/bush_medit_la_dry.xml";
const aBushLarge = "actor|props/flora/bush_medit_la.xml";
const aBushMedDry = "actor|props/flora/bush_medit_me_dry.xml";
const aBushMed = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";
const aBushSmallDry = "actor|props/flora/bush_medit_sm_dry.xml";
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassDry = "actor|props/flora/grass_soft_dry_large_tall.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMed = "actor|geology/stone_granite_med.xml";
const aRockSmall = "actor|geology/stone_granite_small.xml";

// terrain + entity (for painting)
const pPalmForest = [tForestFloor+TERRAIN_SEPARATOR+oPalm, tGrass];
const pPineForest = [tForestFloor+TERRAIN_SEPARATOR+oPine, tGrass];
const pPoplarForest = [tForestFloor+TERRAIN_SEPARATOR+oLombardyPoplar, tGrass];
const pMainForest = [tForestFloor+TERRAIN_SEPARATOR+oCarob, tForestFloor+TERRAIN_SEPARATOR+oBeech, tGrass, tGrass];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clCoral = createTileClass();
var clPlayer = createTileClass();
var clIsland = createTileClass();
var clCity = createTileClass();
var clDirt = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var playerIDs = sortAllPlayers();

//array holding starting islands based on number of players
var startingPlaces=[[0],[0,3],[0,2,4],[0,1,3,4],[0,1,2,3,4],[0,1,2,3,4,5]];

var numIslands = Math.max(6, numPlayers);
var islandX = new Array(numIslands);
var islandZ = new Array(numIslands);
var islandAngle = new Array(numIslands);
//holds all land areas
var areas = [];

var startAngle = randFloat(0, 2 * PI);
for (var i=0; i < numIslands; i++)
{
	islandAngle[i] = startAngle + i*2*PI/numIslands;
	islandX[i] = 0.5 + 0.39*cos(islandAngle[i]);
	islandZ[i] = 0.5 + 0.39*sin(islandAngle[i]);
}

for (var i = 0; i < numIslands; ++i)
{
	var radius = scaleByMapSize(15,40);
	var coral=scaleByMapSize(1,5);
	var wet = 3;
	var dry = 1;
	var gbeach = 2;
	var elevation = 3;

	// get the x and z in tiles
	var fx = fractionToTiles(islandX[i]);
	var fz = fractionToTiles(islandZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	var islandSize = PI*radius*radius;
	var islandBottom=PI*(radius+coral)*(radius+coral);

	//create base
	var placer = new ClumpPlacer(islandBottom, .7, .1, 10, ix, iz);
	var terrainPainter = new LayeredPainter([tOceanRockDeep, tOceanCoral], [5]);
	createArea(placer, [terrainPainter, paintClass(clCoral)],avoidClasses(clCoral,0));
}

//create spoke islands
//put down base resources and animals but do not populate
for (var i=0; i < numIslands; i++)
{
	log("Creating base Island " + (i + 1) + "...");

	var radius = scaleByMapSize(15,40);
	var coral=scaleByMapSize(2,5);
	var wet = 3;
	var dry = 1;
	var gbeach = 2;
	var elevation = 3;

	// get the x and z in tiles
	var fx = fractionToTiles(islandX[i]);
	var fz = fractionToTiles(islandZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	var islandSize = PI*radius*radius;
	var islandBottom=PI*(radius+coral)*(radius+coral);

	// create island
	var placer = new ClumpPlacer(islandSize, .7, .1, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tOceanCoral,tBeachWet, tBeachDry, tBeach, tBeachBlend, tGrass],
		[1, wet, dry, 1, gbeach]
	);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, elevation, 5);
	var temp = createArea(placer, [terrainPainter, paintClass(clPlayer), elevationPainter],avoidClasses(clPlayer,0));
	areas.push(temp);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 10;
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

	// create starting trees
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oPalm, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}
RMS.SetProgress(15);

log("Populating islands ...");
//nPlayer is the player we are on i is the island we are on
var nPlayer = 0;
for (let i = 0; i < numIslands; ++i)
	if (numPlayers >= 6 || i == startingPlaces[numPlayers-1][nPlayer])
	{
		var id = playerIDs[nPlayer];

		// Get the x and z in tiles
		var fx = fractionToTiles(islandX[i]);
		var fz = fractionToTiles(islandZ[i]);
		var ix = round(fx);
		var iz = round(fz);

		// Create city patch
		var cityRadius = 6;
		var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
		var painter = new LayeredPainter([tGrass, tCity], [1]);
		createArea(placer, [painter,paintClass(clCity)], null);

		placeCivDefaultEntities(fx, fz, id, { 'iberWall': 'towers' });

		++nPlayer;
	}
RMS.SetProgress(20);

// get the x and z in tiles
var nCenter = floor(scaleByMapSize(1,4));
startAngle = randFloat(0, 2 * PI);
for (var i = 0; i < nCenter; ++i)
{
	var fx = 0.5;
	var fz = 0.5;

	if (nCenter != 1)
	{
		let isangle = startAngle + i * 2 * PI / nCenter + randFloat(-PI/8, PI/8);
		let dRadius = randFloat(0.1, 0.16);
		fx = 0.5 + dRadius * cos(isangle);
		fz = 0.5 + dRadius * sin(isangle);
	}

	var ix = round(fractionToTiles(fx));
	var iz = round(fractionToTiles(fz));

	var radius = scaleByMapSize(15,30);
	var coral= 2;
	var wet = 3;
	var dry = 1;
	var gbeach = 2;
	var elevation = 3;

	var islandSize = PI*radius*radius;
	var islandBottom=PI*(radius+coral)*(radius+coral);

	// Create base
	var placer = new ClumpPlacer(islandBottom, .7, .1, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tOceanRockDeep, tOceanCoral],
		[5]
	);
	createArea(placer, [terrainPainter, paintClass(clCoral)],avoidClasses(clCoral,0,clPlayer,0));

	// Create island
	var placer = new ClumpPlacer(islandSize, .7, .1, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tOceanCoral,tBeachWet, tBeachDry, tBeach, tBeachBlend, tGrass],
		[1, wet, dry, 1, gbeach]
	);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, elevation, 5);
	var temp = createArea(placer, [terrainPainter, paintClass(clIsland), elevationPainter],avoidClasses(clPlayer,0));

	areas.push(temp);
}
RMS.SetProgress(30);

log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 60), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 3);
createAreasInAreas(
	placer,
	painter,
	avoidClasses(clCity, 0),
	scaleByMapSize(25, 75),15,
	areas
);
RMS.SetProgress(34);

log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, tCliffShrubs],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 12, 2);
createAreasInAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)],
	avoidClasses(clCity, 15, clHill, 15),
	scaleByMapSize(5, 30), 15,
	areas
);
RMS.SetProgress(38);

// Find all water
for (var ix = 0; ix < mapSize; ix++)
	for (var iz = 0; iz < mapSize; iz++)
		if (getHeight(ix,iz) < 0)
			addToClass(ix,iz,clWater);

log("Creating forests...");
var types = [
	[[tForestFloor, tGrass, pPalmForest], [tForestFloor, pPalmForest]],
	[[tForestFloor, tGrass, pPineForest], [tForestFloor, pPineForest]],
	[[tForestFloor, tGrass, pPoplarForest], [tForestFloor, pPoplarForest]],
	[[tForestFloor, tGrass, pMainForest], [tForestFloor, pMainForest]]
];	// some variation
var size = 5; //size
var num = scaleByMapSize(10, 64); //number
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(randIntInclusive(6, 17), 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreasInAreas(
		placer,
		[painter, paintClass(clForest)],
		avoidClasses(clCity, 1, clWater, 3, clForest, 3, clHill, 1),
		num, 20, areas
	);
}
RMS.SetProgress(42);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 5, clRock, 6)],
	scaleByMapSize(4,16), 200, areas
);
RMS.SetProgress(46);

log("Creating small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 5, clRock, 2)],
	scaleByMapSize(4,16), 200, areas
);
RMS.SetProgress(50);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 5, clMetal, 6, clRock, 6)],
	scaleByMapSize(4,16), 200, areas
);
RMS.SetProgress(54);

log("Creating shrub patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter([tBeachBlend,tGrassShrubs],[1]);
	createAreasInAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clHill, 0, clDirt, 6, clCity, 0),
		scaleByMapSize(4, 16), 20, areas
	);
}
RMS.SetProgress(58);

log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter([tGrassDry],[]);
	createAreasInAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clHill, 0, clDirt, 6, clCity, 0),
		scaleByMapSize(4, 16), 20, areas
	);
}
RMS.SetProgress(62);

log("Creating straggler trees...");
for (let tree of [oCarob, oBeech, oLombardyPoplar, oLombardyPoplar, oPine])
	createObjectGroupsByAreasDeprecated(
		new SimpleGroup([new SimpleObject(tree, 1,1, 0,1)], true, clForest),
		0,
		avoidClasses(clWater, 2, clForest, 2, clCity, 3, clBaseResource, 1, clRock, 6, clMetal, 6, clPlayer, 1, clHill, 1),
		scaleByMapSize(2, 38), 50, areas
	);
RMS.SetProgress(66);

log("Create straggler cypresses...");
group = new SimpleGroup(
	[new SimpleObject(oCypress2, 1,3, 0,3), new SimpleObject(oCypress1, 0,2, 0,2)],
	true
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clWater, 2, clForest, 2, clCity, 3, clBaseResource, 1, clRock, 6, clMetal, 6, clPlayer, 1, clHill, 1),
	scaleByMapSize(5, 75), 50, areas
);
RMS.SetProgress(70);

log("Create straggler date palms...");
group = new SimpleGroup(
	[new SimpleObject(oDateS, 1,3, 0,3), new SimpleObject(oDateT, 0,2, 0,2)],
	true
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clWater, 2, clForest, 1, clCity, 0, clBaseResource, 1, clRock, 6, clMetal, 6, clPlayer, 1, clHill, 1),
	scaleByMapSize(5, 75), 50, areas
);
RMS.SetProgress(74);

log("Creating rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockSmall, 0,3, 0,2), new SimpleObject(aRockMed, 0,2, 0,2),
	new SimpleObject(aRockLarge, 0,1, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clCity, 0),
	scaleByMapSize(30, 180), 50
);
RMS.SetProgress(78);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clHill, 1, clCity, 10, clMetal, 6, clRock, 2, clFood, 8),
	3 * numPlayers, 50
);
RMS.SetProgress(82);

log("Creating berry bushes...");
group = new SimpleGroup([new SimpleObject(oBerryBush, 5,7, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clForest, 1, clHill, 1, clCity, 10, clMetal, 6, clRock, 2, clFood, 8),
	1.5 * numPlayers, 100
);
RMS.SetProgress(86);

log("Creating Fish...");
group = new SimpleGroup([new SimpleObject(oFish, 1,1, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8)],
	scaleByMapSize(40,200), 100
);
RMS.SetProgress(90);

log("Creating Whales...");
group = new SimpleGroup([new SimpleObject(oWhale, 1,1, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8, clPlayer,4,clIsland,4)],
	scaleByMapSize(10,40), 100
);
RMS.SetProgress(94);

log("Creating shipwrecks...");
group = new SimpleGroup([new SimpleObject(oShipwreck, 1,1, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8)],
	scaleByMapSize(6,16), 100
);
RMS.SetProgress(98);

log("Creating shipwreck debris...");
group = new SimpleGroup([new SimpleObject(oShipDebris, 1,2, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8)],
	scaleByMapSize(10,20), 100
);
RMS.SetProgress(99);

setSkySet("sunny");
setWaterColor(0.2,0.294,0.49);
setWaterTint(0.208, 0.659, 0.925);
setWaterMurkiness(0.72);
setWaterWaviness(3.0);
setWaterType("ocean");

ExportMap();
