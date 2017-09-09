RMS.LoadLibrary("rmgen");

var tPrimary = ["polar_snow_a"];
var tCliff = ["polar_cliff_a", "polar_cliff_b", "polar_cliff_snow"];
var tSecondary = "polar_snow_glacial";
var tHalfSnow = ["ice_01", "ice_dirt"];
var tSnowLimited = ["polar_snow_b", "polar_ice"];
var tDirt = "ice_dirt";
var tRoad = "polar_ice_b";
var tRoadWild = "polar_ice_cracked";
var tShore = "polar_ice_snow";
var tWater = "polar_ice_c";

var oArcticFox = "gaia/fauna_fox_arctic";
var oWolf = "trigger/fauna_wolf_snow_attack";
var oMuskox = "gaia/fauna_muskox";
var oWalrus = "gaia/fauna_walrus";
var oWhaleFin = "gaia/fauna_whale_fin";
var oWhaleHumpback = "gaia/fauna_whale_humpback";
var oFish = "gaia/fauna_fish";
var oStoneLarge = "gaia/geology_stonemine_medit_quarry";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_desert_badlands_slabs";
var oWood = "gaia/special_treasure_wood";

var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aIceberg = "actor|props/special/eyecandy/iceberg.xml";

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clHill = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clWolf = createTileClass();

// Expected by rmgen1
var clForest = createTileClass();

initTerrain(tPrimary);

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

RMS.SetProgress(20);

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
	let cityRadius = scaleByMapSize(15,25)/3;
	let placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	let painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id, { 'iberWall': 'towers' });

	placeDefaultChicken(fx, fz, clBaseResource, undefined, oMuskox);

	// Create metal mine
	let mAngle = randFloat(0, TWO_PI);
	let mDist = 12;
	let mX = Math.round(fx + mDist * Math.cos(mAngle));
	let mZ = Math.round(fz + mDist * Math.sin(mAngle));
	let group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// Create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = Math.round(fx + mDist * Math.cos(mAngle));
	mZ = Math.round(fz + mDist * Math.sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1, 1, 0, 2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// Create wood treasure
	mAngle += PI/4;
	let bbX = Math.round(fx + mDist * Math.cos(mAngle));
	let bbZ = Math.round(fz + mDist * Math.sin(mAngle));
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oWood, 14,14, 0,3)],
			true, clBaseResource, bbX, bbZ,
			avoidClasses(clBaseResource, 4)
		), 0);

	// Create market
	mAngle += PI/4;
	placeObject(
		Math.round(fx + mDist * Math.cos(mAngle)),
		Math.round(fz + mDist * Math.sin(mAngle)),
		"structures/" + getCivCode(id-1) + "_market",
		id,
		BUILDING_ORIENTATION);
}
RMS.SetProgress(30);

log("Creating central lake...");
var placer = new ChainPlacer(
	2,
	Math.floor(scaleByMapSize(5, 16)),
	Math.floor(scaleByMapSize(35, 200)),
	1,
	Math.round(fractionToTiles(0.5)),
	Math.round(fractionToTiles(0.5)),
	0,
	[Math.floor(mapSize * 0.17)]);

var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater, tWater],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	-4,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 20));

paintTerrainBasedOnHeight(3, Math.floor(scaleByMapSize(20, 40)), 0, tCliff);
paintTerrainBasedOnHeight(Math.floor(scaleByMapSize(20, 40)), 100, 3, tSnowLimited);
RMS.SetProgress(40);

log("Creating small lakes...");
var lakeAreas = [];
var playerConstraint = new AvoidTileClassConstraint(clPlayer, 20);
var waterConstraint = new AvoidTileClassConstraint(clWater, 8);
for (let x = 0; x < mapSize; ++x)
	for (let z = 0; z < mapSize; ++z)
		if (playerConstraint.allows(x, z) && waterConstraint.allows(x, z))
			lakeAreas.push([x, z]);

var numLakes = scaleByMapSize(10, 16);
for (let i = 0; i < numLakes ; ++i)
{
	let chosenPoint = pickRandom(lakeAreas);
	if (!chosenPoint)
		break;

	placer = new ChainPlacer(
		1,
		Math.floor(scaleByMapSize(2, 4)),
		Math.floor(scaleByMapSize(20, 140)),
		0.7,
		chosenPoint[0],
		chosenPoint[1]);

	let terrainPainter = new LayeredPainter(
		[tShore, tWater, tWater],		// terrains
		[1, 3]								// widths
	);
	let elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 5);
	createAreas(
		placer,
		[terrainPainter, elevationPainter, paintClass(clWater)],
		avoidClasses(clPlayer, 20),
		1, 1
	);
}
RMS.SetProgress(50);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));
RMS.SetProgress(60);

log("Creating hills...");
createHills(
	[tPrimary, tPrimary, tSecondary],
	avoidClasses(clPlayer, 20, clHill, 35),
	clHill, scaleByMapSize(20, 240));
RMS.SetProgress(65);

log("Creating dirt patches...");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tDirt,tHalfSnow], [tHalfSnow,tSnowLimited]],
	[2],
	avoidClasses(clWater, 3, clDirt, 5, clPlayer, 12)
);

log("Creating glacier patches...");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tSecondary,
	avoidClasses(clWater, 3, clDirt, 5, clPlayer, 12)
);
RMS.SetProgress(70);

log("Creating stone mines...");
	createMines(
	[
		[new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
		[new SimpleObject(oStoneSmall, 2,5, 1,3)]
	],
	avoidClasses(clWater, 3, clPlayer, 20, clRock, 18, clHill, 2)
);

log("Creating metal mines...");
createMines(
	[
		[new SimpleObject(oMetalLarge, 1,1, 0,4)]
	],
	avoidClasses(clWater, 3, clPlayer, 20, clMetal, 18, clRock, 5, clHill, 2),
	clMetal
);
RMS.SetProgress(75);

createDecoration(
	[
		[new SimpleObject(aRockMedium, 1,3, 0,1)],
		[
			new SimpleObject(aRockLarge, 1,2, 0,1),
			new SimpleObject(aRockMedium, 1,3, 0,2)
		]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
	],
	avoidClasses(clWater, 0, clPlayer, 0)
);

createDecoration(
	[
		[new SimpleObject(aIceberg, 1, 1, 1, 1)]
	],
	[
		scaleByMapSize(8, 131)
	],
	[stayClasses(clWater, 4), avoidClasses(clHill, 2)]
);
RMS.SetProgress(80);

createFood(
	[
		[new SimpleObject(oArcticFox, 1,2, 0,3)],
		[new SimpleObject(oWolf, 4,6, 0,4)],
		[new SimpleObject(oWalrus, 2,3, 0,2)],
		[new SimpleObject(oMuskox, 2,3, 0,2)]
	],
	[
		3 * numPlayers,
		5 * numPlayers,
		5 * numPlayers,
		12 * numPlayers
	],
	avoidClasses(clPlayer, 35, clFood, 16, clWater, 2, clMetal, 4, clRock, 4, clHill, 2)
);

createFood(
	[
		[new SimpleObject(oWhaleFin, 1,2, 0,2)],
		[new SimpleObject(oWhaleHumpback, 1,2, 0,2)]
	],
	[
		scaleByMapSize(1, 6) * 3,
		scaleByMapSize(1, 6) * 3,
	],
	[avoidClasses(clFood, 20, clHill, 5), stayClasses(clWater, 6)]
);

createFood(
	[
		[new SimpleObject(oFish, 2,3, 0,2)]
	],
	[
		100
	],
	[avoidClasses(clFood, 12, clHill, 5), stayClasses(clWater, 6)]
);
RMS.SetProgress(85);

// Create trigger points where wolves spawn
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject("special/trigger_point_A", 1, 1, 0, 0)], true, clWolf),
	0,
	avoidClasses(clWater, 2, clMetal, 4, clRock, 4, clPlayer, 15, clHill, 2, clWolf, 20),
	1000,
	100
);
RMS.SetProgress(95);

if (randBool(1/3))
{
	setSkySet("sunset 1");
	setSunColor(0.8, 0.7, 0.6);
	setTerrainAmbientColor(0.7, 0.6, 0.7);
	setUnitsAmbientColor(0.6, 0.5, 0.6);
	setSunElevation(randFloat(PI/24, PI/7));
}
else
{
	setSkySet(pickRandom(["cumulus", "rain", "mountainous", "overcast", "rain", "stratus"]));
	setSunElevation(randFloat(PI/9, PI/7));
}

setSunRotation(randFloat(0, TWO_PI));

setWaterColor(0.3, 0.3, 0.4);
setWaterTint(0.75, 0.75, 0.75);
setWaterMurkiness(0.92);
setWaterWaviness(0.5);
setWaterType("clap");

setFogThickness(0.76);
setFogFactor(0.7);

setPPEffect("hdr");
setPPContrast(0.6);
setPPSaturation(0.45);
setPPBloom(0.4);

ExportMap();
