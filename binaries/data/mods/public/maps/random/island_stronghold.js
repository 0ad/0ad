Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

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
const oShipwreck = "gaia/treasure/shipwreck";
const oShipDebris = "gaia/treasure/shipwreck_debris";
const oObelisk = "other/obelisk";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

const heightSeaGround = -10;
const heightLand = 3;
const heightHill = 18;

var g_Map = new RandomMap(heightSeaGround, tWater);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

const clPlayer = g_Map.createTileClass();
const clHill = g_Map.createTileClass();
const clForest = g_Map.createTileClass();
const clDirt = g_Map.createTileClass();
const clRock = g_Map.createTileClass();
const clMetal = g_Map.createTileClass();
const clFood = g_Map.createTileClass();
const clBaseResource = g_Map.createTileClass();
const clLand = g_Map.createTileClass();

var startAngle = randomAngle();

var teams = getTeamsArray();
var numTeams = teams.filter(team => team).length;
var teamPosition = distributePointsOnCircle(numTeams, startAngle, fractionToTiles(0.3), mapCenter)[0];
var teamRadius = fractionToTiles(0.05);

var teamNo = 0;

g_Map.log("Creating player islands and bases");

for (let i = 0; i < teams.length; ++i)
{
	if (!teams[i] || isNomad())
		continue;

	++teamNo;

	let [playerPosition, playerAngle] = distributePointsOnCircle(teams[i].length, startAngle + 2 * Math.PI / teams[i].length, teamRadius, teamPosition[i]);
	playerPosition.forEach(position => position.round());

	for (let p = 0; p < teams[i].length; ++p)
	{
		addCivicCenterAreaToClass(playerPosition[p], clPlayer);

		createArea(
			new ChainPlacer(2, Math.floor(scaleByMapSize(5, 11)), Math.floor(scaleByMapSize(60, 250)), Infinity, playerPosition[p], Infinity, [defaultPlayerBaseRadius() * 3/4]),
			[
				new TerrainPainter(tMainTerrain),
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 2),
				new TileClassPainter(clLand)
			]);

		placeCivDefaultStartingEntities(playerPosition[p], teams[i][p], false);
	}

	let mineAngle = randFloat(-1, 1) * Math.PI / teams[i].length;
	let mines = [
		{ "template": oMetalLarge, "angle": mineAngle },
		{ "template": oStoneLarge, "angle": mineAngle + Math.PI / 4 }
	];

	// Mines
	for (let p = 0; p < teams[i].length; ++p)
		for (let mine of mines)
		{
			let position = Vector2D.add(playerPosition[p], new Vector2D(g_InitialMineDistance, 0).rotate(-playerAngle[p] - mine.angle));
			createObjectGroup(
				new SimpleGroup([new SimpleObject(mine.template, 1, 1, 0, 4)], true, clBaseResource, position),
				0,
				[avoidClasses(clBaseResource, 4, clPlayer, 4), stayClasses(clLand, 5)]);
		}

	// Trees
	for (let p = 0; p < teams[i].length; ++p)
	{
		let tries = 10;
		for (let x = 0; x < tries; ++x)
		{
			let tAngle = playerAngle[p] + randFloat(-1, 1) * 2 * Math.PI / teams[i].length;
			let treePosition = Vector2D.add(playerPosition[p], new Vector2D(16, 0).rotate(-tAngle)).round();
			if (createObjectGroup(
				new SimpleGroup([new SimpleObject(oTree2, g_InitialTrees, g_InitialTrees, 0, 7)], true, clBaseResource, treePosition),
				0,
				[avoidClasses(clBaseResource, 4, clPlayer, 4), stayClasses(clLand, 4)]))
				break;
		}
	}

	for (let p = 0; p < teams[i].length; ++p)
		placePlayerBaseBerries({
			"template": oFruitBush,
			"playerID": teams[i][p],
			"playerPosition": playerPosition[p],
			"BaseResourceClass": clBaseResource,
			"baseResourceConstraint": new AndConstraint([avoidClasses(clPlayer, 4), stayClasses(clLand, 5)])
		});

	for (let p = 0; p < teams[i].length; ++p)
		placePlayerBaseChicken({
			"playerID": teams[i][p],
			"playerPosition": playerPosition[p],
			"BaseResourceClass": clBaseResource,
			"baseResourceConstraint": new AndConstraint([avoidClasses(clPlayer, 4), stayClasses(clLand, 5)])
		});

	// Huntable animals
	for (let p = 0; p < teams[i].length; ++p)
	{
		createObjectGroup(
			new SimpleGroup([new SimpleObject(oMainHuntableAnimal, 2 * numPlayers / numTeams, 2 * numPlayers / numTeams, 0, Math.floor(fractionToTiles(0.2)))], true, clBaseResource, teamPosition[i]),
			0,
			[avoidClasses(clBaseResource, 2, clPlayer, 10), stayClasses(clLand, 5)]);

		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(oSecondaryHuntableAnimal, 4 * numPlayers / numTeams, 4 * numPlayers / numTeams, 0, Math.floor(fractionToTiles(0.2)))],
				true, clBaseResource, teamPosition[i]),
			0,
			[avoidClasses(clBaseResource, 2, clPlayer, 10), stayClasses(clLand, 5)]);
	}
}

Engine.SetProgress(40);

g_Map.log("Creating big islands");
createAreas(
	new ChainPlacer(
		Math.floor(scaleByMapSize(4, 8) * (isNomad() ? 2 : 1)),
		Math.floor(scaleByMapSize(8, 16) * (isNomad() ? 2 : 1)),
		Math.floor(scaleByMapSize(25, 60)),
		0.07,
		undefined,
		scaleByMapSize(30, 70)),
	[
		new TerrainPainter(tMainTerrain),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 6),
		new TileClassPainter(clLand)
	],
	avoidClasses(clLand, 3, clPlayer, 3),
	scaleByMapSize(4, 14) * (isNomad() ? 2 : 1),
	1);

g_Map.log("Creating small islands");
createAreas(
	new ChainPlacer(Math.floor(scaleByMapSize(4, 7)), Math.floor(scaleByMapSize(7, 10)), Math.floor(scaleByMapSize(16, 40)), 0.07, undefined, scaleByMapSize(22, 40)),
	[
		new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 6),
		new TileClassPainter(clLand)
	],
	avoidClasses(clLand, 3, clPlayer, 3),
	scaleByMapSize(6, 55),
	1);

Engine.SetProgress(70);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, 0.8, 5));

// repaint clLand to compensate for smoothing
unPaintTileClassBasedOnHeight(-10, 10, 3, clLand);
paintTileClassBasedOnHeight(0, 5, 3, clLand);

Engine.SetProgress(85);

createBumps(avoidClasses(clPlayer, 20));

createMines(
	[
		[new SimpleObject(oMetalLarge, 1, 1, 0, 4)]
	],
	[avoidClasses(clForest, 1, clPlayer, 40, clRock, 20), stayClasses(clLand, 4)],
	clMetal);

createMines(
	[
		[new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)]
	],
	[avoidClasses(clForest, 1, clPlayer, 40, clMetal, 20), stayClasses(clLand, 4)],
	clRock);

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 [avoidClasses(clPlayer, 10, clForest, 20, clBaseResource, 5, clRock, 6, clMetal, 6), stayClasses(clLand, 3)],
 clForest,
 forestTrees);

g_Map.log("Creating hills");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	[avoidClasses(clBaseResource, 20, clHill, 15, clRock, 6, clMetal, 6), stayClasses(clLand, 0)],
	scaleByMapSize(4, 13)
);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, 0.8, 3));

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[
		avoidClasses(clForest, 10, clPlayer, 20, clMetal, 6, clRock, 6, clHill, 1),
		stayClasses(clLand, 4)
	],
	clForest,
	stragglerTrees);

createFood(
	[
		[new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)],
		[new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)]
	],
	[3 * numPlayers, 3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clRock, 6, clMetal, 6), stayClasses(clLand, 2)],
	clFood);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 15, clHill, 1, clFood, 4, clRock, 6, clMetal, 6), stayClasses(clLand, 2)],
	clFood);

if (currentBiome() == "generic/desert")
{
	g_Map.log("Creating obelisks");
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

g_Map.log("Creating dirt patches");
let numb = currentBiome() == "generic/savanna" ? 3 : 1;
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], [1, 1]),
			new TileClassPainter(clDirt)
		],
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 4)],
		numb*scaleByMapSize(15, 45));

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		new TerrainPainter(tTier4Terrain),
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 4)],
		numb * scaleByMapSize(15, 45));

g_Map.log("Creating small decorative rocks");
let group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clHill, 0), stayClasses(clLand, 2)],
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating large decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clHill, 0), stayClasses(clLand, 2)],
	scaleByMapSize(8, 131), 50
);

g_Map.log("Creating fish");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2, 3, 0, 2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clLand, 4, clFood, 20),
	25 * numPlayers, 60
);

g_Map.log("Creating Whales");
group = new SimpleGroup(
	[new SimpleObject(oWhale, 1, 1, 0, 3)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clLand, 4),avoidClasses(clFood, 8)],
	scaleByMapSize(5, 20), 100
);

g_Map.log("Creating shipwrecks");
group = new SimpleGroup(
	[new SimpleObject(oShipwreck, 1, 1, 0, 1)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clLand, 4),avoidClasses(clFood, 8)],
	scaleByMapSize(12, 16), 100
);

g_Map.log("Creating shipwreck debris");
group = new SimpleGroup(
	[new SimpleObject(oShipDebris, 1, 1, 0, 1)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clLand, 4),avoidClasses(clFood, 8)],
	scaleByMapSize(10, 20), 100
);

g_Map.log("Creating small grass tufts");
let planetm = currentBiome() == "generic/tropic" ? 8 : 1;
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1, 2, 0, 1, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(95);

g_Map.log("Creating large grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2, 4, 0, 1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3, 6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 5)],
	planetm * scaleByMapSize(13, 200)
);

paintTerrainBasedOnHeight(1, 2, 0, tShore);
paintTerrainBasedOnHeight(heightSeaGround, 1, 3, tWater);

placePlayersNomad(clPlayer, [stayClasses(clLand, 4), avoidClasses(clHill, 2, clForest, 1, clMetal, 4, clRock, 4, clFood, 2)]);

setSkySet(pickRandom(["cloudless", "cumulus", "overcast"]));
setSunRotation(randomAngle());
setSunElevation(randFloat(1/5, 1/3) * Math.PI);
setWaterWaviness(2);

Engine.SetProgress(100);

g_Map.ExportMap();
