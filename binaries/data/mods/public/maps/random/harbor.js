RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

setSelectedBiome();
initMapSettings();
initTileClasses();

setFogFactor(0.04);

resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 2);
RMS.SetProgress(10);

var players = addBases("radial", 0.38);
RMS.SetProgress(20);

addCenterLake();
RMS.SetProgress(30);

if (g_MapInfo.mapSize >= 192)
{
	addHarbors(players);
	RMS.SetProgress(40);
}

addSpines();
RMS.SetProgress(50);

addElements(shuffleArray([
	{
		"func": addHills,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.hill, 15,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.spine, 5,
			g_TileClasses.valley, 2,
			g_TileClasses.water, 2
		],
		"sizes": ["tiny", "small"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addMountains,
		"avoid": [
			g_TileClasses.bluff, 20,
			g_TileClasses.mountain, 25,
			g_TileClasses.plateau, 20,
			g_TileClasses.player, 20,
			g_TileClasses.spine, 20,
			g_TileClasses.valley, 10,
			g_TileClasses.water, 15
		],
		"sizes": ["small"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addPlateaus,
		"avoid": [
			g_TileClasses.bluff, 20,
			g_TileClasses.mountain, 25,
			g_TileClasses.plateau, 20,
			g_TileClasses.player, 40,
			g_TileClasses.spine, 20,
			g_TileClasses.valley, 10,
			g_TileClasses.water, 15
		],
		"sizes": ["small"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addBluffs,
		"avoid": [
			g_TileClasses.bluff, 20,
			g_TileClasses.mountain, 25,
			g_TileClasses.plateau, 20,
			g_TileClasses.player, 40,
			g_TileClasses.spine, 20,
			g_TileClasses.valley, 10,
			g_TileClasses.water, 15
		],
		"sizes": ["normal"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	}
]));
RMS.SetProgress(60);

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
			g_TileClasses.spine, 5,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal", "many"]
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
			g_TileClasses.spine, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal", "many"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.spine, 5,
			g_TileClasses.water, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["many"]
	}
]));

RMS.SetProgress(70);

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

RMS.SetProgress(80);

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

RMS.SetProgress(90);

ExportMap();

function addCenterLake()
{
	let lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

	createArea(
		new ChainPlacer(
			2,
			Math.floor(scaleByMapSize(2, 12)),
			Math.floor(scaleByMapSize(35, 160)),
			1,
			g_MapInfo.centerOfMap,
			g_MapInfo.centerOfMap,
			0,
			[floor(g_MapInfo.mapSize * 0.17 * lSize)]
		),
		[
			new LayeredPainter(
				[
					g_Terrains.shore,
					g_Terrains.water,
					g_Terrains.water
				],
				[1, 100]
			),
			new SmoothElevationPainter(ELEVATION_SET, -18, 10),
			paintClass(g_TileClasses.water)
		],
		avoidClasses(g_TileClasses.player, 20)
	);

	let fDist = 50;
	if (g_MapInfo.mapSize <= 192)
		fDist = 20;

	// create a bunch of fish
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(g_Gaia.fish, 20, 30, 0, fDist)],
			true,
			g_TileClasses.baseResource,
			g_MapInfo.centerOfMap,
			g_MapInfo.centerOfMap
		),
		0,
		[
			avoidClasses(g_TileClasses.player, 5, g_TileClasses.hill, 3, g_TileClasses.mountain, 3),
			stayClasses(g_TileClasses.water, 5)
		]
	);
}

function addHarbors(players)
{
	for (let i = 0; i < players.length; ++i)
	{
		let ix = round(fractionToTiles(players[i].x));
		let iz = round(fractionToTiles(players[i].z));
		let playerDistX = g_MapInfo.centerOfMap - ix;
		let playerDistZ = g_MapInfo.centerOfMap - iz;
		let offsetX = round(playerDistX / 2.5);
		let offsetZ = round(playerDistZ / 2.5);

		createArea(
			new ClumpPlacer(scaleByMapSize(1200, 1200), 0.5, 0.5, 1, ix + offsetX, iz + offsetZ),
			[
				new LayeredPainter([g_Terrains.shore, g_Terrains.water], [2]),
				new SmoothElevationPainter(ELEVATION_MODIFY, -11, 3),
				paintClass(g_TileClasses.water)
			],
			avoidClasses(
				g_TileClasses.player, 15,
				g_TileClasses.hill, 1
			)
		);

		// create fish in harbor
		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(g_Gaia.fish, 6, 6, 1, 20)],
				true, g_TileClasses.baseResource, ix + offsetX, iz + offsetZ
			),
			0,
			[
				avoidClasses(
					g_TileClasses.hill, 3,
					g_TileClasses.mountain, 3
				),
				stayClasses(g_TileClasses.water, 5)
			]
		);
	}
}

function addSpines()
{
	let spineTile = g_Terrains.dirt;
	let elevation = 35;

	if (currentBiome() == "snowy")
		spineTile = g_Terrains.tier1Terrain;

	if (currentBiome() == "alpine" || currentBiome() == "savanna")
		spineTile = g_Terrains.tier2Terrain;

	if (currentBiome() == "autumn")
		spineTile = g_Terrains.tier4Terrain;

	let split = 1;
	if (g_MapInfo.numPlayers <= 3 || (g_MapInfo.mapSize >= 320 && g_MapInfo.numPlayers <= 4))
		split = 2;

	for (let i = 0; i < g_MapInfo.numPlayers * split; ++i)
	{
		let tang = g_MapInfo.startAngle + (i + 0.5) * TWO_PI / (g_MapInfo.numPlayers * split);

		let fx = fractionToTiles(0.5);
		let fz = fractionToTiles(0.5);
		let ix = round(fx);
		let iz = round(fz);

		let mStartCo = 0.12;
		let mStopCo = 0.40;
		let mSize = 0.5;
		let mWaviness = 0.6;
		let mOffset = 0.4;
		let mTaper = -1.4;

		// make small mountain dividers if we're on a small map
		if (g_MapInfo.mapSize <= 192)
		{
			mSize = 0.02;
			mTaper = -0.1;
			elevation = 20;
		}

		createArea(
			new PathPlacer(
				fractionToTiles(0.5 + mStartCo * cos(tang)),
				fractionToTiles(0.5 + mStartCo * sin(tang)),
				fractionToTiles(0.5 + mStopCo * cos(tang)),
				fractionToTiles(0.5 + mStopCo * sin(tang)),
				scaleByMapSize(14, mSize),
				mWaviness,
				0.1,
				mOffset,
				mTaper
			),
			[
				new LayeredPainter([g_Terrains.cliff, spineTile], [3]),
				new SmoothElevationPainter(ELEVATION_MODIFY, elevation, 3),
				paintClass(g_TileClasses.spine)
			],
			avoidClasses(g_TileClasses.player, 5)
		);
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
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["normal"]
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
