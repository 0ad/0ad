Engine.LoadLibrary("rmgen");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

const tGrass = ["new_alpine_grass_b", "new_alpine_grass_c", "new_alpine_grass_d"];
const tPineForestFloor = "temp_forestfloor_pine";
const tForestFloor = [tPineForestFloor, tPineForestFloor, "alpine_dirt_grass_50"];
const tCliff = ["alpine_cliff_c", "alpine_cliff_c", "alpine_grass_rocky"];
const tCity = ["new_alpine_citytile", "new_alpine_grass_dirt_a"];
const tGrassPatch = ["alpine_grass_a", "alpine_grass_b"];

const oBoar = "gaia/fauna_boar";
const oDeer = "gaia/fauna_deer";
const oBear = "gaia/fauna_bear";
const oPig = "gaia/fauna_pig";
const oBerryBush = "gaia/flora_bush_berry";
const oMetalSmall = "gaia/geology_metal_alpine";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";
const oStoneSmall = "gaia/geology_stone_alpine_a";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";

const oOak = "gaia/flora_tree_oak";
const oOakLarge = "gaia/flora_tree_oak_large";
const oPine = "gaia/flora_tree_pine";
const oAleppoPine = "gaia/flora_tree_aleppo_pine";

const aTreeA = "actor|flora/trees/oak.xml";
const aTreeB = "actor|flora/trees/oak_large.xml";
const aTreeC = "actor|flora/trees/pine.xml";
const aTreeD = "actor|flora/trees/aleppo_pine.xml";

const aTrees = [aTreeA, aTreeB, aTreeC, aTreeD];

const aGrassLarge = "actor|props/flora/grass_soft_large.xml";
const aWoodLarge = "actor|props/special/eyecandy/wood_pile_1_b.xml";
const aWoodA = "actor|props/special/eyecandy/wood_sm_pile_a.xml";
const aWoodB = "actor|props/special/eyecandy/wood_sm_pile_b.xml";
const aBarrel = "actor|props/special/eyecandy/barrel_a.xml";
const aWheel = "actor|props/special/eyecandy/wheel_laying.xml";
const aCeltHomestead = "actor|structures/celts/homestead.xml";
const aCeltHouse = "actor|structures/celts/house.xml";
const aCeltLongHouse = "actor|structures/celts/longhouse.xml";

var pForest = [
		tPineForestFloor+TERRAIN_SEPARATOR+oOak, tForestFloor,
		tPineForestFloor+TERRAIN_SEPARATOR+oPine, tForestFloor,
		tPineForestFloor+TERRAIN_SEPARATOR+oAleppoPine, tForestFloor,
		tForestFloor
		];

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clForestJoin = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clHillDeco = createTileClass();

log("Creating the central dip...");
createArea(
	new ClumpPlacer(mapSize * scaleByMapSize(70, 300), 0.94, 0.05, 0.1, fractionToTiles(0.5), fractionToTiles(0.5)),
	[
		new LayeredPainter([tCliff, tGrass], [3]),
		new SmoothElevationPainter(ELEVATION_SET, 30, 3)
	],
	null);

Engine.SetProgress(5);

// Find all hills
var noise0 = new Noise2D(20);
for (var ix = 0; ix < mapSize; ix++)
	for (var iz = 0; iz < mapSize; iz++)
	{
		var h = getHeight(ix,iz);
		if(h > 40){
			addToClass(ix,iz,clHill);

			// Add hill noise
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
			var n = (noise0.get(x,z) - 0.5) * 40;
			setHeight(ix, iz, h + n);
		}
	}

var [playerIDs, playerX, playerZ] = radialPlayerPlacement(0.3);

function distanceToPlayers(x, z)
{
	var r = 10000;
	for (var i = 0; i < numPlayers; i++)
	{
		var dx = x - playerX[i];
		var dz = z - playerZ[i];
		r = Math.min(r, Math.square(dx) + Math.square(dz));
	}
	return Math.sqrt(r);
}

function playerNearness(x, z)
{
	var d = fractionToTiles(distanceToPlayers(x,z));

	if (d < 13)
		return 0;

	if (d < 19)
		return (d-13)/(19-13);

	return 1;
}

Engine.SetProgress(10);

for (var i=0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	placeCivDefaultEntities(fx, fz, id);

	var citySize = 250;

	// Create the city patch
	var placer = new ClumpPlacer(citySize * 0.4, 0.6, 0.05, 10, ix, iz);
	var painter = new TerrainPainter([tCity]);
	createArea(placer, painter, null);

	// Create starter animals
	placeDefaultChicken(fx, fz, clBaseResource, undefined, oPig);

	// Create starter berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 3,3, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// Create starter metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = bbDist + 4;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// Create starter stone mines
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
		[new SimpleObject(oOak, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

}

log("Marking player territory larger than the city patch...");
for (let i = 0; i < numPlayers; ++i)
	createArea(
		new ClumpPlacer(250, 0.95, 0.3, 0.1, Math.floor(fractionToTiles(playerX[i])), Math.floor(fractionToTiles(playerZ[i]))),
		paintClass(clPlayer));

Engine.SetProgress(30);

log("Creating hills...");
for (let size of [scaleByMapSize(50, 800), scaleByMapSize(50, 400), scaleByMapSize(10, 30), scaleByMapSize(10, 30)])
{
	let mountains = createAreas(
		new ClumpPlacer(size, 0.1, 0.2, 0.1),
		[
			new LayeredPainter([tCliff, [tForestFloor, tForestFloor, tCliff]], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 50, size < 50 ? 2 : 4),
			paintClass(clHill)
		],
		avoidClasses(clPlayer, 8, clBaseResource, 2, clHill, 5),
		scaleByMapSize(1, 4));

	if (size > 100 && mountains.length)
		createAreasInAreas(
			new ClumpPlacer(size * 0.3, 0.94, 0.05, 0.1),
			[
				new LayeredPainter([tCliff, tForestFloor], [2]),
				new SmoothElevationPainter(ELEVATION_MODIFY, 10, 3)
			],
			stayClasses(clHill, 4),
			mountains.length * 2,
			20,
			mountains);

	let ravine = createAreas(
		new ClumpPlacer(size, 0.1, 0.2, 0.1),
		[
			new LayeredPainter([tCliff, tForestFloor], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 10, 2),
			paintClass(clHill)
		],
		avoidClasses(clPlayer, 6, clBaseResource, 2, clHill, 5),
		scaleByMapSize(1, 3));

	if (size > 150 && ravine.length)
	{
		log("Placing huts in ravines...");
		createObjectGroupsByAreasDeprecated(
			new RandomGroup(
				[
					new SimpleObject(aCeltHouse, 0, 1, 4, 5),
					new SimpleObject(aCeltLongHouse, 1, 1, 4, 5)
				],
				true,
				clHillDeco),
			0,
			[avoidClasses(clHillDeco, 3), stayClasses(clHill, 3)],
			ravine.length * 5, 20,
			ravine);

		createObjectGroupsByAreasDeprecated(
			new RandomGroup([new SimpleObject(aCeltHomestead, 1, 1, 1, 1)], true, clHillDeco),
			0,
			[avoidClasses(clHillDeco, 5), stayClasses(clHill, 4)],
			ravine.length * 2, 100,
			ravine);

		// Place noise
		createAreasInAreas(
			new ClumpPlacer(size * 0.3, 0.94, 0.05, 0.1),
			[
				new LayeredPainter([tCliff, tForestFloor], [2]),
				new SmoothElevationPainter(ELEVATION_SET, 2, 2)
			],
			[avoidClasses(clHillDeco, 2), stayClasses(clHill, 0)],
			ravine.length * 2,
			20,
			ravine);

		createAreasInAreas(
			new ClumpPlacer(size * 0.1, 0.3, 0.05, 0.1),
			[
				new LayeredPainter([tCliff, tForestFloor], [2]),
				new SmoothElevationPainter(ELEVATION_SET, 40, 2),
				paintClass(clHill)
			],
			[avoidClasses(clHillDeco, 2), borderClasses(clHill, 15, 1)],
			ravine.length * 2,
			50,
			ravine);
	}
}

Engine.SetProgress(50);

var explorableArea = {};
explorableArea.points = [];

var playerClass = getTileClass(clPlayer);
var hillDecoClass = getTileClass(clHillDeco);

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var h = getHeight(ix,iz);

		if(h > 15 && h < 45 && playerClass.countMembersInRadius(ix, iz, 1) == 0)
		{
			// explorable area
			var pt = {};
			pt.x = ix;
			pt.z = iz;
			explorableArea.points.push(pt);
		}

		if (h > 35 && randBool(0.1) ||
		    h < 15 && randBool(0.05) && hillDecoClass.countMembersInRadius(ix, iz, 1) == 0)
			placeObject(ix + randFloat(0, 1), iz + randFloat(0, 1), pickRandom(aTrees), 0, randFloat(0, 2 * PI));
	}
}

Engine.SetProgress(55);

// Add some general noise - after placing height dependant trees
for (var ix = 0; ix < mapSize; ix++)
{
	var x = ix / (mapSize + 1.0);
	for (var iz = 0; iz < mapSize; iz++)
	{
		var z = iz / (mapSize + 1.0);
		var h = getHeight(ix,iz);
		var pn = playerNearness(x,z);
		var n = (noise0.get(x,z) - 0.5) * 10;
		setHeight(ix, iz, h + (n * pn));
	}
}

Engine.SetProgress(60);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(400, 6000, 0.8);
var [forestTreesJoin, forestTrees] = getTreeCounts(forestTrees, forestTrees, 0.25);

var num = forestTrees / (scaleByMapSize(6, 16) * numPlayers);
createAreasInAreas(
	new ClumpPlacer(forestTrees / num, 0.1, 0.1, 1),
	[
		new TerrainPainter(pForest),
		paintClass(clForest)
	],
	avoidClasses(clPlayer, 5, clBaseResource, 4, clForest, 6, clHill, 4),
	num,
	100,
	[explorableArea]
);

var num = forestTreesJoin / (scaleByMapSize(4,6) * numPlayers);
createAreasInAreas(
	new ClumpPlacer(forestTreesJoin / num, 0.1, 0.1, 1),
	[
		new TerrainPainter(pForest),
		paintClass(clForest),
		paintClass(clForestJoin)
	],
	[avoidClasses(clPlayer, 5, clBaseResource, 4, clForestJoin, 5, clHill, 4), borderClasses(clForest, 1, 4)],
	num,
	100,
	[explorableArea]
);

Engine.SetProgress(70);

log("Creating grass patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new LayeredPainter([[tGrass, tGrassPatch], [tGrassPatch, tGrass], [tGrass, tGrassPatch]], [1, 1]),
		avoidClasses(clForest, 0, clHill, 2, clPlayer, 5),
		scaleByMapSize(15, 45));

log("Creating chopped forest patches...");
for (let size of [scaleByMapSize(20, 120)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tForestFloor),
		avoidClasses(clForest, 1, clHill, 2, clPlayer, 5),
		scaleByMapSize(4, 12));

Engine.SetProgress(75);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 1,2, 0,4), new SimpleObject(oStoneLarge, 0,1, 0,4)], true, clRock);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clHill, 4, clForest, 2, clPlayer, 20, clRock, 10)],
	scaleByMapSize(6,20), 100,
	[explorableArea]
);

log("Creating small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clHill, 4, clForest, 2, clPlayer, 20, clRock, 10)],
	scaleByMapSize(6,20), 100,
	[explorableArea]
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalSmall, 1,2, 0,4), new SimpleObject(oMetalLarge, 0,1, 0,4)], true, clMetal);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clHill, 4, clForest, 2, clPlayer, 20, clMetal, 10, clRock, 5)],
	scaleByMapSize(6,20), 100,
	[explorableArea]
);

Engine.SetProgress(80);

log("Creating wildlife...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clHill, 4, clForest, 0, clPlayer, 0, clBaseResource, 20),
	3 * numPlayers, 100,
	[explorableArea]
);

group = new SimpleGroup(
	[new SimpleObject(oBoar, 2,3, 0,5)],
	true, clFood
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clHill, 4, clForest, 0, clPlayer, 0, clBaseResource, 15),
	numPlayers, 50,
	[explorableArea]
);

group = new SimpleGroup(
	[new SimpleObject(oBear, 1,1, 0,4)],
	false, clFood
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clHill, 4, clForest, 0, clPlayer, 20),
	scaleByMapSize(3, 12), 200,
	[explorableArea]
);

Engine.SetProgress(85);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 4, clFood, 20),
	randIntInclusive(3, 12) * numPlayers + 2, 50
);

log("Creating decorative props...");
group = new SimpleGroup(
	[
		new SimpleObject(aWoodA, 1,2, 0,1),
		new SimpleObject(aWoodB, 1,3, 0,1),
		new SimpleObject(aWheel, 0,2, 0,1),
		new SimpleObject(aWoodLarge, 0,1, 0,1),
		new SimpleObject(aBarrel, 0,2, 0,1)
	],
	true
);
createObjectGroupsByAreasDeprecated(
	group, 0,
	avoidClasses(clForest, 0),
	scaleByMapSize(5, 50), 50,
	[explorableArea]
);

Engine.SetProgress(90);

log("Creating straggler trees...");
var types = [oOak, oOakLarge, oPine, oAleppoPine];
var num = Math.floor(stragglerTrees / types.length);
for (let type of types)
	createObjectGroupsByAreasDeprecated(
		new SimpleGroup([new SimpleObject(type, 1, 1, 0, 3)], true, clForest),
		0,
		avoidClasses(clForest, 4, clHill, 5, clPlayer, 10, clBaseResource, 2, clMetal, 5, clRock, 5),
		num, 20,
		[explorableArea]);

Engine.SetProgress(95);

log("Creating grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassLarge, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2),
	scaleByMapSize(50, 300), 20,
	[explorableArea]
);

setTerrainAmbientColor(0.44,0.51,0.56);
setUnitsAmbientColor(0.44,0.51,0.56);

ExportMap();
