/**
 * Returns starting position in tile coordinates for the given player.
 */
function getPlayerTileCoordinates(playerIdx, teamIdx, fractionX, fractionZ)
{
	let playerAngle = startAngle + (playerIdx+1) * TWO_PI / teams[teamIdx].length;

	let fx = fractionToTiles(fractionX + 0.05 * cos(playerAngle));
	let fz = fractionToTiles(fractionZ + 0.05 * sin(playerAngle));

	return [playerAngle, fx, fz, round(fx), round(fz)];
}

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmbiome");
RMS.LoadLibrary("heightmap");

const g_InitialMines = 1;
const g_InitialMineDistance = 14;
const g_InitialTrees = 50;

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.hill;
const tTier4Terrain = g_Terrains.tier4Terrain;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oWhale = "gaia/fauna_whale_humpback";
const oShipwreck = "other/special_treasure_shipwreck";
const oShipDebris = "other/special_treasure_shipwreck_debris";
const oObelisk = "other/obelisk";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

let clPlayer = createTileClass();
let clHill = createTileClass();
let clForest = createTileClass();
let clDirt = createTileClass();
let clRock = createTileClass();
let clMetal = createTileClass();
let clFood = createTileClass();
let clBaseResource = createTileClass();
let clLand = createTileClass();

initTerrain(tWater);

// some constants
let radius = scaleByMapSize(15, 25);

let fx = fractionToTiles(0.5);
let fz = fractionToTiles(0.5);

let startAngle = randFloat(0, TWO_PI);

// Group players by team
let teams = [];
for (let i = 0; i < numPlayers; ++i)
{
	let team = getPlayerTeam(i);
	if (team == -1)
		continue;

	if (!teams[team])
		teams[team] = [];

	teams[team].push(i+1);
}

// Players without a team get a custom index
for (let i = 0; i < numPlayers; ++i)
{
	let team = getPlayerTeam(i);
	if (team != -1)
		continue;

	let unusedIndex = teams.findIndex(team => !team);

	if (unusedIndex != -1)
		teams[unusedIndex] = [i+1];
	else
		teams.push([i+1]);
}

// Get number of used team IDs
let numTeams = teams.filter(team => team).length;

RMS.SetProgress(10);

let shoreRadius = 6;
let elevation = 3;
let teamNo = 0;

for (let i = 0; i < teams.length; ++i)
{
	if (!teams[i])
		continue;

	++teamNo;
	let teamAngle = startAngle + teamNo*TWO_PI/numTeams;
	let fractionX = 0.5 + 0.3 * cos(teamAngle);
	let fractionZ = 0.5 + 0.3 * sin(teamAngle);
	let teamX = fractionToTiles(fractionX);
	let teamZ = fractionToTiles(fractionZ);

	log("Creating island and starting entities for team " + i);
	for (let p = 0; p < teams[i].length; ++p)
	{
		let [playerAngle, fx, fz, ix, iz] = getPlayerTileCoordinates(p, i, fractionX, fractionZ);

		addCivicCenterAreaToClass(ix, iz, clPlayer);

		// create an island
		let placer = new ChainPlacer(2, floor(scaleByMapSize(5, 11)), floor(scaleByMapSize(60, 250)), 1, ix, iz, 0, [floor(mapSize * 0.01)]);
		let terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain, tMainTerrain],       // terrains
			[1, shoreRadius]     // widths
		);
		let elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,          // type
			elevation,              // elevation
			shoreRadius               // blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);

		placeCivDefaultEntities(fx, fz, teams[i][p], { "iberWall": false });
	}

	log("Create initial mines for team " + i);
	for (let p = 0; p < teams[i].length; ++p)
	{
		let [playerAngle, fx, fz, ix, iz] = getPlayerTileCoordinates(p, i, fractionX, fractionZ);
		let mAngle = randFloat(playerAngle - PI / teams[i].length, playerAngle + PI / teams[i].length);

		// Metal
		let mX = round(fx + g_InitialMineDistance * cos(mAngle));
		let mZ = round(fz + g_InitialMineDistance * sin(mAngle));
		let group = new SimpleGroup(
			[new SimpleObject(oMetalLarge, g_InitialMines, g_InitialMines, 0, 4)],
			true, clBaseResource, mX, mZ
		);
		createObjectGroup(group, 0, [avoidClasses(clBaseResource, 2, clPlayer, 4), stayClasses(clLand, 2)]);

		// Stone
		let sX = round(fx + g_InitialMineDistance * cos(mAngle + PI/4));
		let sZ = round(fz + g_InitialMineDistance * sin(mAngle + PI/4));
		group = new SimpleGroup(
			[new SimpleObject(oStoneLarge, g_InitialMines, g_InitialMines, 0, 4)],
			true, clBaseResource, sX, sZ
		);
		createObjectGroup(group, 0, [avoidClasses(clBaseResource, 2, clPlayer, 4), stayClasses(clLand, 2)]);
	}

	log("Place initial trees and animals for team " + i);
	for (let p = 0; p < teams[i].length; ++p)
	{
		let [playerAngle, fx, fz, ix, iz] = getPlayerTileCoordinates(p, i, fractionX, fractionZ);

		placeDefaultChicken(fx, fz, clBaseResource, [stayClasses(clLand, 5)]);

		// create initial berry bushes
		let bbAngle = randFloat(PI, PI*1.5);
		let bbDist = 10;
		let bbX = round(fx + bbDist * cos(bbAngle));
		let bbZ = round(fz + bbDist * sin(bbAngle));
		let group = new SimpleGroup(
			[new SimpleObject(oFruitBush, 5, 5, 0, 3)],
			true, clBaseResource, bbX, bbZ
		);
		createObjectGroup(group, 0, [avoidClasses(clBaseResource, 4, clPlayer, 4), stayClasses(clLand, 5)]);

		// create initial trees
		let tries = 10;
		let tDist = 16;
		for (let x = 0; x < tries; ++x)
		{
			let tAngle = randFloat(playerAngle - TWO_PI/teams[i].length,
			                       playerAngle + TWO_PI/teams[i].length);

			let tX = round(fx + tDist * cos(tAngle));
			let tZ = round(fz + tDist * sin(tAngle));

			group = new SimpleGroup(
				[new SimpleObject(oTree2, g_InitialTrees, g_InitialTrees, 0, 7)],
				true, clBaseResource, tX, tZ
			);
			if (createObjectGroup(group, 0, [avoidClasses(clBaseResource, 4, clPlayer, 4), stayClasses(clLand, 4)]))
				break;
		}

		// create huntable animals
		group = new SimpleGroup(
			[new SimpleObject(oMainHuntableAnimal, 2 * numPlayers / numTeams, 2 * numPlayers / numTeams, 0, floor(mapSize * 0.2))],
			true, clBaseResource, teamX, teamZ
		);
		createObjectGroup(group, 0, [avoidClasses(clBaseResource, 2, clHill, 1, clPlayer, 10), stayClasses(clLand, 5)]);
		group = new SimpleGroup(
			[new SimpleObject(oSecondaryHuntableAnimal, 4 * numPlayers / numTeams, 4 * numPlayers / numTeams, 0, floor(mapSize * 0.2))],
			true, clBaseResource, teamX, teamZ
		);
		createObjectGroup(group, 0, [avoidClasses(clBaseResource, 2, clHill, 1, clPlayer, 10), stayClasses(clLand, 5)]);
	}
}

RMS.SetProgress(40);

log("Creating expansion islands...");
let landAreas = [];
let playerConstraint = new AvoidTileClassConstraint(clPlayer, floor(scaleByMapSize(12, 16)));
let landConstraint = new AvoidTileClassConstraint(clLand, floor(scaleByMapSize(12, 16)));

for (let x = 0; x < mapSize; ++x)
	for (let z = 0; z < mapSize; ++z)
		if (playerConstraint.allows(x, z) && landConstraint.allows(x, z))
			landAreas.push([x, z]);

log("Creating big islands...");
let chosenPoint;
let landAreaLen;
let numIslands = scaleByMapSize(4, 14);
for (let i = 0; i < numIslands; ++i)
{
	landAreaLen = landAreas.length;
	if (!landAreaLen)
		break;

	chosenPoint = pickRandom(landAreas);

	// create big islands
	let placer = new ChainPlacer(floor(scaleByMapSize(4, 8)), floor(scaleByMapSize(8, 14)), floor(scaleByMapSize(25, 60)), 0.07, chosenPoint[0], chosenPoint[1], scaleByMapSize(30, 70));
	let terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain],		// terrains
		[2]								// widths
	);

	let elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 6);
	let newIsland = createAreas(
		placer,
		[terrainPainter, elevationPainter, paintClass(clLand)],
		avoidClasses(clLand, 3, clPlayer, 3),
		1, 1
	);

	if (!newIsland || !newIsland.length)
		continue;

	let n = 0;
	for (let j = 0; j < landAreaLen; ++j)
	{
		let x = landAreas[j][0];
		let z = landAreas[j][1];

		if (playerConstraint.allows(x, z) && landConstraint.allows(x, z))
			landAreas[n++] = landAreas[j];
	}
	landAreas.length = n;
}

playerConstraint = new AvoidTileClassConstraint(clPlayer, floor(scaleByMapSize(9, 12)));
landConstraint = new AvoidTileClassConstraint(clLand, floor(scaleByMapSize(9, 12)));

log("Creating small islands...");
numIslands = scaleByMapSize(6, 18) * scaleByMapSize(1, 3);
for (let i = 0; i < numIslands; ++i)
{
	landAreaLen = landAreas.length;
	if (!landAreaLen)
		break;

	chosenPoint = pickRandom(landAreas);

	let placer = new ChainPlacer(floor(scaleByMapSize(4, 7)), floor(scaleByMapSize(7, 10)), floor(scaleByMapSize(16, 40)), 0.07, chosenPoint[0], chosenPoint[1], scaleByMapSize(22, 40));
	let terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain],		// terrains
		[2]								// widths
	);

	let elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 6);
	let newIsland = createAreas(
		placer,
		[terrainPainter, elevationPainter, paintClass(clLand)],
		avoidClasses(clLand, 3, clPlayer, 3),
		1, 1
	);

	if (newIsland === undefined)
		continue;

	let temp = [];
	for (let j = 0; j < landAreaLen; ++j)
	{
		let x = landAreas[j][0];
		let z = landAreas[j][1];

		if (playerConstraint.allows(x, z) && landConstraint.allows(x, z))
			temp.push([x, z]);
	}
	landAreas = temp;
}

RMS.SetProgress(70);

log("Smoothing heightmap...");
for (let i = 0; i < 5; ++i)
	globalSmoothHeightmap();

// repaint clLand to compensate for smoothing
unPaintTileClassBasedOnHeight(-10, 10, 3, clLand);
paintTileClassBasedOnHeight(0, 5, 3, clLand);

RMS.SetProgress(85);

createBumps();

createMines(
[
	[new SimpleObject(oMetalLarge, 1, 1, 3, (numPlayers * 2) + 1)]
],
[avoidClasses(clForest, 1, clPlayer, 40, clRock, 20, clHill, 5), stayClasses(clLand, 4)],
clMetal
);

createMines(
[
	[new SimpleObject(oStoneLarge, 1, 1, 3, (numPlayers * 2) + 1)], [new SimpleObject(oStoneSmall, 2, 2, 2, (numPlayers * 2) + 1)]
],
[avoidClasses(clForest, 1, clPlayer, 40, clMetal, 20, clHill, 5), stayClasses(clLand, 4)],
clRock
);

createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 [avoidClasses(clPlayer, 10, clForest, 20, clHill, 10, clBaseResource, 5, clRock, 6, clMetal, 6), stayClasses(clLand, 3)],
 clForest,
 1,
 ...rBiomeTreeCount(1));

log("Creating hills...");
let placer = new ChainPlacer(1, floor(scaleByMapSize(4, 6)), floor(scaleByMapSize(16, 40)), 0.5);
let painter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
let elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[painter, elevationPainter, paintClass(clHill)],
	[avoidClasses(clBaseResource, 20, clHill, 15, clRock, 6, clMetal, 6), stayClasses(clLand, 0)],
	scaleByMapSize(4, 13)
);
for (let i = 0; i < 3; ++i)
	globalSmoothHeightmap();

createStragglerTrees(
		[oTree1, oTree2, oTree4, oTree3],
		[avoidClasses(clForest, 10, clPlayer, 20, clMetal, 6, clRock, 6, clHill, 1),
		 stayClasses(clLand, 4)]
);

createFood(
	[
		[new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)],
		[new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)]
	],
	[3 * numPlayers, 3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clRock, 6, clMetal, 6), stayClasses(clLand, 2)]
);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 15, clHill, 1, clFood, 4, clRock, 6, clMetal, 6), stayClasses(clLand, 2)]
);

if (currentBiome() == "desert")
{
	log("Creating obelisks");
	let group = new SimpleGroup(
		[new SimpleObject(oObelisk, 1, 1, 0, 1)],
		true
	);
	createObjectGroupsDeprecated(
		group, 0,
		[avoidClasses(clBaseResource, 0, clHill, 0, clRock, 0, clMetal, 0, clFood, 0), stayClasses(clLand, 1)],
		scaleByMapSize(3, 8), 1000
	);
}

log("Creating dirt patches...");
let sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
let numb = currentBiome() == "savanna" ? 3 : 1;

for (let i = 0; i < sizes.length; ++i)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new LayeredPainter(
		[[tMainTerrain,tTier1Terrain], [tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],		// terrains
		[1, 1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 4)],
		numb*scaleByMapSize(15, 45)
	);
}

log("Creating grass patches...");
sizes = [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)];
for (let i = 0; i < sizes.length; ++i)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	painter = new TerrainPainter(tTier4Terrain);
	createAreas(
		placer,
		painter,
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 4)],
		numb * scaleByMapSize(15, 45)
	);
}

log("Creating small decorative rocks...");
let group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clHill, 0), stayClasses(clLand, 2)],
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clHill, 0), stayClasses(clLand, 2)],
	scaleByMapSize(8, 131), 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2, 3, 0, 2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clLand, 4, clFood, 20),
	25 * numPlayers, 60
);

log("Creating Whales...");
group = new SimpleGroup(
	[new SimpleObject(oWhale, 1, 1, 0, 3)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clLand, 4),avoidClasses(clFood, 8)],
	scaleByMapSize(5, 20), 100
);

log("Creating shipwrecks...");
group = new SimpleGroup(
	[new SimpleObject(oShipwreck, 1, 1, 0, 1)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clLand, 4),avoidClasses(clFood, 8)],
	scaleByMapSize(12, 16), 100
);

log("Creating shipwreck debris...");
group = new SimpleGroup(
	[new SimpleObject(oShipDebris, 1, 1, 0, 1)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clLand, 4),avoidClasses(clFood, 8)],
	scaleByMapSize(10, 20), 100
);

placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius, [stayClasses(clLand, 5)]);

log("Creating small grass tufts...");
let planetm = currentBiome() == "tropic" ? 8 : 1;
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1, 2, 0, 1, -PI / 8, PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2, 4, 0, 1.8, -PI / 8, PI / 8), new SimpleObject(aGrassShort, 3, 6, 1.2,2.5, -PI / 8, PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 5)],
	planetm * scaleByMapSize(13, 200)
);

paintTerrainBasedOnHeight(1, 2, 0, tShore);
paintTerrainBasedOnHeight(getMapBaseHeight(), 1, 3, tWater);

setSkySet(pickRandom(["cloudless", "cumulus", "overcast"]));
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/5, PI/3));
setWaterWaviness(2);

RMS.SetProgress(100);

ExportMap();
