// Coordinates: -3.177437, 35.574687
// Map Width: 120km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

setBiome("savanna");
initForestFloor();
initTileClasses(["eden", "highlands"]);

log("Initializing environment...");

setSunColor(0.733, 0.746, 0.574);

setWaterHeight(18);
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
g_Terrains.mainTerrain = "savanna_riparian_bank";
g_Terrains.forestFloor1 = "savanna_dirt_rocks_b";
g_Terrains.forestFloor2 = "savanna_dirt_rocks_c";
g_Terrains.tier1Terrain = "savanna_dirt_rocks_a";
g_Terrains.tier2Terrain = "savanna_grass_a";
g_Terrains.tier3Terrain = "savanna_grass_b";
g_Terrains.tier4Terrain = "savanna_forest_floor_a";
g_Terrains.roadWild = "savanna_riparian_dry";
g_Terrains.road = "road2";
g_Gaia.mainHuntableAnimal = "gaia/fauna_giraffe";
g_Gaia.secondaryHuntableAnimal = "gaia/fauna_zebra";
g_Gaia.metalLarge = "gaia/geology_metal_savanna_slabs";
g_Gaia.metalSmall = "gaia/geology_metal_tropic";
g_Gaia.fish = "gaia/fauna_fish_tilapia";
g_Gaia.tree1 = "gaia/flora_tree_baobab";
g_Gaia.tree2 = "gaia/flora_tree_baobab";
g_Gaia.tree3 = "gaia/flora_tree_baobab";
g_Gaia.tree4 = "gaia/flora_tree_baobab";
g_Gaia.tree5 = "gaia/flora_tree_baobab";
g_Decoratives.grass = "actor|props/flora/grass_savanna.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_soft_dry_tuft_a.xml";
g_Decoratives.rockLarge = "actor|geology/stone_savanna_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_savanna_med.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_desert_dry_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_dry_a.xml";
initForestFloor();
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, getMapBaseHeight());
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("ngorongoro", (tile, x, y) => {

	if (tile.indexOf("grass1") >= 0 || tile.indexOf("savanna_wash") >= 0)
		addToClass(x, y, g_TileClasses.mountain);

	if (tile.indexOf("new_savanna") >= 0)
		addToClass(x, y, g_TileClasses.eden);
	else if (tile.indexOf("savanna_shrubs") >= 0 || tile.indexOf("savanna_grass") >= 0 || tile.indexOf("peat_temp") >= 0 || tile.indexOf("grass_field") >= 0 || tile.indexOf("grass b") >= 0)
		addToClass(x, y, g_TileClasses.highlands);
});
RMS.SetProgress(30);

log("Paint tile classes...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");
//Coordinate system of the heightmap
var singleBases = [
	[75, 275],
	[35, 210],
	[150, 285],
	[100, 35],
	[200, 35],
	[260, 110],
	[230,260],
	[45,135]
];

var strongholdBases = [
	[80, 250],
	[205, 65]
];
randomPlayerPlacementAt(getTeamsArray(), singleBases, strongholdBases, scale, 0.06);
RMS.SetProgress(50);

log("Render lowlands...");
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	}
]);

addElements(shuffleArray([
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2

		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["scarce"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["big"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["unique"],
		"amounts": ["tons"]
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
		"amounts": ["tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 4,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["big"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
RMS.SetProgress(60);

g_Gaia.mainHuntableAnimal = "gaia/fauna_lioness";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_lion";
g_Terrains.mainTerrain = "savanna_grass_a_wetseason";
g_Terrains.forestFloor1 = "savanna_grass_a";
g_Terrains.forestFloor2 = "savanna_grass_b";
g_Terrains.tier1Terrain = "savanna_grass_a_wetseason";
g_Terrains.tier2Terrain = "savanna_grass_b_wetseason";
g_Terrains.tier3Terrain = "savanna_shrubs_a_wetseason";
g_Terrains.tier4Terrain = "savanna_shrubs_b";

log("Render highlands...");
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	}
]);

addElements(shuffleArray([
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(70);

g_Gaia.mainHuntableAnimal = "gaia/fauna_rhino";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_elephant_african_bush";
initForestFloor();

log("Render eden...");
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	}
]);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 8,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["scarce"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 2,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 8,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["scarce"]
	}
]));
RMS.SetProgress(80);

ExportMap();
