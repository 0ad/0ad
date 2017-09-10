// Coordinates: 40.879820, 28.306729
// Map Width: 400km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing tile classes...");
setBiome("mediterranean");
initMapSettings();
initTileClasses();

log("Initializing environment...");

setSunColor(0.753, 0.586, 0.584);
setSkySet("sunset");

setWaterHeight(18);
setWaterTint(0.25, 0.67, 0.65);
setWaterColor(0.18, 0.36, 0.39);
setWaterWaviness(8);
setWaterMurkiness(0.99);
setWaterType("lake");

setTerrainAmbientColor(0.521, 0.475, 0.322);

setSunRotation(PI * .85);
setSunElevation(PI / 14);

setFogFactor(0.15);
setFogThickness(0);
setFogColor(0.64, 0.5, 0.35);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

log("Initializing biome...");

g_Terrains.mainTerrain = "grass_mediterranean_dry_1024test";
g_Terrains.forestFloor1 = "steppe_grass_dirt_66";
g_Terrains.forestFloor2 = "steppe_dirt_a";
g_Terrains.tier1Terrain = "medit_grass_field_b";
g_Terrains.tier2Terrain = "medit_grass_field_dry";
g_Terrains.tier3Terrain = "medit_shrubs_golden";
g_Terrains.tier4Terrain = "steppe_dirt_b";
g_Terrains.roadWild = "road_med_a";
g_Terrains.road = "road2";
g_Gaia.mainHuntableAnimal = "gaia/fauna_horse";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_boar";
g_Gaia.fish = "gaia/fauna_fish";
g_Gaia.tree1 = "gaia/flora_tree_carob";
g_Gaia.tree2 = "gaia/flora_tree_poplar_lombardy";
g_Gaia.tree3 = "gaia/flora_tree_dead";
g_Gaia.tree4 = "gaia/flora_tree_dead";
g_Gaia.tree5 = "gaia/flora_tree_carob";
g_Gaia.fruitBush = "gaia/flora_bush_grapes";
g_Gaia.metalSmall = "gaia/geology_metal_desert_small";
g_Decoratives.grass = "actor|props/special/eyecandy/block_limestone.xml";
g_Decoratives.grassShort = "actor|props/special/eyecandy/blocks_sandstone_pile_a.xml";
g_Decoratives.rockLarge = "actor|geology/stone_savanna_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_granite_small.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_medit_me_dry.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
g_Decoratives.reeds = "actor|props/flora/reeds_pond_lush_a.xml";

initBiome();
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 1);
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("marmara", (tile, x, y) => {
	if (tile.indexOf("cliff") >= 0)
		addToClass(x, y, g_TileClasses.mountain);
});
RMS.SetProgress(30);

log("Paint tile classes...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");
// Coordinate system of the heightmap
var singleBases = [
	[40, 175],
	[80, 280],
	[120, 40],
	[285, 165],
	[200, 50]
];

if (g_MapInfo.mapSize >= 320 || g_MapInfo.numPlayers > singleBases.length)
	singleBases.push(
		[45, 70],
		[280, 80],
		[125, 205]
	);

var strongholdBases = [
	[265, 65],
	[60, 220],
	[105, 60]
];
randomPlayerPlacementAt(singleBases, strongholdBases, scale, 0.06);
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
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 30,
			g_TileClasses.water, 3
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
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
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
			g_TileClasses.forest, 10,
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
		"sizes": ["huge"],
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
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(80);

log("Adding reeds...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(g_Decoratives.reeds, 5, 12, 1, 2),
			new SimpleObject(g_Decoratives.rockMedium, 1, 2, 1, 3)
		],
		true,
		g_TileClasses.dirt
	),
	0,
	[
		stayClasses(g_TileClasses.water, 0),
		borderClasses(g_TileClasses.water, scaleByMapSize(2,8), scaleByMapSize(2,8))
	],
	scaleByMapSize(1000, 4000),
	500
);
RMS.SetProgress(85);

ExportMap();
