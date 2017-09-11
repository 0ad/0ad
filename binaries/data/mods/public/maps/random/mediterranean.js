// Coordinates: 49.665548, 10.541500
// Map Width: 5000km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing environment...");
setBiome("temperate");
initMapSettings();
initTileClasses(["autumn", "desert", "medit", "polar", "steppe", "temp"]);

setSunColor(0.733, 0.746, 0.574);

setWindAngle(-0.589049);
setWaterTint(0.556863, 0.615686, 0.643137);
setWaterColor(0.494118, 0.639216, 0.713726);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("ocean");

setTerrainAmbientColor(0.72, 0.72, 0.82);

setSunRotation(PI * 0.95);
setSunElevation(PI / 6);

setSkySet("cumulus");
setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 1);
RMS.SetProgress(10);

var biomes = {
	"autumn": {
		// terrains
		"mainTerrain": "temp_grass_d_aut",
		"forestFloor1": "temp_grass_long_b_aut",
		"forestFloor2": "temp_grass_long_b_aut",
		"roadWild": "road_rome_a",
		"road": "road_muddy",
		// gaia
		"tree1": "gaia/flora_tree_oak_aut_new",
		"tree2": "gaia/flora_tree_oak_dead",
		"tree3": "gaia/flora_tree_apple",
		"tree4": "gaia/flora_tree_euro_beech_aut",
		"tree5": "gaia/flora_tree_pine",
		"fruitBush": "gaia/flora_bush_berry",
		"mainHuntableAnimal": "gaia/fauna_sheep",
		"secondaryHuntableAnimal": "gaia/fauna_wolf",
		"stoneLarge": "gaia/geology_stonemine_temperate_quarry",
		"stoneSmall": "gaia/geology_stone_temperate",
		"metalLarge": "gaia/geology_metal_temperate_slabs",
		"metalSmall": "gaia/geology_metal_temperate",
		// decoratives
		"grass": "actor|props/flora/grass_soft_dry_small.xml",
		"grassShort": "actor|props/flora/grass_soft_dry_tuft_a.xml",
		"rockLarge": "actor|geology/stone_granite_med.xml",
		"rockMedium": "actor|geology/stone_granite_small.xml",
		"bushMedium": "actor|props/flora/bush_medit_me_dry.xml",
		"bushSmall": "actor|props/flora/bush_medit_sm_dry.xml",
	},
	"desert": {
		// terrains
		"mainTerrain": "sand_scrub_100",
		"forestFloor1": "sand_scrub_25",
		"forestFloor2": "sand_scrub_25",
		"roadWild": "desert_city_tile_pers",
		"road": "desert_city_tile_pers_dirt",
		// gaia
		"tree1": "gaia/flora_tree_cretan_date_palm_short",
		"tree2": "gaia/flora_tree_senegal_date_palm",
		"tree3": "gaia/flora_tree_date_palm",
		"tree4": "gaia/flora_tree_cretan_date_palm_tall",
		"tree5": "gaia/flora_tree_date_palm",
		"fruitBush": "gaia/flora_bush_grapes",
		"mainHuntableAnimal": "gaia/fauna_camel",
		"secondaryHuntableAnimal": "gaia/fauna_gazelle",
		"stoneLarge": "gaia/geology_stonemine_desert_quarry",
		"stoneSmall": "gaia/geology_stone_desert_small",
		"metalLarge": "gaia/geology_metal_desert_slabs",
		"metalSmall": "gaia/geology_metal_desert_small",
		// decoratives
		"grass": "actor|props/flora/grass_field_parched_tall.xml",
		"grassShort": "actor|props/flora/grass_soft_dry_tuft_a.xml",
		"rockLarge": "actor|structures/gravestone.xml",
		"rockMedium": "actor|geology/stone_desert_med.xml",
		"bushMedium": "actor|props/flora/bush_desert_dry_a.xml",
		"bushSmall": "actor|props/flora/plant_desert_a.xml",
	},
	"medit": {
		// terrains
		"mainTerrain": "medit_grass_field_a",
		"forestFloor1": "medit_grass_wild",
		"forestFloor2": "medit_grass_wild",
		"roadWild": "road_rome_a",
		"road": "road_muddy",
		// gaia
		"tree1": "gaia/flora_tree_poplar_lombardy",
		"tree2": "gaia/flora_tree_cypress",
		"tree3": "gaia/flora_tree_olive",
		"tree4": "gaia/flora_tree_carob",
		"tree5": "gaia/flora_tree_tamarix",
		"fruitBush": "gaia/flora_bush_grapes",
		"mainHuntableAnimal": "gaia/fauna_deer",
		"secondaryHuntableAnimal": "gaia/fauna_goat",
		"stoneLarge": "gaia/geology_stonemine_medit_quarry",
		"stoneSmall": "gaia/geology_stone_mediterranean",
		"metalLarge": "gaia/geology_metal_mediterranean_slabs",
		"metalSmall": "gaia/geology_metal_greek",
		// decoratives
		"grass": "actor|props/flora/grass_soft_small_tall.xml",
		"grassShort": "actor|props/flora/grass_soft_small.xml",
		"rockLarge": "actor|geology/stone_granite_greek_large.xml",
		"rockMedium": "actor|geology/stone_granite_greek_med.xml",
		"bushMedium": "actor|props/flora/bush_medit_underbrush.xml",
		"bushSmall": "actor|props/flora/bush_medit_me_lush.xml",
	},
	"polar": {
		// terrains
		"mainTerrain": "polar_tundra_snow",
		"forestFloor1": "ice_dirt",
		"forestFloor2": "ice_dirt",
		"roadWild": "road_flat",
		"road": "road1",
		// gaia
		"tree1": "gaia/flora_tree_pine_w",
		"tree2": "gaia/flora_tree_dead",
		"tree3": "gaia/flora_tree_dead",
		"tree4": "gaia/flora_tree_pine_w",
		"tree5": "gaia/flora_tree_pine_w",
		"fruitBush": "gaia/fauna_wolf_snow",
		"mainHuntableAnimal": "gaia/fauna_muskox",
		"secondaryHuntableAnimal": "gaia/fauna_wolf",
		"stoneLarge": "gaia/geology_stonemine_alpine_quarry",
		"stoneSmall": "gaia/geology_stone_alpine_a",
		"metalLarge": "gaia/geology_metal_alpine_slabs",
		"metalSmall": "gaia/geology_metal_alpine",
		// decoratives
		"grass": "actor|props/flora/grass_field_parched_short.xml",
		"grassShort": "actor|props/flora/grass_soft_dry_tuft_a.xml",
		"rockLarge": "actor|geology/stone_granite_med.xml",
		"rockMedium": "actor|geology/stone_granite_small.xml",
		"bushMedium": "actor|props/flora/bush_highlands.xml",
		"bushSmall": "actor|props/flora/bush_medit_sm.xml",
	},
	"steppe": {
		// terrains
		"mainTerrain": "steppe_grass_a",
		"forestFloor1": "steppe_grass_c",
		"forestFloor2": "steppe_grass_c",
		"roadWild": "road2",
		"road": "medit_city_tile_dirt",
		// gaia
		"tree1": "gaia/flora_tree_poplar",
		"tree2": "gaia/flora_tree_toona",
		"tree3": "gaia/flora_tree_dead",
		"tree4": "gaia/flora_tree_acacia",
		"tree5": "gaia/flora_tree_poplar_lombardy",
		"fruitBush": "gaia/flora_bush_grapes",
		"mainHuntableAnimal": "gaia/fauna_deer",
		"secondaryHuntableAnimal": "gaia/fauna_horse",
		"stoneLarge": "gaia/geology_stonemine_alpine_quarry",
		"stoneSmall": "gaia/geology_stone_alpine_a",
		"metalLarge": "gaia/geology_metal_alpine_slabs",
		"metalSmall": "gaia/geology_metal_alpine",
		// decoratives
		"grass": "actor|props/flora/grass_medit_flowering_tall.xml",
		"grassShort": "actor|props/flora/grass_field_bloom_short.xml",
		"rockLarge": "actor|geology/stone_granite_greek_med.xml",
		"rockMedium": "actor|geology/stone_granite_greek_small.xml",
		"bushMedium": "actor|props/flora/bush_dry_a.xml",
		"bushSmall": "actor|props/flora/bush_highlands.xml",
	},
	"temp": {
		// terrains
		"mainTerrain": "temp_grass_long",
		"forestFloor1": "temp_grass_clovers_2",
		"forestFloor2": "temp_grass_clovers_2",
		"roadWild": "temp_road_overgrown",
		"road": "temp_road",
		// gaia
		"tree1": "gaia/flora_tree_oak_new",
		"tree2": "gaia/flora_tree_oak_dead",
		"tree3": "gaia/flora_tree_apple",
		"tree4": "gaia/flora_tree_euro_beech",
		"tree5": "gaia/flora_tree_oak_large",
		"fruitBush": "gaia/flora_bush_berry",
		"mainHuntableAnimal": "gaia/fauna_pig",
		"secondaryHuntableAnimal": "gaia/fauna_boar",
		"stoneLarge": "gaia/geology_stonemine_temperate_quarry",
		"stoneSmall": "gaia/geology_stone_temperate",
		"metalLarge": "gaia/geology_metal_temperate_slabs",
		"metalSmall": "gaia/geology_metal_temperate",
		// decoratives
		"grass": "actor|props/flora/grass_soft_large_tall.xml",
		"grassShort": "actor|props/flora/grass_soft_large.xml",
		"rockLarge": "actor|geology/stone_granite_large.xml",
		"rockMedium": "actor|geology/stone_granite_med.xml",
		"bushMedium": "actor|props/flora/bush_tempe_b.xml",
		"bushSmall": "actor|props/flora/bush_tempe_underbrush.xml",
	}
};

log("Copying heightmap...");
var scale = paintHeightmap("mediterranean", (tile, x, y) => {

	if (tile.indexOf("cliff") >= 0)
		addToClass(x, y, g_TileClasses.mountain);

	if (tile.indexOf("desert") >= 0)
		addToClass(x, y, g_TileClasses.desert);

	if (tile.indexOf("medit") >= 0 && tile.indexOf("sand") < 0)
		addToClass(x, y, g_TileClasses.medit);

	if (tile.indexOf("polar") >= 0)
		addToClass(x, y, g_TileClasses.polar);

	if (tile.indexOf("steppe") >= 0)
		addToClass(x, y, g_TileClasses.steppe);

	if (tile.indexOf("temp") >= 0)
		addToClass(x, y, g_TileClasses.temp);

	if (tile.indexOf("aut") >= 0)
		addToClass(x, y, g_TileClasses.autumn);
});
RMS.SetProgress(30);

log("Rendering water...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(40);

log("Placing player bases...");
// Coordinate system of the heightmap
var singleBases = [
	[70,30],
	[90,180],
	[270,75],
	[240,280],
	[160,180]
];

if (g_MapInfo.mapSize >= 320 || g_MapInfo.numPlayers > singleBases.length)
	singleBases.push(
		[140,60],
		[170,250],
		[210, 35],
		[300,155],
		[50,105]
	);

var strongholdBases = [
	[110,50],
	[180,260],
	[260,55]
];

randomPlayerPlacementAt(singleBases, strongholdBases, scale, 0.06, (tileX, tileY) => {

	for (let biome in biomes)
		if (checkIfInClass(tileX, tileY, g_TileClasses[biome]))
		{
			setLocalBiome(biomes[biome]);
			break;
		}
});
RMS.SetProgress(50);

function setLocalBiome(b)
{
	g_Terrains.mainTerrain = b.mainTerrain;
	g_Terrains.forestFloor1 = b.forestFloor1;
	g_Terrains.forestFloor2 = b.forestFloor2;
	g_Terrains.roadWild = b.roadWild;
	g_Terrains.road = b.road;
	g_Gaia.tree1 = b.tree1;
	g_Gaia.tree2 = b.tree2;
	g_Gaia.tree3 = b.tree3;
	g_Gaia.tree4 = b.tree4;
	g_Gaia.tree5 = b.tree5;
	g_Gaia.fruitBush = b.fruitBush;
	g_Gaia.mainHuntableAnimal = b.mainHuntableAnimal;
	g_Gaia.secondaryHuntableAnimal = b.secondaryHuntableAnimal;
	g_Gaia.stoneLarge = b.stoneLarge;
	g_Gaia.stoneSmall = b.stoneSmall;
	g_Gaia.metalLarge = b.metalLarge;
	g_Gaia.metalSmall = b.metalSmall;
	g_Decoratives.grass = b.grass;
	g_Decoratives.grassShort = b.grassShort;
	g_Decoratives.rockLarge = b.rockLarge;
	g_Decoratives.rockMedium = b.rockMedium;
	g_Decoratives.bushMedium = b.bushMedium;
	g_Decoratives.bushSmall = b.bushSmall;
	initBiome();
}

log("Placing fish...");
g_Gaia.fish = "gaia/fauna_fish";
addElements([
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 10,
		],
		"stay": [g_TileClasses.water, 4],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["many"]
	}
]);
RMS.SetProgress(60);

log("Placing whale...");
g_Gaia.fish = "gaia/fauna_whale_fin";
addElements([
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 2,
			g_TileClasses.desert, 50,
			g_TileClasses.steppe, 50
		],
		"stay": [g_TileClasses.water, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["scarce"]
	}
]);
RMS.SetProgress(70);

log("Rendering local biomes...");
for (let biome in biomes)
{
	setLocalBiome(biomes[biome]);

	let localAvoid = g_TileClasses[biome == "temp" ? "plateau" : "autumn"];

	addElements([
		{
			"func": addMetal,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 3,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 30,
				g_TileClasses.rock, 10,
				g_TileClasses.metal, 25,
				g_TileClasses.water, 4,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": ["many"]
		},
		{
			"func": addStone,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 3,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 30,
				g_TileClasses.rock, 10,
				g_TileClasses.metal, 25,
				g_TileClasses.water, 4,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": ["many"]
		},
		{
			"func": addForests,
			"avoid": [
				g_TileClasses.berries, 3,
				g_TileClasses.forest, 15,
				g_TileClasses.metal, 3,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 12,
				g_TileClasses.rock, 2,
				g_TileClasses.water, 2,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["normal"]
		},
		{
			"func": addSmallMetal,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 3,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 30,
				g_TileClasses.rock, 10,
				g_TileClasses.metal, 15,
				g_TileClasses.water, 4,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": ["few", "normal", "many"]
		},
		{
			"func": addBerries,
			"avoid": [
				g_TileClasses.berries, 30,
				g_TileClasses.forest, 2,
				g_TileClasses.metal, 4,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 4,
				g_TileClasses.water, 2,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["tons"]
		},
		{
			"func": addAnimals,
			"avoid": [
				g_TileClasses.animals, 10,
				g_TileClasses.forest, 1,
				g_TileClasses.metal, 2,
				g_TileClasses.mountain, 1,
				g_TileClasses.player, 15,
				g_TileClasses.rock, 2,
				g_TileClasses.water, 1,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["tons"]
		},
				{
			"func": addAnimals,
			"avoid": [
				g_TileClasses.animals, 10,
				g_TileClasses.forest, 1,
				g_TileClasses.metal, 2,
				g_TileClasses.mountain, 1,
				g_TileClasses.player, 15,
				g_TileClasses.rock, 2,
				g_TileClasses.water, 1,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["small"],
			"mixes": ["normal"],
			"amounts": ["tons"]
		},
		{
			"func": addStragglerTrees,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 5,
				g_TileClasses.metal, 2,
				g_TileClasses.mountain, 1,
				g_TileClasses.player, 12,
				g_TileClasses.rock, 2,
				g_TileClasses.water, 3,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["some"]
		},
		{
			"func": addDecoration,
			"avoid": [
				g_TileClasses.forest, 2,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 12,
				g_TileClasses.water, 4,
				localAvoid, 2
			],
			"stay": [g_TileClasses[biome], 0],
			"sizes": ["small"],
			"mixes": ["same"],
			"amounts": ["normal"]
		}
	]);
}
RMS.SetProgress(90);

ExportMap();
