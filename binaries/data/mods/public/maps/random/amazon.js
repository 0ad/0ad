// Coordinates: 0.138050, -50.466245
// Map Width: 200km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing tile classes...");
setBiome("tropic");
initTileClasses();

log("Initializing environment...");
setSunColor(0.733, 0.746, 0.574);

setWaterTint(0.576, 0.541, 0.322);
setWaterColor(0.521, 0.475, 0.322);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("lake");

setTerrainAmbientColor(0.521, 0.475, 0.322);

setSunRotation(-1 * PI);
setSunElevation(PI / 6.25);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

log("Initializing biome...");
g_Terrains.mainTerrain = "tropic_dirt_a_plants";
g_Terrains.forestFloor1 = "tropic_grass_c";
g_Terrains.forestFloor2 = "tropic_grass_c";
g_Terrains.tier1Terrain = "tropic_dirt_a";
g_Terrains.tier2Terrain = "tropic_plants";
g_Terrains.tier3Terrain = "tropic_grass_plants";
g_Terrains.tier4Terrain = "tropic_dirt_a_plants";
g_Terrains.roadWild = "road_rome_a";
g_Terrains.road = "road_stones";
g_Gaia.mainHuntableAnimal = "gaia/fauna_peacock";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_crocodile";
g_Gaia.fish = "gaia/fauna_fish_tilapia";
g_Gaia.tree1 = "gaia/flora_tree_palm_tropical";
g_Gaia.tree2 = "gaia/flora_tree_date_palm";
g_Gaia.tree3 = "gaia/flora_tree_date_palm";
g_Gaia.tree4 = "gaia/flora_tree_palm_tropical";
g_Gaia.tree5 = "gaia/flora_tree_date_palm";
g_Gaia.fruitBush = "gaia/flora_bush_berry";
g_Decoratives.grass = "actor|props/flora/grass_tropical.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_soft_tuft_a.xml";
g_Decoratives.rockLarge = "actor|geology/stone_savanna_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_savanna_med.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_tropic_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_tropic_b.xml";
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, getMapBaseHeight());
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("amazon");
RMS.SetProgress(30);

log("Paint tile classes...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");
//Coordinate system of the heightmap
var singleBases = [
	[90, 115],
	[240, 157],
	[35, 155],
	[140, 25],
	[260, 75],
	[160, 285],
	[105, 220],
	[185, 90]
];
var strongholdBases = [
	[80, 240],
	[190, 60]
];
randomPlayerPlacementAt(getTeamsArray(), singleBases, strongholdBases, scale, 0.06);
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
		"amounts": ["many"]
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
		"amounts": ["many"]
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
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
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
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["tons"]
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
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal", "many"]
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
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 12,
			g_TileClasses.player, 8
		],
		"stay": [g_TileClasses.water, 4],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
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
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
RMS.SetProgress(80);

log("Adding lillies...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(g_Decoratives.lillies, 5, 12, 1, 4),
			new SimpleObject(g_Decoratives.rockMedium, 1, 2, 1, 5)
		],
		true,
		g_TileClasses.dirt
	),
	0,
	[
		stayClasses(g_TileClasses.water, 1),
		borderClasses(g_TileClasses.water, scaleByMapSize(2,8), scaleByMapSize(2,5))
	],
	scaleByMapSize(100, 5000),
	500
);
RMS.SetProgress(90);

ExportMap();
