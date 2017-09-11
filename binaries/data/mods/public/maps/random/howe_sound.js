// Coordinates: 49.545673, -123.309144
// Map Width: 50km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing tile classes...");
setBiome("snowy");
initMapSettings();
initTileClasses(["island"]);

log("Initializing environment...");

setSunColor(0.733, 0.746, 0.574);
setSkySet("stratus");

setWaterTint(0.388, 0.650, 0.661);
setWaterColor(0.388, 0.650, 0.661);
setWaterWaviness(8);
setWaterMurkiness(0.8);
setWaterType("lake");

setTerrainAmbientColor(0.349, 0.514, 0.671);

setSunRotation(PI * -0.5);
setSunElevation(PI/9);

setFogFactor(0.08);
setFogThickness(0);
setFogColor(0.75, 0.75, 0.75);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

log("Initializing biome...");

g_Terrains.mainTerrain = "snow rough";
g_Terrains.forestFloor1 = "snow grass 2";
g_Terrains.forestFloor2 = "snow 50";
g_Terrains.tier1Terrain = "snow grass 100";
g_Terrains.tier2Terrain = "snow rocks";
g_Terrains.tier3Terrain = "snow rough";
g_Terrains.tier4Terrain = "snow grass 75";
g_Terrains.roadWild = "path a";
g_Terrains.road = "road_flat";
g_Gaia.mainHuntableAnimal = "gaia/fauna_muskox";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_wolf_snow";
g_Gaia.fish = "gaia/fauna_fish";
g_Gaia.tree1 = "gaia/flora_tree_pine_w";
g_Gaia.tree2 = "gaia/flora_tree_pine_w";
g_Gaia.tree3 = "gaia/flora_tree_pine_w";
g_Gaia.tree4 = "gaia/flora_tree_dead";
g_Gaia.tree5 = "gaia/flora_tree_pine_w";
g_Gaia.metalSmall = "gaia/geology_metal_temperate";
g_Gaia.fruitBush = "gaia/flora_bush_berry";
g_Decoratives.grass = "actor|props/flora/grass_soft_dry_tuft_a.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_field_bloom_short.xml";
g_Decoratives.rockLarge = "actor|props/special/eyecandy/standing_stones.xml";
g_Decoratives.rockMedium = "actor|geology/stone_granite_small.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_medit_me_dry.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_medit_sm_dry.xml";

initBiome();
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 1);
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("howe_sound", (tile, x, y) => {
	if (tile.indexOf("polar_ice_cracked") >= 0)
		addToClass(x, y, g_TileClasses.island);
});

log("Paint tile classes...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");
//Coordinate system of the heightmap
var singleBases = [
	[40, 85],
	[230, 40],
	[280, 110],
	[240, 180],
	[50, 170],
	[100, 240],
	[280, 280],
	[170, 280]
];
var strongholdBases = [
	[90, 210],
	[255, 120]
];
randomPlayerPlacementAt(singleBases, strongholdBases, scale, 0.06);
RMS.SetProgress(50);

log("Render mainland...");
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
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
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
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["few"]
	}
]);

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
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 30,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal", "many"]
	},
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 30,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
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
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
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
			g_TileClasses.forest, 15,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	}
]));

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
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
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
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
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
			g_TileClasses.water, 5,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
RMS.SetProgress(65);

g_Gaia.mainHuntableAnimal = "gaia/fauna_bear";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_wolf_snow";

log("Render islands...");
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
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
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
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 10,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));

addElements(shuffleArray([
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
		"stay": [g_TileClasses.island, 2],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["many"]
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
			g_TileClasses.water, 2
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["small"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(80);

log("Placing whale...");
g_Gaia.fish = "gaia/fauna_whale_fin";
addElements([
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 2,
		],
		"stay": [g_TileClasses.water, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	}
]);
RMS.SetProgress(90);

ExportMap();
