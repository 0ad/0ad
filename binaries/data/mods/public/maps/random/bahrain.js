// Coordinates: 25.574723, 50.670750
// Map Width: 180km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

setBiome("desert");
initForestFloor();
initTileClasses(["island"]);

log("Initializing environment...");

setSunColor(0.733, 0.746, 0.574);
setSkySet("cloudless");

setWaterTint(0.37, 0.67, 0.73);
setWaterColor(0.24, 0.44, 0.56);
setWaterWaviness(9);
setWaterMurkiness(0.8);
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
g_Terrains.mainTerrain = "desert_dirt_rough_2";
g_Terrains.forestFloor1 = "grass_dead";
g_Terrains.forestFloor2 = "desert_dirt_persia_1";
g_Terrains.tier1Terrain = "desert_sand_dunes_stones";
g_Terrains.tier2Terrain = "desert_sand_scrub";
g_Terrains.tier3Terrain = "desert_plants_b";
g_Terrains.tier4Terrain = "medit_dirt_dry";
g_Terrains.roadWild = "desert_city_tile_pers_dirt";
g_Terrains.road = "desert_city_tile_pers";
g_Gaia.mainHuntableAnimal = "gaia/fauna_camel";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_gazelle";
g_Gaia.fish = "gaia/fauna_fish";
g_Gaia.tree1 = "gaia/flora_tree_cretan_date_palm_tall";
g_Gaia.tree2 = "gaia/flora_tree_cretan_date_palm_short";
g_Gaia.tree3 = "gaia/flora_tree_cretan_date_palm_patch";
g_Gaia.tree4 = "gaia/flora_tree_cretan_date_palm_tall";
g_Gaia.tree5 = "gaia/flora_tree_cretan_date_palm_short";
g_Gaia.fruitBush = "gaia/flora_bush_grapes";
g_Decoratives.grass = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.rockLarge = "actor|geology/stone_savanna_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_granite_greek_small.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_desert_dry_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_medit_la_dry";
initForestFloor();
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, getMapBaseHeight());
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("bahrain", (tile, x, y) => {
	if (tile == "sand")
		addToClass(x, y, g_TileClasses.island);
});
RMS.SetProgress(20);

log("Paint tile classes...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");
const numPlayers = getNumPlayers();
const teamsArray = getTeamsArray();

//Coordinate system of the heightmap
var singleBases = [
	[30, 220],
	[230, 30],
	[75, 130],
	[120, 35],
	[210, 110],
	[240, 220]
];

if (numPlayers > singleBases.length)
	singleBases.push(
		[40, 55],
		[280, 150]
	);

var strongholdBases = [
	[75, 55],
	[250, 55]
];

if (teamsArray.length > strongholdBases.length)
	strongholdBases.push(
		[45, 180],
		[260, 195]
	);

randomPlayerPlacementAt(teamsArray, singleBases, strongholdBases, scale, 0.06);
RMS.SetProgress(50);

log("Render mainland...");
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
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
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["small"],
		"mixes": ["same"],
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
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 30,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
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
			g_TileClasses.player, 30,
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal"]
	}
]));

addElements([
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 35,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2,
			g_TileClasses.island, 2
		],
		"sizes": ["big"],
		"mixes": ["similar"],
		"amounts": ["few"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["many"]
	},
]);

addElements(shuffleArray([
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3,
			g_TileClasses.island, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
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
		"amounts": ["many"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 2,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5,
			g_TileClasses.island, 2
		],
		"sizes": ["small"],
		"mixes": ["same"],
		"amounts": ["normal"]
	}
]));
RMS.SetProgress(65);

g_Terrains.mainTerrain = "sand";
g_Terrains.forestFloor1 = "desert_wave";
g_Terrains.forestFloor2 = "desert_sahara";
g_Terrains.tier1Terrain = "sand_scrub_25";
g_Terrains.tier2Terrain = "sand_scrub_75";
g_Terrains.tier3Terrain = "sand_scrub_50";
g_Terrains.tier4Terrain = "sand";
initForestFloor();

log("Render island...");
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["few"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["tiny"],
		"mixes": ["same"],
		"amounts": ["scarce"]
	}
]);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 7,
			g_TileClasses.metal, 7,
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
			g_TileClasses.forest, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 7,
			g_TileClasses.metal, 7,
			g_TileClasses.water, 3
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));

addElements(shuffleArray([
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 10,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["normal"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 2,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"stay": [g_TileClasses.island, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));

RMS.SetProgress(80);

log("Adding more decoratives...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject("actor|props/special/eyecandy/awning_wood_small.xml", 1, 1, 1, 7),
			new SimpleObject("actor|props/special/eyecandy/barrels_buried.xml", 1, 2, 1, 7)
		],
		true,
		g_TileClasses.dirt
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 10,
		g_TileClasses.mountain, 2,
		g_TileClasses.forest, 2
	),
	2 * scaleByMapSize(1, 4),
	200
);
RMS.SetProgress(85);

log("Creating food treasures...");
for (let treasure of ["wood", "food_bin"])
{
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[new SimpleObject("gaia/special_treasure_" + treasure, 1, 1, 0, 2)],
			true
		),
		0,
		avoidClasses(
			g_TileClasses.water, 2,
			g_TileClasses.player, 25,
			g_TileClasses.forest, 2
		),
		3 * numPlayers,
		200
	);
}
RMS.SetProgress(90);

log("Creating shipwrecks...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject("other/special_treasure_shipwreck_sail_boat_cut", 1, 1, 0, 1)],
		true
	),
	0,
	stayClasses(g_TileClasses.water, 2),
	numPlayers,
	200
);
RMS.SetProgress(95);

ExportMap();
