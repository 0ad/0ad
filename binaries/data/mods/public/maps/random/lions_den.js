Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

const topTerrain = g_Terrains.tier2Terrain;

const heightValley = 0;
const heightPath = 10;
const heightDen = 15;
const heightHill = 50;

var g_Map = new RandomMap(heightHill, topTerrain);

const mapCenter = getMapCenter();
const numPlayers = getNumPlayers();
const startAngle = randomAngle();

initTileClasses(["step"]);
createArea(
	new MapBoundsPlacer(),
	paintClass(g_TileClasses.land));

Engine.SetProgress(10);

addBases("radial", fractionToTiles(0.4), fractionToTiles(randFloat(0.05, 0.1)), startAngle);
Engine.SetProgress(20);

createSunkenTerrain();
Engine.SetProgress(30);

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2
		],
		"stay": [g_TileClasses.player, 1],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.forest, 2
		],
		"stay": [g_TileClasses.player, 1],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.step, 2
		 ],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.step, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["scarce"]
	}
]);
Engine.SetProgress(40);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.mountain, 5,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 5,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal", "big"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 3,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 5,
			g_TileClasses.rock, 3,
			g_TileClasses.step, 1
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal", "big"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
Engine.SetProgress(60);

addElements(shuffleArray([
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
	},
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 10,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.baseResource, 5,
			g_TileClasses.forest, 0,
			g_TileClasses.metal, 1,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 1
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.baseResource, 5,
			g_TileClasses.forest, 0,
			g_TileClasses.metal, 1,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 1,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 3
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 3,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["normal", "many", "tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.player, 10,
			g_TileClasses.baseResource, 5,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.metal, 5,
			g_TileClasses.rock, 5
		],
		"stay": [g_TileClasses.player, 1],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
Engine.SetProgress(75);

addElements([
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.valley, 4,
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.land, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["tons"]
	}
]);
Engine.SetProgress(80);

addElements([
	{
		"func": addProps,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.valley, 4,
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.land, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["scarce"]
	}
]);
Engine.SetProgress(85);

addElements([
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.mountain, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["tons"]
	}
]);
Engine.SetProgress(90);

addElements([
	{
		"func": addProps,
		"avoid": [
			g_TileClasses.baseResource, 5,
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.mountain, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["scarce"]
	}
]);
Engine.SetProgress(95);

placePlayersNomad(
	g_TileClasses.player,
	[
		new HeightConstraint(heightValley, heightPath),
		avoidClasses(
			g_TileClasses.forest, 1,
			g_TileClasses.metal, 4,
			g_TileClasses.rock, 4,
			g_TileClasses.animals, 2)
	]);

g_Map.ExportMap();

function createSunkenTerrain()
{
	var base = g_Terrains.mainTerrain;
	var middle = g_Terrains.dirt;
	var lower = g_Terrains.tier2Terrain;
	var road = g_Terrains.road;

	if (currentBiome() == "snowy")
	{
		middle = g_Terrains.tier2Terrain;
		lower = g_Terrains.tier1Terrain;
	}

	if (currentBiome() == "alpine")
	{
		middle = g_Terrains.shore;
		lower = g_Terrains.tier4Terrain;
	}

	if (currentBiome() == "mediterranean")
	{
		middle = g_Terrains.tier1Terrain;
		lower = g_Terrains.forestFloor1;
	}

	if (currentBiome() == "savanna")
	{
		middle = g_Terrains.tier2Terrain;
		lower = g_Terrains.tier4Terrain;
	}

	if (currentBiome() == "tropic" || currentBiome() == "autumn")
		road = g_Terrains.roadWild;

	if (currentBiome() == "autumn")
		middle = g_Terrains.shore;

	var expSize = diskArea(fractionToTiles(0.14)) / numPlayers;
	var expDist = 0.1 + numPlayers / 200;
	var expAngle = 0.75;

	if (numPlayers <= 2)
	{
		expSize = diskArea(fractionToTiles(0.075));
		expAngle = 0.72;
	}

	var nRoad = 0.44;
	var nExp = 0.425;

	if (numPlayers < 4)
	{
		nRoad = 0.42;
		nExp = 0.4;
	}

	log("Creating central valley...");
	createArea(
		new ClumpPlacer(diskArea(fractionToTiles(0.29)), 1, 1, 1, mapCenter),
		[
			new LayeredPainter([g_Terrains.cliff, lower], [3]),
			new SmoothElevationPainter(ELEVATION_SET, heightValley, 3),
			paintClass(g_TileClasses.valley)
		]);

	log("Creating central hill...");
	createArea(
		new ClumpPlacer(diskArea(fractionToTiles(0.21)), 1, 1, 1, mapCenter),
		[
			new LayeredPainter([g_Terrains.cliff, topTerrain], [3]),
			new SmoothElevationPainter(ELEVATION_SET, heightHill, 3),
			paintClass(g_TileClasses.mountain)
		]);

	let getCoords = (distance, playerID, playerIDOffset) => {
		let angle = startAngle + (playerID + playerIDOffset) * 2 * Math.PI / numPlayers;
		return Vector2D.add(mapCenter, new Vector2D(fractionToTiles(distance), 0).rotate(-angle)).round();
	};

	for (let i = 0; i < numPlayers; ++i)
	{
		let playerPosition = getCoords(0.4, i, 0);

		log("Creating path from player to expansion...");
		let expansionPosition = getCoords(expDist, i, expAngle);
		createArea(
			new PathPlacer(playerPosition, expansionPosition, 12, 0.7, 0.5, 0.1, -1),
			[
				new LayeredPainter([g_Terrains.cliff, middle, road], [3, 4]),
				new SmoothElevationPainter(ELEVATION_SET, heightPath, 3),
				paintClass(g_TileClasses.step)
			]);

		log("Creating path from player to the neighbor...");
		for (let neighborOffset of [-0.5, 0.5])
		{
			let neighborPosition = getCoords(nRoad, i, neighborOffset);
			let pathPosition = getCoords(0.47, i, 0);
			createArea(
				new PathPlacer(pathPosition, neighborPosition, 19, 0.4, 0.5, 0.1, -0.6),
				[
					new LayeredPainter([g_Terrains.cliff, middle, road], [3, 6]),
					new SmoothElevationPainter(ELEVATION_SET, heightPath, 3),
					paintClass(g_TileClasses.step)
				]);
		}

		log("Creating the den of the player...");
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.1)) / (isNomad() ? 2 : 1), 0.9, 0.3, 1, playerPosition),
			[
				new LayeredPainter([g_Terrains.cliff, base], [3]),
				new SmoothElevationPainter(ELEVATION_SET, heightDen, 3),
				paintClass(g_TileClasses.valley)
			]);

		log("Creating the expansion of the player...");
		createArea(
			new ClumpPlacer(expSize, 0.9, 0.3, 1, expansionPosition),
			[
				new LayeredPainter([g_Terrains.cliff, base], [3]),
				new SmoothElevationPainter(ELEVATION_SET, heightDen, 3),
				paintClass(g_TileClasses.settlement)
			],
			[avoidClasses(g_TileClasses.settlement, 2)]);
	}

	log("Creating the expansions between players after the paths were created...");
	for (let i = 0; i < numPlayers; ++i)
	{
		let position = getCoords(nExp, i, 0.5);
		createArea(
			new ClumpPlacer(expSize, 0.9, 0.3, 1, position),
			[
				new LayeredPainter([g_Terrains.cliff, lower], [3]),
				new SmoothElevationPainter(ELEVATION_SET, heightValley, 3),
				paintClass(g_TileClasses.settlement)
			]);
	}
}
