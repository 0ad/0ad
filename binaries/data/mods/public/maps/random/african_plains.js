RMS.LoadLibrary("rmgen");

var tPrimary = ["savanna_grass_a"];
var tForestFloor = "savanna_forestfloor_a";
var tCliff = ["savanna_cliff_a", "savanna_cliff_a_red", "savanna_cliff_b", "savanna_cliff_b_red"];
var tSecondary = "savanna_grass_b";
var tGrassShrubs = ["savanna_shrubs_a"];
var tGrass = ["savanna_grass_a_wetseason", "savanna_grass_b_wetseason"];
var tDirt = "savanna_dirt_a";
var tDirt2 = "savanna_dirt_a_red";
var tDirt3 = "savanna_dirt_b";
var tDirt4 = "savanna_dirt_rocks_a";
var tCitytiles = "savanna_tile_a";
var tShore = "savanna_riparian_bank";
var tWater = "savanna_riparian_wet";

var oBaobab = "gaia/flora_tree_baobab";
var oPalm = "gaia/flora_tree_senegal_date_palm";
var oBerryBush = "gaia/flora_bush_berry";
var oWildebeest = "gaia/fauna_wildebeest";
var oZebra = "gaia/fauna_zebra";
var oRhino = "gaia/fauna_rhino";
var oLion = "gaia/fauna_lion";
var oLioness = "gaia/fauna_lioness";
var oHawk = "gaia/fauna_hawk";
var oGiraffe = "gaia/fauna_giraffe";
var oGiraffe2 = "gaia/fauna_giraffe_infant";
var oGazelle = "gaia/fauna_gazelle";
var oElephant = "gaia/fauna_elephant_african_bush";
var oElephant2 = "gaia/fauna_elephant_african_infant";
var oCrocodile = "gaia/fauna_crocodile";
var oFish = "gaia/fauna_fish";
var oStoneSmall = "gaia/geology_stone_savanna_small";
var oMetalLarge = "gaia/geology_metal_savanna_slabs";

var aBush = "actor|props/flora/bush_medit_sm_dry.xml";
var aRock = "actor|geology/stone_savanna_med.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPalm, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
RMS.SetProgress(20);

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

initTerrain(tPrimary);

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

for (var i = 0; i < numPlayers; ++i)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);

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
	var painter = new LayeredPainter([tPrimary,tCitytiles], [1]);
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
	while (abs(mAngle - bbAngle) < PI/3)
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
	createStoneMineFormation(mX, mZ, tDirt4);
	addToClass(mX, mZ, clPlayer);

	// create starting trees
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBaobab, 2,7)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}
RMS.SetProgress(20);

createHills([tDirt2, tCliff, tGrassShrubs], avoidClasses(clPlayer, 35, clForest, 20, clHill, 20, clWater, 2), clHill, scaleByMapSize(5, 8));
RMS.SetProgress(30);

var lakeAreas = [];
var playerConstraint = new AvoidTileClassConstraint(clPlayer, 20);
var waterConstraint = new AvoidTileClassConstraint(clWater, 8);

for (var x = 0; x < mapSize; ++x)
	for (var z = 0; z < mapSize; ++z)
		if (playerConstraint.allows(x, z) && waterConstraint.allows(x, z))
			lakeAreas.push([x, z]);

log("Creating water holes...");
placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), floor(scaleByMapSize(60, 100)), 5);
var terrainPainter = new LayeredPainter(
	[tShore, tWater],		// terrains
	[1]							// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 7);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clWater)],
	avoidClasses(clPlayer, 22, clWater, 8, clHill, 2),
	scaleByMapSize(2, 5)
);
RMS.SetProgress(45);

paintTerrainBasedOnHeight(3, floor(scaleByMapSize(20, 40)), 0, tCliff);
paintTerrainBasedOnHeight(floor(scaleByMapSize(20, 40)), 100, 3, tGrass);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

createForests(
	[tPrimary, tForestFloor, tForestFloor, pForest, pForest],
	avoidClasses(clPlayer, 20, clForest, 20, clHill, 0, clWater, 2),
	clForest,
	1.0
);

log("Creating dirt patches...");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tDirt,tDirt3], [tDirt2,tDirt4]],
	[2],
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

log("Creating shrubs...");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tGrassShrubs,
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

log("Creating grass patches...");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tSecondary,
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);
RMS.SetProgress(60);

log("Creating stone mines...");
createMines(
	[
		[new SimpleObject(oStoneSmall, 0,2, 0,4)],
		[new SimpleObject(oStoneSmall, 2,5, 1,3)]
	],
	avoidClasses(clWater, 4, clForest, 4, clPlayer, 20, clRock, 10, clHill, 4)
);

log("Creating metal mines...");
createMines(
	[
		[new SimpleObject(oMetalLarge, 1,1, 0,4)]
	],
	avoidClasses(clWater, 4, clForest, 4, clPlayer, 20, clMetal, 18, clRock, 5, clHill, 4),
	clMetal
);
RMS.SetProgress(70);

createDecoration(
	[
		[new SimpleObject(aBush, 1,3, 0,1)],
		[new SimpleObject(aRock, 1,2, 0,1)]
	],
	[
		scaleByMapSize(8, 131),
		scaleByMapSize(8, 131),
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0)
);
RMS.SetProgress(75);

log("Creating giraffes...");
group = new SimpleGroup(
	[new SimpleObject(oGiraffe, 2,4, 0,4), new SimpleObject(oGiraffe2, 0,2, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clPlayer, 20, clFood, 11, clHill, 4),
	scaleByMapSize(4,12), 50
);

log("Creating elephants...");
group = new SimpleGroup(
	[new SimpleObject(oElephant, 2,4, 0,4), new SimpleObject(oElephant2, 0,2, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clPlayer, 20, clFood, 11, clHill, 4),
	scaleByMapSize(4,12), 50
);

log("Creating lions...");
group = new SimpleGroup(
	[new SimpleObject(oLion, 0,1, 0,4), new SimpleObject(oLioness, 2,3, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clPlayer, 20, clFood, 11, clHill, 4),
	scaleByMapSize(4,12), 50
);

createFood(
	[
		[new SimpleObject(oHawk, 1,1, 0,3)],
		[new SimpleObject(oGazelle, 3,5, 0,3)],
		[new SimpleObject(oZebra, 3,5, 0,3)],
		[new SimpleObject(oWildebeest, 4,6, 0,3)],
		[new SimpleObject(oRhino, 1,1, 0,3)]
	],
	[
		3 * numPlayers,
		3 * numPlayers,
		3 * numPlayers,
		3 * numPlayers,
		3 * numPlayers,
	],
	avoidClasses(clFood, 20, clWater, 5, clHill, 2, clPlayer, 16)
);

createFood(
	[
		[new SimpleObject(oCrocodile, 2,3, 0,3)]
	],
	[
		3 * numPlayers,
	],
	stayClasses(clWater, 6)
);

createFood(
	[
		[new SimpleObject(oBerryBush, 5,7, 0,4)]
	],
	[
		randIntInclusive(1, 4) * numPlayers + 2
	],
	avoidClasses(clWater, 3, clForest, 2, clPlayer, 20, clHill, 3, clFood, 10)
);

createFood(
	[
		[new SimpleObject(oFish, 2,3, 0,2)]
	],
	[
		15 * numPlayers
	],
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)]
);
RMS.SetProgress(85);

createStragglerTrees(
	[oBaobab],
	avoidClasses(clWater, 5, clForest, 2, clHill, 3, clPlayer, 12, clMetal, 4, clRock, 4)
);

setPPEffect("hdr");
setPPSaturation(0.48);
setPPContrast(0.53);
setPPBloom(0.12);

setFogThickness(0.25);
setFogFactor(0.25);
setFogColor(0.8, 0.7, 0.5);

setSkySet("sunny");
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 4, PI / 2));

setWaterColor(0.223, 0.247, 0.2); // dark majestic blue
setWaterTint(0.462, 0.756, 0.566); // light blue
setWaterMurkiness(5.92);
setWaterWaviness(0);
setWaterType("clap");

ExportMap();
