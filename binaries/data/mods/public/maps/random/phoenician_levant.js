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

const oGrapeBush = "gaia/flora_bush_grapes";
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

const aBush1 = "actor|props/flora/bush_medit_sm.xml";
const aBush2 = "actor|props/flora/bush_medit_me.xml";
const aBush3 = "actor|props/flora/bush_medit_la.xml";
const aBush4 = "actor|props/flora/bush_medit_me.xml";
const aDecorativeRock = "actor|geology/stone_granite_med.xml";

// terrain + entity (for painting)
const pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor + TERRAIN_SEPARATOR + oCarob, tForestFloor, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();

var clPlayer = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clGrass = createTileClass();
var clHill = createTileClass();
var clIsland = createTileClass();

var playerIDs = sortAllPlayers();

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	var fx = fractionToTiles(0.66 + 0.2 * (i % 2));
	var fz = fractionToTiles((i + 1) / (numPlayers + 1));
	var ix = floor(fx);
	var iz = floor(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

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
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oCarob, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aBush1, clBaseResource, radius);
}

RMS.SetProgress(30);

paintRiver({
	"parallel": true,
	"startX": 0,
	"startZ": 0,
	"endX": 0,
	"endZ": 1,
	"width": 1,
	"fadeDist": 0.05,
	"deviation": 0,
	"waterHeight": -3,
	"landHeight": 1,
	"meanderShort": 20,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height, riverFraction) => {

		if (height < 0)
			addToClass(ix, iz, clWater);

		if (height < -1.5)
			placeTerrain(ix, iz, tWater);
		else
			placeTerrain(ix, iz, tShore);
	}
});

RMS.SetProgress(40);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clWater, 2, clPlayer, 20),
	scaleByMapSize(100, 200));

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, 15, 2),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 20, clForest, 1, clHill, 15, clWater, 0),
	scaleByMapSize(1, 4) * numPlayers * 3);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.5);
var num = scaleByMapSize(10,42);
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 5))), 0.5),
	[
		new TerrainPainter([tForestFloor, pForest]),
		paintClass(clForest)
	],
	avoidClasses(clPlayer, 20, clForest, 10, clWater, 1, clHill, 1, clBaseResource, 3),
	num,
	50);
RMS.SetProgress(50);

log("Creating grass patches...");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter(
				[[tGrass, tRocksShrubs], [tRocksShrubs, tRocksGrass], [tRocksGrass, tGrass]],
				[1, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clForest, 0, clGrass, 5, clPlayer, 10, clWater, 4, clDirt, 5, clHill, 1),
		scaleByMapSize(15, 45));
RMS.SetProgress(55);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter(
				[[tDirt, tDirtB], [tDirt, tMainDirt], [tDirtB, tMainDirt]],
				[1, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clForest, 0, clDirt, 5, clPlayer, 10, clWater, 4, clGrass, 5, clHill, 1),
		scaleByMapSize(15, 45));
RMS.SetProgress(60);

log("Creating cyprus...");
createAreas(
	new ClumpPlacer(4.5 * scaleByMapSize(60, 540), 0.2, 0.1, 0.01),
	[
		new LayeredPainter([tShore, tHill], [12]),
		new SmoothElevationPainter(ELEVATION_SET, 6, 8),
		paintClass(clIsland)
	],
	[stayClasses (clWater, 5)],
	1);

log("Creating cyprus stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating cyprus small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating cyprus metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clIsland, 9),
	14 * scaleByMapSize(4,16), 100
);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clWater, 3, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 1),
	scaleByMapSize(16, 262), 50
);

log("Creating shrubs...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2), new SimpleObject(aBush4, 1,2, 0,1), new SimpleObject(aBush3, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 3, clPlayer, 0, clHill, 1),
	scaleByMapSize(40, 360), 50
);
RMS.SetProgress(70);

log("Creating fish...");
group = new SimpleGroup([new SimpleObject(oFish, 1,3, 2,6)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clIsland, 2, clFood, 10), stayClasses(clWater, 5)],
	3*scaleByMapSize(5,20), 50
);

log("Creating sheeps...");
group = new SimpleGroup([new SimpleObject(oSheep, 5,7, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

log("Creating deers...");
group = new SimpleGroup([new SimpleObject(oDeer, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 7, clWater, 3, clFood, 10, clHill, 1),
	scaleByMapSize(5,20), 50
);

log("Creating grape bushes...");
group = new SimpleGroup(
	[new SimpleObject(oGrapeBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 15, clHill, 1, clFood, 7),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);
RMS.SetProgress(90);

var stragglerTreeConfig = [
	[1, avoidClasses(clForest, 0, clWater, 1, clPlayer, 8, clMetal, 6, clHill, 1)],
	[3, stayClasses(clIsland, 9)]
];
for (let [amount, constraint] of stragglerTreeConfig)
	createStragglerTrees(
		[oDatePalm, oSDatePalm, oCarob, oFanPalm, oPoplar, oCypress],
		constraint,
		clForest,
		amount * stragglerTrees);

setSkySet("sunny");
setSunColor(0.917, 0.828, 0.734);
setWaterColor(0.263,0.314,0.631);
setWaterTint(0.133, 0.725,0.855);
setWaterWaviness(2.0);
setWaterType("ocean");
setWaterMurkiness(0.8);

setTerrainAmbientColor(0.57, 0.58, 0.55);
setUnitsAmbientColor(0.447059, 0.509804, 0.54902);

setSunElevation(0.671884);
setSunRotation(-0.582913);

setFogFactor(0.2);
setFogThickness(0.15);
setFogColor(0.8, 0.7, 0.6);

setPPEffect("hdr");
setPPContrast(0.53);
setPPSaturation(0.47);
setPPBloom(0.52);

ExportMap();
