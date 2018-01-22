Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

var heightSeaGround = -18;
var heightLand = 2;
var heightOffsetHarbor = -11;

InitMap(heightLand, g_Terrains.mainTerrain);

initTileClasses();

setFogFactor(0.04);

createArea(
	new MapBoundsPlacer(),
	paintClass(g_TileClasses.land));

Engine.SetProgress(10);

const mapSize = getMapSize();
const mapCenter = getMapCenter();

const startAngle = randomAngle();
const players = addBases("radial", fractionToTiles(0.38), fractionToTiles(0.05), startAngle);
Engine.SetProgress(20);

addCenterLake();
Engine.SetProgress(30);

if (mapSize >= 192)
{
	addHarbors(players);
	Engine.SetProgress(40);
}

addSpines();
Engine.SetProgress(50);

addElements([
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 12,
			g_TileClasses.hill, 8,
			g_TileClasses.mountain, 8,
			g_TileClasses.player, 8,
			g_TileClasses.spine, 4
		],
		"stay": [g_TileClasses.water, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["many"]
	}
]);

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
		"baseHeight": heightLand,
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
Engine.SetProgress(60);

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

Engine.SetProgress(70);

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

Engine.SetProgress(80);

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

Engine.SetProgress(90);

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

ExportMap();

function addCenterLake()
{
	createArea(
		new ChainPlacer(
			2,
			Math.floor(scaleByMapSize(2, 12)),
			Math.floor(scaleByMapSize(35, 160)),
			1,
			mapCenter.x,
			mapCenter.y,
			0,
			[Math.floor(fractionToTiles(0.2))]),
		[
			new LayeredPainter(
				[
					g_Terrains.shore,
					g_Terrains.water,
					g_Terrains.water
				],
				[1, 100]
			),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 10),
			paintClass(g_TileClasses.water)
		],
		avoidClasses(g_TileClasses.player, 20)
	);

	let fDist = 50;
	if (mapSize <= 192)
		fDist = 20;
}

function addHarbors(players)
{
	for (let player of players)
	{
		let harborPosition = Vector2D.add(player.position, Vector2D.sub(mapCenter, player.position).div(2.5).round());
		createArea(
			new ClumpPlacer(1200, 0.5, 0.5, 1, harborPosition.x, harborPosition.y),
			[
				new LayeredPainter([g_Terrains.shore, g_Terrains.water], [2]),
				new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetHarbor, 3),
				paintClass(g_TileClasses.water)
			],
			avoidClasses(
				g_TileClasses.player, 15,
				g_TileClasses.hill, 1
			)
		);
	}
}

function addSpines()
{
	let smallSpines = mapSize <= 192;
	let spineSize = smallSpines ? 0.02 : 0.5;
	let spineTapering = smallSpines ?-0.1 :  -1.4;
	let heightOffsetSpine = smallSpines ? 20 : 35;

	let numPlayers = getNumPlayers();
	let spineTile = g_Terrains.dirt;

	if (currentBiome() == "snowy")
		spineTile = g_Terrains.tier1Terrain;

	if (currentBiome() == "alpine" || currentBiome() == "savanna")
		spineTile = g_Terrains.tier2Terrain;

	if (currentBiome() == "autumn")
		spineTile = g_Terrains.tier4Terrain;

	let split = 1;
	if (numPlayers <= 3 || mapSize >= 320 && numPlayers <= 4)
		split = 2;

	for (let i = 0; i < numPlayers * split; ++i)
	{
		let tang = startAngle + (i + 0.5) * 2 * Math.PI / (numPlayers * split);
		let start = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.12), 0).rotate(-tang));
		let end = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.4), 0).rotate(-tang));

		createArea(
			new PathPlacer(start, end, scaleByMapSize(14, spineSize), 0.6, 0.1, 0.4, spineTapering),
			[
				new LayeredPainter([g_Terrains.cliff, spineTile], [3]),
				new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetSpine, 3),
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
