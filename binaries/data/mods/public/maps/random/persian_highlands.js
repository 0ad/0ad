Engine.LoadLibrary("rmgen");

const tCity = "desert_city_tile_pers_dirt";

if (randBool()) // summer
{
	var tDirtMain = ["desert_dirt_persia_1", "desert_dirt_persia_2", "grass_field_dry"];
	var tLakebed1 = ["desert_lakebed_dry_b", "desert_lakebed_dry"];
	var tLakebed2 = ["desert_lakebed_dry_b", "desert_lakebed_dry", "desert_shore_stones", "desert_shore_stones"];
	var tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_crumbling"];
	var tForestFloor = "medit_grass_field_dry";
	var tRocky = "desert_dirt_persia_rocky";
	var tRocks = "desert_dirt_persia_rocks";
	var tGrass = "grass_field_dry";
}
else //spring
{
	var tDirtMain = ["desert_grass_a", "desert_grass_a", "desert_grass_a", "desert_plants_a"];
	var tLakebed1 = ["desert_lakebed_dry_b", "desert_lakebed_dry"];
	var tLakebed2 = "desert_grass_a_sand";
	var tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_crumbling"];
	var tForestFloor = "desert_plants_b_persia";
	var tRocky = "desert_plants_b_persia";
	var tRocks = "desert_plants_a";
	var tGrass = "desert_dirt_persia_rocky";

	setTerrainAmbientColor(0.329412, 0.419608, 0.501961);
}

const oGrapesBush = "gaia/flora_bush_grapes";
const oCamel = "gaia/fauna_camel";
const oSheep = "gaia/fauna_sheep";
const oGoat = "gaia/fauna_goat";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oOak = "gaia/flora_tree_oak";

const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_dry_a.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

// terrain + entity (for painting)
const pForestO = [tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor, tDirtMain, tDirtMain];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clPatch = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clCP = createTileClass();

initTerrain(tDirtMain);

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

	// create the city patch
	var cityRadius = 10;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tCity, tCity], [3]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oGrapesBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while (Math.abs(mAngle - bbAngle) < Math.PI / 3)
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
		[new SimpleObject(oOak, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

Engine.SetProgress(10);

log("Creating rock patches...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(3, 6)), Math.floor(scaleByMapSize(20, 45)), 0),
	[
		new TerrainPainter(tRocky),
		paintClass(clPatch)
	],
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(5, 20));
Engine.SetProgress(15);

log("Creating secondary rock patches...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), Math.floor(scaleByMapSize(15, 40)), 0),
	[
		new TerrainPainter([tRocky, tRocks]),
		paintClass(clPatch)
	],
	avoidClasses(clPatch, 2, clPlayer, 4),
	scaleByMapSize(15, 50));
Engine.SetProgress(20);

log("Creating dirt patches...");
createAreas(
	new ChainPlacer(
		1,
		Math.floor(scaleByMapSize(3, 5)),
		Math.floor(scaleByMapSize(15, 40)),
		0),
	[
		new TerrainPainter([tGrass]),
		paintClass(clPatch)
	],
	avoidClasses(clPatch, 2, clPlayer, 4),
	scaleByMapSize(15, 50));
Engine.SetProgress(25);

log("Creating centeral plateau...");
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(5, 13)),
		Math.floor(scaleByMapSize(35, 200)),
		1,
		mapSize / 2,
		mapSize / 2,
		0,
		[Math.floor(scaleByMapSize(18, 68))]),
	[
		new LayeredPainter([tLakebed2, tLakebed1], [6]),
		new SmoothElevationPainter(ELEVATION_MODIFY, -10, 8),
		paintClass(clCP)
	],
	avoidClasses(clPlayer, 18));
Engine.SetProgress(30);

log("Creating hills...");
for (let i = 0; i < scaleByMapSize(20, 80); ++i)
	createMountain(
		Math.floor(scaleByMapSize(40, 60)),
		Math.floor(scaleByMapSize(3, 4)),
		Math.floor(scaleByMapSize(6, 12)),
		Math.floor(scaleByMapSize(4, 10)),
		avoidClasses(clPlayer, 7, clCP, 5, clHill, Math.floor(scaleByMapSize(18, 25))),
		randIntExclusive(0, mapSize),
		randIntExclusive(0, mapSize),
		tCliff,
		clHill,
		14);
Engine.SetProgress(35);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.7);
var types = [
	[[tDirtMain, tForestFloor, pForestO], [tForestFloor, pForestO]],
	[[tDirtMain, tForestFloor, pForestO], [tForestFloor, pForestO]]
];
var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(
			Math.floor(scaleByMapSize(1, 2)),
			Math.floor(scaleByMapSize(2, 5)),
			Math.floor(size / Math.floor(scaleByMapSize(8, 3))),
			1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(
			clPlayer, 6,
			clForest, 10,
			clHill, 1,
			clCP, 1),
		num);
Engine.SetProgress(50);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4), new RandomObject(aBushes, 2, 4, 0, 2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clCP, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clCP, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1, clCP, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating centeral stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clCP, 6),
	5*scaleByMapSize(5,30), 50
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clCP, 6),
	5*scaleByMapSize(5,30), 50
);

log("Creating centeral metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clCP, 6),
	5*scaleByMapSize(5,30), 50
);

Engine.SetProgress(60);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

Engine.SetProgress(65);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

log("Creating goat...");
group = new SimpleGroup(
	[new SimpleObject(oGoat, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clCP, 2),
	3 * numPlayers, 50
);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clCP, 2),
	3 * numPlayers, 50
);

log("Creating grape bush...");
group = new SimpleGroup(
	[new SimpleObject(oGrapesBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10, clCP, 2),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

log("Creating camels...");
group = new SimpleGroup(
	[new SimpleObject(oCamel, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clCP, 2),
	3 * numPlayers, 50
);

Engine.SetProgress(90);

createStragglerTrees(
	[oOak],
	avoidClasses(clForest, 1, clHill, 1, clPlayer, 1, clBaseResource, 6, clMetal, 6, clRock, 6, clCP, 2),
	clForest,
	stragglerTrees);

setSunColor(1.0, 0.796, 0.374);
setSunElevation(PI / 6);
setSunRotation(-1.86532);

setFogFactor(0.2);
setFogThickness(0.0);
setFogColor(0.852, 0.746, 0.493);

setPPEffect("hdr");
setPPContrast(0.75);
setPPSaturation(0.45);
setPPBloom(0.3);

ExportMap();
