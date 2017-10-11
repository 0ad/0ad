RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

setSelectedBiome();
initTileClasses();

resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, getMapBaseHeight());
RMS.SetProgress(10);

const teamsArray = getTeamsArray();
const startAngle = randFloat(0, 2 * Math.PI);
addBases("stronghold", 0.37, 0.04, startAngle);
RMS.SetProgress(20);

// Change the starting angle and add the players again
var rotation = PI;

if (teamsArray.length == 2)
	rotation = PI / 2;

if (teamsArray.length == 4)
	rotation = PI + PI / 4;

addBases("stronghold", 0.15, 0.04, startAngle + rotation);
RMS.SetProgress(40);

addElements(shuffleArray([
	{
		"func": addHills,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.hill, 15,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.valley, 2,
			g_TileClasses.water, 2
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
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
		"sizes": ["huge"],
		"mixes": ["same", "similar"],
		"amounts": ["tons"]
	},
	{
		"func": addPlateaus,
		"avoid": [
			g_TileClasses.bluff, 20,
			g_TileClasses.mountain, 25,
			g_TileClasses.plateau, 20,
			g_TileClasses.player, 40,
			g_TileClasses.valley, 10,
			g_TileClasses.water, 15
		],
		"sizes": ["huge"],
		"mixes": ["same", "similar"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(50);

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
RMS.SetProgress(60);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.plateau, 2,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addStone,
		"avoid": [g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.plateau, 2,
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
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["few", "normal", "many", "tons"]
	}
]));
RMS.SetProgress(80);

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
			g_TileClasses.water, 5
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	}
]));
RMS.SetProgress(90);

ExportMap();
