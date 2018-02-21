Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

var heightLand = 2;

var g_Map = new RandomMap(heightLand, g_Terrains.mainTerrain);
var mapCenter = g_Map.getCenter();
var mapSize = g_Map.getSize();

initTileClasses(["bluffsPassage", "nomadArea"]);
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.land));

Engine.SetProgress(10);

var playerIDs;
var playerPosition;
if (!isNomad())
{
	let playerbasePattern = randomStartingPositionPattern(getTeamsArray());
	[playerIDs, playerPosition] = createBasesByPattern(playerbasePattern.setup, playerbasePattern.distance, playerbasePattern.groupedDistance, randomAngle());
	markPlayerAvoidanceArea(playerPosition, defaultPlayerBaseRadius());
}
Engine.SetProgress(20);

addElements([
	{
		"func": addBluffs,
		"baseHeight": heightLand,
		"avoid": [g_TileClasses.bluffIgnore, 0],
		"sizes": ["normal", "big", "huge"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addHills,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.hill, 15,
			g_TileClasses.player, 20
		],
		"sizes": ["normal", "big"],
		"mixes": ["normal"],
		"amounts": ["tons"]
	}
]);
Engine.SetProgress(30);

if (!isNomad())
	createBluffsPassages(playerPosition);

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.bluff, 2,
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
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
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	}
]);
Engine.SetProgress(50);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.bluff, 5],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.bluff, 5],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	// Forests on bluffs
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.forest, 6,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"stay": [g_TileClasses.bluff, 5],
		"sizes": ["big"],
		"mixes": ["normal"],
		"amounts": ["tons"]
	},
	// Forests on mainland
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.bluff, 10,
			g_TileClasses.forest, 10,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"sizes": ["small"],
		"mixes": ["same"],
		"amounts": ["normal"]
	}
]));
Engine.SetProgress(70);

addElements(shuffleArray([
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3
		],
		"sizes": ["small"],
		"mixes": ["similar"],
		"amounts": ["few"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.bluffsPassage, 4,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"sizes": ["tiny"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
Engine.SetProgress(90);

if (isNomad())
{
	g_Map.log("Preventing units to be spawned at the map border");
	createArea(
		new DiskPlacer(mapSize / 2 - scaleByMapSize(15, 35), mapCenter),
		new TileClassPainter(g_TileClasses.nomadArea));

	placePlayersNomad(
		g_TileClasses.player,
		[
			stayClasses(g_TileClasses.nomadArea, 0),
			avoidClasses(
				g_TileClasses.bluff, 2,
				g_TileClasses.water, 4,
				g_TileClasses.forest, 1,
				g_TileClasses.metal, 4,
				g_TileClasses.rock, 4,
				g_TileClasses.mountain, 4,
				g_TileClasses.animals, 2)
		]);
}

g_Map.ExportMap();
