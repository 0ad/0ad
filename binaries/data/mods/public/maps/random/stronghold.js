Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

const heightLand = 30;

var g_Map = new RandomMap(heightLand, g_Terrains.mainTerrain);

initTileClasses();

createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.land));

Engine.SetProgress(20);

const [playerIDs, playerPosition] = createBasesByPattern("stronghold", fractionToTiles(randFloat(0.2, 0.35)), fractionToTiles(randFloat(0.05, 0.1)), randomAngle());
markPlayerAvoidanceArea(playerPosition, defaultPlayerBaseRadius());

Engine.SetProgress(30);

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
			g_TileClasses.valley, 5,
			g_TileClasses.water, 7
		],
		"sizes": ["big", "huge"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addHills,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.hill, 15,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 20,
			g_TileClasses.valley, 2,
			g_TileClasses.water, 2
		],
		"sizes": ["normal", "big"],
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
			g_TileClasses.valley, 10,
			g_TileClasses.water, 15
		],
		"sizes": ["big", "huge"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addPlateaus,
		"avoid": [
			g_TileClasses.bluff, 20,
			g_TileClasses.mountain, 25,
			g_TileClasses.plateau, 25,
			g_TileClasses.player, 40,
			g_TileClasses.valley, 10,
			g_TileClasses.water, 15
		],
		"sizes": ["big", "huge"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addValleys,
		"baseHeight": heightLand,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.hill, 5,
			g_TileClasses.mountain, 25,
			g_TileClasses.plateau, 10,
			g_TileClasses.player, 40,
			g_TileClasses.valley, 15,
			g_TileClasses.water, 10
		],
		"sizes": ["normal", "big"],
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	}
]));

if (!isNomad())
	createBluffsPassages(playerPosition);

Engine.SetProgress(60);

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
			g_TileClasses.valley, 5,
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
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	}
]);
Engine.SetProgress(70);

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
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries,
			5, g_TileClasses.bluff,
			5, g_TileClasses.forest,
			18, g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["few", "normal", "many", "tons"]
	}
]));
Engine.SetProgress(80);

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
Engine.SetProgress(90);

placePlayersNomad(
	g_TileClasses.player,
	avoidClasses(
		g_TileClasses.bluff, 4,
		g_TileClasses.plateau, 4,
		g_TileClasses.forest, 1,
		g_TileClasses.metal, 4,
		g_TileClasses.rock, 4,
		g_TileClasses.mountain, 4,
		g_TileClasses.animals, 2));

g_Map.ExportMap();
