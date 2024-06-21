Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

function* GenerateMap()
{
	setSelectedBiome();

	const heightLand = 1;
	const heightBarrier = 30;

	globalThis.g_Map = new RandomMap(heightLand, g_Terrains.mainTerrain);

	initTileClasses();

	const mapCenter = g_Map.getCenter();

	createArea(
		new MapBoundsPlacer(),
		new TileClassPainter(g_TileClasses.land));

	yield 10;

	const teamsArray = getTeamsArray();
	const startAngle = randomAngle();
	createBases(
		...playerPlacementByPattern(
			"line",
			fractionToTiles(0.2),
			fractionToTiles(0.08),
			startAngle,
			undefined),
		false);
	yield 20;

	placeBarriers();
	yield 40;

	addElements(shuffleArray([
		{
			"func": addBluffs,
			"baseHeight": heightLand,
			"avoid": [
				g_TileClasses.bluff, 20,
				g_TileClasses.hill, 5,
				g_TileClasses.mountain, 20,
				g_TileClasses.plateau, 20,
				g_TileClasses.player, 30,
				g_TileClasses.spine, 15,
				g_TileClasses.valley, 5,
				g_TileClasses.water, 7
			],
			"sizes": ["normal", "big"],
			"mixes": ["varied"],
			"amounts": ["few"]
		},
		{
			"func": addHills,
			"avoid": [
				g_TileClasses.bluff, 5,
				g_TileClasses.hill, 15,
				g_TileClasses.mountain, 2,
				g_TileClasses.plateau, 5,
				g_TileClasses.player, 20,
				g_TileClasses.spine, 15,
				g_TileClasses.valley, 2,
				g_TileClasses.water, 2
			],
			"sizes": ["normal", "big"],
			"mixes": ["varied"],
			"amounts": ["few"]
		},
		{
			"func": addLakes,
			"avoid": [
				g_TileClasses.bluff, 7,
				g_TileClasses.hill, 2,
				g_TileClasses.mountain, 15,
				g_TileClasses.plateau, 10,
				g_TileClasses.player, 20,
				g_TileClasses.spine, 15,
				g_TileClasses.valley, 10,
				g_TileClasses.water, 25
			],
			"sizes": ["big", "huge"],
			"mixes": ["varied", "unique"],
			"amounts": ["few"]
		}
	]));
	yield 50;

	addElements([
		{
			"func": addLayeredPatches,
			"avoid": [
				g_TileClasses.bluff, 2,
				g_TileClasses.dirt, 5,
				g_TileClasses.forest, 2,
				g_TileClasses.mountain, 2,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 12,
				g_TileClasses.spine, 5,
				g_TileClasses.water, 3
			],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["normal"]
		},
		{
			"func": addDecoration,
			"avoid": [
				g_TileClasses.bluff, 2,
				g_TileClasses.forest, 2,
				g_TileClasses.mountain, 2,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 12,
				g_TileClasses.spine, 5,
				g_TileClasses.water, 3
			],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["normal"]
		}
	]);
	yield 60;

	addElements(shuffleArray([
		{
			"func": addMetal,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.bluff, 5,
				g_TileClasses.forest, 3,
				g_TileClasses.mountain, 2,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 30,
				g_TileClasses.rock, 10,
				g_TileClasses.metal, 20,
				g_TileClasses.spine, 5,
				g_TileClasses.water, 3
			],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": g_AllAmounts
		},
		{
			"func": addStone,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.bluff, 5,
				g_TileClasses.forest, 3,
				g_TileClasses.mountain, 2,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 30,
				g_TileClasses.rock, 20,
				g_TileClasses.metal, 10,
				g_TileClasses.spine, 5,
				g_TileClasses.water, 3
			],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": g_AllAmounts
		},
		{
			"func": addForests,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.bluff, 5,
				g_TileClasses.forest, 18,
				g_TileClasses.metal, 3,
				g_TileClasses.mountain, 5,
				g_TileClasses.plateau, 5,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 3,
				g_TileClasses.spine, 5,
				g_TileClasses.water, 2
			],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": ["few", "normal", "many", "tons"]
		}
	]));
	yield 80;

	addElements(shuffleArray([
		{
			"func": addBerries,
			"avoid": [
				g_TileClasses.berries, 30,
				g_TileClasses.bluff, 5,
				g_TileClasses.forest, 5,
				g_TileClasses.metal, 10,
				g_TileClasses.mountain, 2,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 10,
				g_TileClasses.spine, 2,
				g_TileClasses.water, 3
			],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": g_AllAmounts
		},
		{
			"func": addAnimals,
			"avoid": [
				g_TileClasses.animals, 20,
				g_TileClasses.bluff, 5,
				g_TileClasses.forest, 2,
				g_TileClasses.metal, 2,
				g_TileClasses.mountain, 1,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 2,
				g_TileClasses.spine, 2,
				g_TileClasses.water, 3
			],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": g_AllAmounts
		},
		{
			"func": addStragglerTrees,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.bluff, 5,
				g_TileClasses.forest, 7,
				g_TileClasses.metal, 2,
				g_TileClasses.mountain, 1,
				g_TileClasses.plateau, 2,
				g_TileClasses.player, 12,
				g_TileClasses.rock, 2,
				g_TileClasses.spine, 2,
				g_TileClasses.water, 5
			],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": g_AllAmounts
		}
	]));
	yield 90;

	placePlayersNomad(
		g_TileClasses.player,
		avoidClasses(
			g_TileClasses.bluff, 4,
			g_TileClasses.water, 4,
			g_TileClasses.spine, 4,
			g_TileClasses.plateau, 4,
			g_TileClasses.forest, 1,
			g_TileClasses.metal, 4,
			g_TileClasses.rock, 4,
			g_TileClasses.mountain, 4,
			g_TileClasses.animals, 2));

	return g_Map;

	function placeBarriers()
	{
		let spineTerrain = g_Terrains.dirt;

		if (currentBiome() == "generic/arctic")
			spineTerrain = g_Terrains.tier1Terrain;

		if (currentBiome() == "generic/alpine" || currentBiome() == "generic/savanna")
			spineTerrain = g_Terrains.tier2Terrain;

		if (currentBiome() == "generic/autumn")
			spineTerrain = g_Terrains.tier4Terrain;

		const spineCount = isNomad() ? randIntInclusive(1, 4) : teamsArray.length;

		for (let i = 0; i < spineCount; ++i)
		{
			let mSize = 8;
			let mWaviness = 0.6;
			let mOffset = 0.5;
			let mTaper = -1.5;

			if (spineCount > 3 || g_Map.getSize() <= 192)
			{
				mWaviness = 0.2;
				mOffset = 0.2;
				mTaper = -1;
			}

			if (spineCount >= 5)
			{
				mSize = 4;
				mWaviness = 0.2;
				mOffset = 0.2;
				mTaper = -0.7;
			}

			const angle = startAngle + (i + 0.5) * 2 * Math.PI / spineCount;
			const start =
				Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.075), 0).rotate(-angle));
			const end =
				Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.42), 0).rotate(-angle));
			createArea(
				new PathPlacer(start, end, scaleByMapSize(14, mSize), mWaviness, 0.1, mOffset,
					mTaper),
				[
					new LayeredPainter([g_Terrains.cliff, spineTerrain], [2]),
					new SmoothElevationPainter(ELEVATION_SET, heightBarrier, 2),
					new TileClassPainter(g_TileClasses.spine)
				],
				avoidClasses(g_TileClasses.player, 5, g_TileClasses.baseResource, 5));
		}

		addElements([
			{
				"func": addDecoration,
				"avoid": [
					g_TileClasses.bluff, 2,
					g_TileClasses.forest, 2,
					g_TileClasses.mountain, 2,
					g_TileClasses.player, 12,
					g_TileClasses.water, 3
				],
				"stay": [g_TileClasses.spine, 5],
				"sizes": ["huge"],
				"mixes": ["unique"],
				"amounts": ["tons"]
			}
		]);

		addElements([
			{
				"func": addProps,
				"avoid": [
					g_TileClasses.forest, 2,
					g_TileClasses.player, 2,
					g_TileClasses.prop, 20,
					g_TileClasses.water, 3
				],
				"stay": [g_TileClasses.spine, 8],
				"sizes": ["normal"],
				"mixes": ["normal"],
				"amounts": ["scarce"]
			}
		]);
	}
}
