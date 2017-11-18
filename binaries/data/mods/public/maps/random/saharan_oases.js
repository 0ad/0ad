RMS.LoadLibrary("rmgen");

const tCity = "desert_city_tile";
const tCityPlaza = "desert_city_tile_plaza";
const tFineSand = "desert_sand_smooth";
const tDirt1 = "desert_dirt_rough_2";
const tSandDunes = "desert_sand_dunes_50";
const tDirt2 = "desert_dirt_rough";
const tDirtCracks = "desert_dirt_cracks";
const tShore = "desert_shore_stones";
const tWaterDeep = "desert_shore_stones_wet";
const tLush = "desert_grass_a";
const tSLush = "desert_grass_a_sand";

const oGrapeBush = "gaia/flora_bush_grapes";
const oCamel = "gaia/fauna_camel";
const oGazelle = "gaia/fauna_gazelle";
const oGoat = "gaia/fauna_goat";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oDatePalm = "gaia/flora_tree_date_palm";
const oSDatePalm = "gaia/flora_tree_cretan_date_palm_short";
const oWoodTreasure = "gaia/special_treasure_wood";
const oFoodTreasure = "gaia/special_treasure_food_bin";

const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_medit_sm_dry.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

// terrain + entity (for painting)
const pForest = [tLush + TERRAIN_SEPARATOR + oDatePalm, tLush + TERRAIN_SEPARATOR + oSDatePalm, tLush];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clGrass = createTileClass();
var clPond = createTileClass();
var clTreasure = createTileClass();

var [playerIDs, playerX, playerZ, playerAngle] = radialPlayerPlacement();

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
	var mDist = radius - 5;
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
	var num = floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = 12;
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oSDatePalm, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aBush1, clBaseResource, radius);
}

RMS.SetProgress(30);

log("Creating oases...");
for (let i = 0; i < numPlayers; ++i)
	createArea(
		new ClumpPlacer(diskArea(scaleByMapSize(16, 60)) * 0.185,
			0.6,
			0.15,
			0,
			mapSize * (0.5 + 0.18 * Math.cos(playerAngle[i]) + scaleByMapSize(1, 4) * Math.cos(playerAngle[i]) / 100),
			mapSize * (0.5 + 0.18 * Math.sin(playerAngle[i]) + scaleByMapSize(1, 4) * Math.sin(playerAngle[i]) / 100)),
		[
			new LayeredPainter(
				[tSLush ,[tLush, pForest], [tLush, pForest], tShore, tShore, tWaterDeep],
				[2, 2, 1, 3, 1]),
			new SmoothElevationPainter(ELEVATION_MODIFY, -3, 10),
			paintClass(clWater)
		],
		null);
RMS.SetProgress(50);

log("Creating grass patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tDirt1, tSandDunes], [tSandDunes, tDirt2], [tDirt2, tDirt1]],
				[1, 1]
			),
			paintClass(clDirt)
		],
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 0, clWater, 1, clDirt, 5),
		scaleByMapSize(15, 45));
RMS.SetProgress(55);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tDirt2, tDirtCracks], [tDirt2, tFineSand], [tDirtCracks, tFineSand]],
				[1, 1]
			),
			paintClass(clDirt)
		],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 0, clWater, 1, clGrass, 5),
		scaleByMapSize(15, 45));
RMS.SetProgress(60);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 26, clRock, 10, clWater, 1),
	2*scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 26, clRock, 10, clWater, 1),
	2*scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 26, clMetal, 10, clRock, 5, clWater, 1),
	2*scaleByMapSize(4,16), 100
);

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
	scaleByMapSize(10, 100), 50
);

log("Creating small decorative rocks on mines...");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	stayClasses(clRock, 0),
	5*scaleByMapSize(16, 262), 50
);

group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	stayClasses(clMetal, 0),
	5*scaleByMapSize(16, 262), 50
);

log("Creating gazelles...");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	6*scaleByMapSize(5,20), 50
);

log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	5*scaleByMapSize(5,20), 50
);

log("Creating treasures...");
group = new SimpleGroup([new SimpleObject(oFoodTreasure, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	3*scaleByMapSize(5,20), 50
);

group = new SimpleGroup([new SimpleObject(oWoodTreasure, 1,1, 0,2)], true, clTreasure);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 8, 5),
	3*scaleByMapSize(5,20), 50
);

log("Creating camels...");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	borderClasses(clWater, 14, 5),
	5*scaleByMapSize(5,20), 50
);

setSkySet("sunny");
setSunColor(0.746, 0.718, 0.539);
setWaterColor(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1.0);
setWaterType("clap");
setWaterMurkiness(0.5);

ExportMap();
