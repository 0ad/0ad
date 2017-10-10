// Location: 49.337248, 1.106107
// Map Width: 80km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing tile classes...");
setBiome("alpine");
initTileClasses(["shallowWater"]);

log("Initializing environment...");

setSunColor(0.733, 0.746, 0.574);

setWaterTint(0.224, 0.271, 0.270);
setWaterColor(0.224, 0.271, 0.270);
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
g_Terrains.mainTerrain = "new_alpine_grass_d";
g_Terrains.forestFloor1 = "alpine_grass_d";
g_Terrains.forestFloor2 = "alpine_grass_c";
g_Terrains.tier1Terrain = "new_alpine_grass_c";
g_Terrains.tier2Terrain = "new_alpine_grass_b";
g_Terrains.tier3Terrain = "alpine_grass_a";
g_Terrains.tier4Terrain = "new_alpine_grass_e";
g_Terrains.roadWild = "new_alpine_citytile";
g_Terrains.road = "new_alpine_citytile";
g_Gaia.mainHuntableAnimal = "gaia/fauna_deer";
g_Gaia.secondaryHuntableAnimal = "gaia/fauna_pig";
g_Gaia.metalLarge = "gaia/geology_metal_alpine_slabs";
g_Gaia.metalSmall = "gaia/geology_metal_alpine";
g_Gaia.fish = "gaia/fauna_fish_tilapia";
g_Gaia.tree1 = "gaia/flora_tree_poplar";
g_Gaia.tree2 = "gaia/flora_tree_toona";
g_Gaia.tree3 = "gaia/flora_tree_apple";
g_Gaia.tree4 = "gaia/flora_tree_acacia";
g_Gaia.tree5 = "gaia/flora_tree_carob";
g_Decoratives.grass = "actor|props/flora/grass_soft_large.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_tufts_a.xml";
g_Decoratives.rockLarge = "actor|geology/stone_granite_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_granite_small.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_tempe_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_tempe_b.xml";
g_Decoratives.reeds = "actor|props/flora/reeds_pond_lush_a.xml";
g_Decoratives.lillies = "actor|props/flora/water_lillies.xml";
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, getMapBaseHeight());
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("ratumacos", (tile, x, y) => {
	if (tile.indexOf("mud_temp") >= 0)
		addToClass(x, y, g_TileClasses.mountain);
});
RMS.SetProgress(30);

log("Paint tile classes...");
paintTileClassBasedOnHeight(-3, -1, 3, g_TileClasses.shallowWater);
paintTileClassBasedOnHeight(-100, -3, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");

//Coordinate system of the heightmap
var singleBases = [
	[100, 265],
	[180, 260],
	[245, 220],
	[275, 145],
	[40, 165],
	[70, 95],
	[130, 50],
	[205, 45]
];

var strongholdBases = [
	[65, 140],
	[180, 60],
	[260, 190],
	[120, 270]
];
randomPlayerPlacementAt(getTeamsArray(), singleBases, strongholdBases, scale, 0.06);
RMS.SetProgress(50);

log("Render gaia...");
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.shallowWater, 3
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
			g_TileClasses.shallowWater, 3
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
			g_TileClasses.metal, 30,
			g_TileClasses.water, 3,
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3,
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3,
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal"]
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
			g_TileClasses.shallowWater, 2
		],
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
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3,
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["huge"],
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
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	},
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10,
			g_TileClasses.water, 3,
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
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
			g_TileClasses.shallowWater, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(80);

log("Adding lillies...");
createDecoration(
	[
		[new SimpleObject(g_Decoratives.reeds, 1,3, 0,1)],
		[new SimpleObject(g_Decoratives.lillies, 1,2, 0,1)]
	],
	[
		200 * Math.pow(scaleByMapSize(3, 12), 2),
		100 * Math.pow(scaleByMapSize(3, 12), 2)
	],
	stayClasses(g_TileClasses.shallowWater, 0)
);
RMS.SetProgress(90);

ExportMap();
