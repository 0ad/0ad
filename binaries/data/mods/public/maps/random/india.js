RMS.LoadLibrary("rmgen");

const tGrass1 = "savanna_grass_a";
const tDirt1 = "savanna_dirt_a";
const tDirt4 = "savanna_dirt_b";
const tCityTiles = "savanna_tile_a_dirt_red";
const tShore = "savanna_riparian_bank";
const tWater = "savanna_riparian_wet";

const oTree = "gaia/flora_tree_palm_tropic";
const oBerryBush = "gaia/flora_bush_berry";
const oRabbit = "gaia/fauna_rabbit";
const oTiger = "gaia/fauna_tiger";
const oCrocodile = "gaia/fauna_crocodile";
const oFish = "gaia/fauna_fish";
const oElephant = "gaia/fauna_elephant_asian";
const oElephantInfant = "gaia/fauna_elephant_asian_infant";
const oBoar = "gaia/fauna_boar";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

const aBush = "actor|props/flora/bush_medit_sm_dry.xml";
const aRock = "actor|geology/stone_savanna_med.xml";

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

var clPlayer = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var playerIDs = sortAllPlayers();

var playerX = [];
var playerZ = [];
var startAngle = randFloat(0, 2 * PI);

for (let i = 0; i < numPlayers; ++i)
{
	let playerAngle = startAngle + i * 2 * PI / numPlayers;
	playerX[i] = 0.5 + 0.35 * Math.cos(playerAngle);
	playerZ[i] = 0.5 + 0.35 * Math.sin(playerAngle);
}

for (let i = 0; i < numPlayers; ++i)
{
	let id = playerIDs[i];
	log("Creating base for player " + id + "...");

	let fx = fractionToTiles(playerX[i]);
	let fz = fractionToTiles(playerZ[i]);
	let ix = Math.round(fx);
	let iz = Math.round(fz);

	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);

	// Create the city patch
	let radius = scaleByMapSize(15, 25);
	let cityRadius = radius / 3;
	createArea(
		new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz),
		new LayeredPainter([tDirt1, tCityTiles], [1]),
		null);

	placeCivDefaultEntities(fx, fz, id);
	placeDefaultChicken(fx, fz, clBaseResource);

	// Create berry bushes
	var bbAngle = randFloat(0, 2 * PI);
	var bbDist = 12;
	var bbX = Math.round(fx + bbDist * Math.cos(bbAngle));
	var bbZ = Math.round(fz + bbDist * Math.sin(bbAngle));
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oBerryBush, 5,5, 0,3)],
			true, clBaseResource, bbX, bbZ
		),
		0);

	// Create metal mine
	var mAngle = bbAngle;
	while (Math.abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, 2 * PI);
	var mDist = 13;
	var mX = Math.round(fx + mDist * Math.cos(mAngle));
	var mZ = Math.round(fz + mDist * Math.sin(mAngle));
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
			true, clBaseResource, mX, mZ
		),
		0);

	// Create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = Math.round(fx + mDist * Math.cos(mAngle));
	mZ = Math.round(fz + mDist * Math.sin(mAngle));
	createStoneMineFormation(mX, mZ, tDirt4);
	addToClass(mX, mZ, clPlayer);

	// Create starting trees
	var num = Math.floor(PI * radius * radius / 300);
	var tAngle = randFloat(-PI/3, 4 * PI/3);
	var tDist = randFloat(13, 15);
	var tX = Math.round(fx + tDist * Math.cos(tAngle));
	var tZ = Math.round(fz + tDist * Math.sin(tAngle));
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oTree, num, num, 4, 6)],
			false, clBaseResource, tX, tZ
		),
		0,
		avoidClasses(clBaseResource,2));
}
RMS.SetProgress(20);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.5, 0.08, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800)
);

log("Creating the half dried-up lake...");
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(2, 16)),
		Math.floor(scaleByMapSize(35, 200)),
		1,
		Math.round(fractionToTiles(0.5)),
		Math.round(fractionToTiles(0.5)),
		0,
		[Math.floor(mapSize * 0.008 * Math.pow(scaleByMapSize(1, 66), 1/8))]),
	[
		new SmoothElevationPainter(ELEVATION_SET, -3, 4),
		paintClass(clWater)
	],
	avoidClasses(clPlayer, 20));

log("Creating more shore jaggedness...");
createAreas(
	new ChainPlacer(2, Math.floor(scaleByMapSize(4, 6)), 3, 1),
	[
		new SmoothElevationPainter(ELEVATION_SET, 3, 4),
		unPaintClass(clWater)
	],
	borderClasses(clWater, 4, 7),
	scaleByMapSize(12, 130) * 2, 150
);

paintTerrainBasedOnHeight(2.4, 3.4, 3, tGrass1);
paintTerrainBasedOnHeight(1, 2.4, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
paintTileClassBasedOnHeight(-6, 0, 1, clWater);
RMS.SetProgress(55);

var playerConstraint = new AvoidTileClassConstraint(clPlayer, 30);
var minesConstraint = new AvoidTileClassConstraint(clRock, 25);
var waterConstraint = new AvoidTileClassConstraint(clWater, 10);

log("Creating stone mines...");
for (let i = 0; i < scaleByMapSize(12, 30); ++i)
{
	let mX = randIntInclusive(1, mapSize - 1);
	let mZ = randIntInclusive(1, mapSize - 1);
	if (playerConstraint.allows(mX, mZ) && minesConstraint.allows(mX, mZ) && waterConstraint.allows(mX, mZ))
	{
		createStoneMineFormation(mX, mZ, tDirt4);
		addToClass(mX, mZ, clRock);
	}
}

log("Creating metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	avoidClasses(clPlayer, 20, clMetal, 10, clRock, 8, clWater, 4),
	scaleByMapSize(2, 12), 100
);
RMS.SetProgress(65);

log("Creating small decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(aRock, 1, 3, 0, 3)],
		true
	),
	0,
	avoidClasses(clPlayer, 7, clWater, 1),
	scaleByMapSize(200, 1200), 1
);
RMS.SetProgress(70);

log("Creating boar...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBoar, 1, 2, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

log("Creating tigers...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oTiger, 2, 2, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

log("Creating crocodiles...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oCrocodile, 2, 4, 0, 4)],
		true, clFood
	), 0,
	stayClasses(clWater, 1),
	scaleByMapSize(4, 12), 50
);

log("Creating elephants...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oElephant, 2, 4, 0, 4),
			new SimpleObject(oElephantInfant, 1, 2, 0, 4)
		],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

log("Creating rabbits...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oRabbit, 5, 6, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		25 * numPlayers
	],
	[avoidClasses(clFood, 20), stayClasses(clWater, 2)]
);

log("Creating berry bush...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 3, clPlayer, 20, clFood, 12, clRock, 7, clMetal, 2),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);
RMS.SetProgress(85);

log("Creating trees...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oTree, 1, 7, 0, 3)],
		true, clForest
	),
	0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 1, clRock, 7, clWater, 1),
	scaleByMapSize(70, 500)
);

log("Creating large grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(aBush, 2, 4, 0, 1.8, -PI/8, PI/8)]
	),
	0,
	avoidClasses(clWater, 3, clPlayer, 2, clForest, 0),
	scaleByMapSize(100, 1200)
);

setSunColor(0.87451, 0.847059, 0.647059);
setWaterColor(0.741176, 0.592157, 0.27451);
setWaterTint(0.741176, 0.592157, 0.27451);
setWaterWaviness(2.0);
setWaterType("clap");
setWaterMurkiness(0.835938);

setUnitsAmbientColor(0.57, 0.58, 0.55);
setTerrainAmbientColor(0.447059, 0.509804, 0.54902);

setFogFactor(0.25);
setFogThickness(0.15);
setFogColor(0.847059, 0.737255, 0.482353);

setPPEffect("hdr");
setPPContrast(0.57031);
setPPBloom(0.34);

ExportMap();
