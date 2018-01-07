Engine.LoadLibrary("rmgen");

const tGrass = ["temp_grass", "temp_grass", "temp_grass_d"];
const tForestFloor = "temp_plants_bog";
const tGrassA = "temp_grass_plants";
const tGrassB = "temp_plants_bog";
const tMud = "temp_mud_a";
const tRoad = "temp_road";
const tRoadWild = "temp_road_overgrown";
const tShoreBlend = "temp_grass_plants";
const tShore = "temp_plants_bog";
const tWater = "temp_mud_a";

const oBeech = "gaia/flora_tree_euro_beech";
const oOak = "gaia/flora_tree_oak";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oHorse = "gaia/fauna_horse";
const oWolf = "gaia/fauna_wolf";
const oRabbit = "gaia/fauna_rabbit";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";

const aGrass = "actor|props/flora/grass_soft_small_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oBeech, tForestFloor];
const pForestP = [tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

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
	var ix = Math.round(fx);
	var iz = Math.round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = Math.round(fx + bbDist * cos(bbAngle));
	var bbZ = Math.round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while (Math.abs(mAngle - bbAngle) < Math.PI / 3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = radius - 4;
	var mX = Math.round(fx + mDist * cos(mAngle));
	var mZ = Math.round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = Math.round(fx + mDist * cos(mAngle));
	mZ = Math.round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = Math.floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = 12;
	var tX = Math.round(fx + tDist * cos(tAngle));
	var tZ = Math.round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBeech, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

Engine.SetProgress(15);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800));

log("Creating marshes...");
for (let i = 0; i < 7; ++i)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(6, 12)), Math.floor(scaleByMapSize(15, 60)), 0.8),
		[
			new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
			new SmoothElevationPainter(ELEVATION_SET, -2, 3),
			paintClass(clWater)
		],
		avoidClasses(clPlayer, 20, clWater, Math.round(scaleByMapSize(7,16)*randFloat(0.8,1.35))),
		scaleByMapSize(4,20));

log("Creating reeds...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aReeds, 5, 10, 0, 4), new SimpleObject(aLillies, 5, 10, 0, 4)], true),
	0,
	stayClasses(clWater, 1),
	scaleByMapSize(400,2000), 100);
Engine.SetProgress(40);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 1, 2),
	stayClasses(clWater, 2),
	scaleByMapSize(50, 100));

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 2500, 0.7);
var types = [
	[[tForestFloor, tGrass, pForestD], [tForestFloor, pForestD]],
	[[tForestFloor, tGrass, pForestP], [tForestFloor, pForestP]]
];
var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 4))), 1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 20, clWater, 0, clForest, 10, clHill, 1),
		num);
Engine.SetProgress(50);

log("Creating mud patches...");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 1),
		[
			new LayeredPainter([tGrassA, tGrassB, tMud], [1, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 8),
		scaleByMapSize(15, 45));

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1)],
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1)],
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(60);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

Engine.SetProgress(65);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	6 * numPlayers, 50
);

log("Creating horse...");
group = new SimpleGroup(
	[new SimpleObject(oHorse, 1,3, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers, 50
);

Engine.SetProgress(75);

log("Creating rabbit...");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 5,7, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	6 * numPlayers, 50
);

log("Creating wolf...");
group = new SimpleGroup(
	[new SimpleObject(oWolf, 1,3, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers, 50
);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

Engine.SetProgress(80);

createStragglerTrees(
	[oOak, oBeech],
	avoidClasses(clForest, 1, clHill, 1, clPlayer, 13, clMetal, 6, clRock, 6, clWater, 0),
	clForest,
	stragglerTrees);

Engine.SetProgress(85);

log("Creating small grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aGrassShort, 1, 2, 0, 1)]),
	0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 13, clDirt, 0),
	scaleByMapSize(13, 200));

Engine.SetProgress(90);

log("Creating large grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aGrass, 2, 4, 0, 1.8),
			new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)
		]),
	0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 13, clDirt, 1, clForest, 0),
	scaleByMapSize(13, 200));

Engine.SetProgress(95);

log("Creating bushes...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aBushMedium, 1, 2, 0, 2),
			new SimpleObject(aBushSmall, 2, 4, 0, 2)
		]),
	0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

setSkySet("cirrus");
setWaterColor(0.753,0.635,0.345);				// muddy brown
setWaterTint(0.161,0.514,0.635);				// clear blue for blueness
setWaterMurkiness(0.8);
setWaterWaviness(1.0);
setWaterType("clap");

setFogThickness(0.25);
setFogFactor(0.6);

setPPEffect("hdr");
setPPSaturation(0.44);
setPPBloom(0.3);

ExportMap();
