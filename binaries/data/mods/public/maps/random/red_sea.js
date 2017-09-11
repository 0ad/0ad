// Coordinates: 21.824205, 40.289810
// Map Width: 2900km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing biome...");
setBiome("desert");
initMapSettings();
initTileClasses();

setSunColor(0.733, 0.746, 0.574);

setWindAngle(-0.43);
setWaterTint(0.161, 0.286, 0.353);
setWaterColor(0.129, 0.176, 0.259);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("lake");

setTerrainAmbientColor(0.58, 0.443, 0.353);

setSunRotation(PI * 1.1);
setSunElevation(PI / 7);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Terrains.mainTerrain = "desert_dirt_rocks_2";
g_Terrains.forestFloor1 = "desert_grass_a_sand";
g_Terrains.forestFloor2 = "desert_grass_a_sand";
g_Terrains.tier1Terrain = "desert_dirt_rocks_2";
g_Terrains.tier2Terrain = "desert_dirt_rough";
g_Terrains.tier3Terrain = "desert_dirt_rough";
g_Terrains.tier4Terrain = "desert_sand_stones";
g_Terrains.roadWild = "road2";
g_Terrains.road = "road2";
g_Gaia.tree1 = "gaia/flora_tree_date_palm";
g_Gaia.tree2 = "gaia/flora_tree_senegal_date_palm";
g_Gaia.tree3 = "gaia/flora_tree_fig";
g_Gaia.tree4 = "gaia/flora_tree_cretan_date_palm_tall";
g_Gaia.tree5 = "gaia/flora_tree_cretan_date_palm_short";
g_Gaia.fruitBush = "gaia/flora_bush_grapes";
g_Decoratives.grass = "actor|props/flora/grass_field_dry_tall_b.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.rockLarge = "actor|geology/stone_desert_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_savanna_med.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_desert_dry_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
g_Decoratives.dust = "actor|particle/dust_storm_reddish.xml";
initBiome();

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 1);
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("red_sea", (tile, x, y) => {
	if (tile.indexOf("cliff") >= 0)
		addToClass(x, y, g_TileClasses.mountain);
});
RMS.SetProgress(30);

log("Rendering water...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing players...");
// Coordinate system of the heightmap
var singleBases = [
	[175, 30],
	[45, 210],
	[280, 180],
	[180, 180],
	[230, 115],
	[130, 280],
	[200, 253],
	[90, 115],
	[45, 45]
];
var strongholdBases = [
	[50, 160],
	[100, 50],
	[170, 260],
	[260, 160]
];
randomPlayerPlacementAt(singleBases, strongholdBases, scale, 0.04);
RMS.SetProgress(50);

log("Adding mines and forests...");
addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
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
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
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
			g_TileClasses.berries, 3,
			g_TileClasses.forest, 20,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"sizes": ["big"],
		"mixes": ["similar"],
		"amounts": ["few"]
	}
]));
RMS.SetProgress(60);

log("Ensure initial forests...");
addElements([{
	"func": addForests,
	"avoid": [
		g_TileClasses.berries, 2,
		g_TileClasses.forest, 25,
		g_TileClasses.metal, 3,
		g_TileClasses.mountain, 5,
		g_TileClasses.player, 15,
		g_TileClasses.rock, 3,
		g_TileClasses.water, 2
	],
	"sizes": ["small"],
	"mixes": ["similar"],
	"amounts": ["tons"]
}]);
RMS.SetProgress(65);

log("Adding berries and animals...");
addElements(shuffleArray([
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 2,
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
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
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
			g_TileClasses.forest, 15,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
RMS.SetProgress(70);

log("Adding decoration...");
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
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["many"]
	}
]);
RMS.SetProgress(80);

log("Adding reeds...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(g_Decoratives.reeds, 5, 12, 1, 4),
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
	scaleByMapSize(100, 1000),
	500
);
RMS.SetProgress(85);

log("Adding dust...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(g_Decoratives.dust, 1, 1, 1, 4)],
		false
	),
	0,
	[
		stayClasses(g_TileClasses.dirt, 1),
		avoidClasses(
			g_TileClasses.player, 10,
			g_TileClasses.water, 3
		)
	],
	Math.pow(scaleByMapSize(5, 20), 2),
	500
);
RMS.SetProgress(90);

ExportMap();
