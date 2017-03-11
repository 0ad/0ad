const g_BiomeTemperate = 1;
const g_BiomeSnowy = 2;
const g_BiomeDesert = 3;
const g_BiomeAlpine = 4;
const g_BiomeMediterranean = 5;
const g_BiomeSavanna = 6;
const g_BiomeTropic = 7;
const g_BiomeAutumn = 8;

var g_BiomeID = g_BiomeTemperate;

var g_Terrains = {
	"mainTerrain": ["temp_grass_long_b"],
	"forestFloor1": "temp_forestfloor_pine",
	"forestFloor2": "temp_plants_bog",
	"tier1Terrain": "temp_grass_d",
	"tier2Terrain": "temp_grass_c",
	"tier3Terrain": "temp_grass_clovers_2",
	"tier4Terrain": "temp_grass_plants",
	"cliff": ["temp_cliff_a", "temp_cliff_b"],
	"hill": ["temp_dirt_gravel", "temp_dirt_gravel_b"],
	"dirt": ["temp_dirt_gravel", "temp_dirt_gravel_b"],
	"road": "temp_road",
	"roadWild": "temp_road_overgrown",
	"shoreBlend": "temp_mud_plants",
	"shore": "sand_grass_25",
	"water": "medit_sand_wet"
};

var g_Gaia = {
	"tree1": "gaia/flora_tree_oak",
	"tree2": "gaia/flora_tree_oak_large",
	"tree3": "gaia/flora_tree_apple",
	"tree4": "gaia/flora_tree_pine",
	"tree5": "gaia/flora_tree_aleppo_pine",
	"fruitBush": "gaia/flora_bush_berry",
	"chicken": "gaia/fauna_chicken",
	"fish": "gaia/fauna_fish",
	"mainHuntableAnimal": "gaia/fauna_deer",
	"secondaryHuntableAnimal": "gaia/fauna_sheep",
	"stoneLarge": "gaia/geology_stonemine_medit_quarry",
	"stoneSmall": "gaia/geology_stone_mediterranean",
	"metalLarge": "gaia/geology_metal_mediterranean_slabs"
};

var g_Decoratives = {
	"grass": "actor|props/flora/grass_soft_large_tall.xml",
	"grassShort": "actor|props/flora/grass_soft_large.xml",
	"reeds": "actor|props/flora/reeds_pond_lush_a.xml",
	"lillies": "actor|props/flora/pond_lillies_large.xml",
	"rockLarge": "actor|geology/stone_granite_large.xml",
	"rockMedium": "actor|geology/stone_granite_med.xml",
	"bushMedium": "actor|props/flora/bush_medit_me.xml",
	"bushSmall": "actor|props/flora/bush_medit_sm.xml",
	"tree": "actor|flora/trees/oak.xml"
};

/**
 * Randomizes environment, optionally excluding some biome IDs.
 */
function randomizeBiome(avoid = [])
{
	let biomeIndex;
	do
		biomeIndex = randInt(1, 8);
	while (avoid.indexOf(biomeIndex) != -1);

	setBiome(biomeIndex);

	return biomeIndex;
}

function setBiome(biomeIndex)
{
	setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));

	setSunRotation(randFloat(0, TWO_PI));
	setSunElevation(randFloat(PI/ 6, PI / 3));

	setUnitsAmbientColor(0.57, 0.58, 0.55);
	setTerrainAmbientColor(0.447059, 0.509804, 0.54902);

	g_BiomeID = biomeIndex;

	if (g_BiomeID == g_BiomeTemperate)
	{
		// temperate ocean blue, a bit too deep and saturated perhaps but it looks nicer.
		// this assumes ocean settings, maps that aren't oceans should reset.
		setWaterColor(0.114, 0.192, 0.463);
		setWaterTint(0.255, 0.361, 0.651);
		setWaterWaviness(5.5);
		setWaterMurkiness(0.83);

		setFogThickness(0.25);
		setFogFactor(0.4);

		setPPEffect("hdr");
		setPPSaturation(0.62);
		setPPContrast(0.62);
		setPPBloom(0.3);

		g_Terrains = {
			"cliff": ["temp_cliff_a", "temp_cliff_b"],
			"hill": ["temp_dirt_gravel", "temp_dirt_gravel_b"],
			"dirt": ["temp_dirt_gravel", "temp_dirt_gravel_b"],
			"road": "temp_road",
			"roadWild": "temp_road_overgrown",
			"shoreBlend": "temp_mud_plants",
			"shore": "sand_grass_25",
			"water": "medit_sand_wet"
		};

		if (randInt(2))
		{
			g_Terrains.mainTerrain = "alpine_grass";
			g_Terrains.forestFloor1 = "temp_forestfloor_pine";
			g_Terrains.forestFloor2 = "temp_grass_clovers_2";
			g_Terrains.tier1Terrain = "alpine_grass_a";
			g_Terrains.tier2Terrain = "alpine_grass_b";
			g_Terrains.tier3Terrain = "alpine_grass_c";
			g_Terrains.tier4Terrain = "temp_grass_mossy";
		}
		else
		{
			g_Terrains.mainTerrain = "temp_grass_long_b";
			g_Terrains.forestFloor1 = "temp_forestfloor_pine";
			g_Terrains.forestFloor2 = "temp_plants_bog";
			g_Terrains.tier1Terrain = "temp_grass_d";
			g_Terrains.tier2Terrain = "temp_grass_c";
			g_Terrains.tier3Terrain = "temp_grass_clovers_2";
			g_Terrains.tier4Terrain = "temp_grass_plants";
		}

		g_Gaia = {
			"fruitBush": "gaia/flora_bush_berry",
			"chicken": "gaia/fauna_chicken",
			"fish": "gaia/fauna_fish",
			"mainHuntableAnimal": "gaia/fauna_deer",
			"secondaryHuntableAnimal": "gaia/fauna_sheep",
			"stoneLarge": "gaia/geology_stonemine_temperate_quarry",
			"stoneSmall": "gaia/geology_stone_temperate",
			"metalLarge": "gaia/geology_metal_temperate_slabs"
		};

		var random_trees = randInt(3);
		if (random_trees == 0)
		{
			g_Gaia.tree1 = "gaia/flora_tree_oak";
			g_Gaia.tree2 = "gaia/flora_tree_oak_large";
		}
		else if (random_trees == 1)
		{
			g_Gaia.tree1 = "gaia/flora_tree_poplar";
			g_Gaia.tree2 = "gaia/flora_tree_poplar";
		}
		else
		{
			g_Gaia.tree1 = "gaia/flora_tree_euro_beech";
			g_Gaia.tree2 = "gaia/flora_tree_euro_beech";
		}
		g_Gaia.tree3 = "gaia/flora_tree_apple";
		random_trees = randInt(3);
		if (random_trees == 0)
		{
			g_Gaia.tree4 = "gaia/flora_tree_pine";
			g_Gaia.tree5 = "gaia/flora_tree_aleppo_pine";
		}
		else if (random_trees == 1)
		{
			g_Gaia.tree4 = "gaia/flora_tree_pine";
			g_Gaia.tree5 = "gaia/flora_tree_pine";
		}
		else
		{
			g_Gaia.tree4 = "gaia/flora_tree_aleppo_pine";
			g_Gaia.tree5 = "gaia/flora_tree_aleppo_pine";
		}

		g_Decoratives = {
			"grass": "actor|props/flora/grass_soft_large_tall.xml",
			"grassShort": "actor|props/flora/grass_soft_large.xml",
			"reeds": "actor|props/flora/reeds_pond_lush_a.xml",
			"lillies": "actor|props/flora/water_lillies.xml",
			"rockLarge": "actor|geology/stone_granite_large.xml",
			"rockMedium": "actor|geology/stone_granite_med.xml",
			"bushMedium": "actor|props/flora/bush_medit_me.xml",
			"bushSmall": "actor|props/flora/bush_medit_sm.xml",
			"tree": "actor|flora/trees/oak.xml"
		};
	}
	else if (g_BiomeID == g_BiomeSnowy)
	{
		setSunColor(0.550, 0.601, 0.644);				// a little darker
		// Water is a semi-deep blue, fairly wavy, fairly murky for an ocean.
		// this assumes ocean settings, maps that aren't oceans should reset.
		setWaterColor(0.067, 0.212, 0.361);
		setWaterTint(0.4, 0.486, 0.765);
		setWaterWaviness(5.5);
		setWaterMurkiness(0.83);

		g_Terrains = {
			"mainTerrain": ["polar_snow_b", "snow grass 75", "snow rocks", "snow forest"],
			"forestFloor1": "polar_tundra_snow",
			"forestFloor2": "polar_tundra_snow",
			"cliff": ["alpine_cliff_a", "alpine_cliff_b"],
			"tier1Terrain": "polar_snow_a",
			"tier2Terrain": "polar_ice_snow",
			"tier3Terrain": "polar_ice",
			"tier4Terrain": "snow grass 2",
			"hill": ["polar_snow_rocks", "polar_cliff_snow"],
			"dirt": "snow grass 2",
			"road": "new_alpine_citytile",
			"roadWild": "polar_ice_cracked",
			"shoreBlend": "polar_ice",
			"shore": "alpine_shore_rocks_icy",
			"water": "alpine_shore_rocks"
		};

		g_Gaia = {
			"tree1": "gaia/flora_tree_pine_w",
			"tree2": "gaia/flora_tree_pine_w",
			"tree3": "gaia/flora_tree_pine_w",
			"tree4": "gaia/flora_tree_pine_w",
			"tree5": "gaia/flora_tree_pine",
			"fruitBush": "gaia/flora_bush_berry",
			"chicken": "gaia/fauna_chicken",
			"mainHuntableAnimal": "gaia/fauna_muskox",
			"fish": "gaia/fauna_fish_tuna",
			"secondaryHuntableAnimal": "gaia/fauna_walrus",
			"stoneLarge": "gaia/geology_stonemine_alpine_quarry",
			"stoneSmall": "gaia/geology_stone_alpine_a",
			"metalLarge": "gaia/geology_metal_alpine_slabs"
		};

		g_Decoratives = {
			"grass": "actor|props/flora/grass_soft_dry_small_tall.xml",
			"grassShort": "actor|props/flora/grass_soft_dry_large.xml",
			"reeds": "actor|props/flora/reeds_pond_dry.xml",
			"lillies": "actor|geology/stone_granite_large.xml",
			"rockLarge": "actor|geology/stone_granite_large.xml",
			"rockMedium": "actor|geology/stone_granite_med.xml",
			"bushMedium": "actor|props/flora/bush_desert_dry_a.xml",
			"bushSmall": "actor|props/flora/bush_desert_dry_a.xml",
			"tree": "actor|flora/trees/pine_w.xml"
		};

		setFogFactor(0.6);
		setFogThickness(0.21);
		setPPSaturation(0.37);
		setPPEffect("hdr");
	}
	else if (g_BiomeID == g_BiomeDesert)
	{
		setSunColor(0.733, 0.746, 0.574);

		// Went for a very clear, slightly blue-ish water in this case, basically no waves.
		setWaterColor(0, 0.227, 0.843);
		setWaterTint(0, 0.545, 0.859);
		setWaterWaviness(1);
		setWaterMurkiness(0.22);

		setFogFactor(0.5);
		setFogThickness(0.0);
		setFogColor(0.852, 0.746, 0.493);

		setPPEffect("hdr");
		setPPContrast(0.67);
		setPPSaturation(0.42);
		setPPBloom(0.23);

		g_Terrains = {
			"mainTerrain": ["desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_dunes_50", "desert_sand_smooth"],
			"forestFloor1": "forestfloor_dirty",
			"forestFloor2": "desert_forestfloor_palms",
			"cliff": ["desert_cliff_1", "desert_cliff_2", "desert_cliff_3", "desert_cliff_4", "desert_cliff_5"],
			"tier1Terrain": "desert_dirt_rough",
			"tier2Terrain": "desert_dirt_rocks_1",
			"tier3Terrain": "desert_dirt_rocks_2",
			"tier4Terrain": "desert_dirt_rough",
			"hill": ["desert_dirt_rocks_1", "desert_dirt_rocks_2", "desert_dirt_rocks_3"],
			"dirt": ["desert_lakebed_dry", "desert_lakebed_dry_b"],
			"road": "desert_city_tile",
			"roadWild": "desert_city_tile",
			"shoreBlend": "desert_shore_stones",
			"shore": "desert_sand_smooth",
			"water": "desert_sand_wet"
		};

		g_Gaia = {
			"fruitBush": "gaia/flora_bush_grapes",
			"chicken": "gaia/fauna_chicken",
			"mainHuntableAnimal": "gaia/fauna_camel",
			"fish": "gaia/fauna_fish",
			"secondaryHuntableAnimal": "gaia/fauna_gazelle",
			"stoneLarge": "gaia/geology_stonemine_desert_quarry",
			"stoneSmall": "gaia/geology_stone_desert_small",
			"metalLarge": "gaia/geology_metal_desert_slabs"
		};
		if (randInt(2))
		{
			g_Gaia.tree1 = "gaia/flora_tree_cretan_date_palm_short";
			g_Gaia.tree2 = "gaia/flora_tree_cretan_date_palm_tall";
		}
		else
		{
			g_Gaia.tree1 = "gaia/flora_tree_date_palm";
			g_Gaia.tree2 = "gaia/flora_tree_date_palm";
		}
		g_Gaia.tree3 = "gaia/flora_tree_fig";
		if (randInt(2))
		{
			g_Gaia.tree4 = "gaia/flora_tree_tamarix";
			g_Gaia.tree5 = "gaia/flora_tree_tamarix";
		}
		else
		{
			g_Gaia.tree4 = "gaia/flora_tree_senegal_date_palm";
			g_Gaia.tree5 = "gaia/flora_tree_senegal_date_palm";
		}

		g_Decoratives = {
			"grass": "actor|props/flora/grass_soft_dry_small_tall.xml",
			"grassShort": "actor|props/flora/grass_soft_dry_large.xml",
			"reeds": "actor|props/flora/reeds_pond_lush_a.xml",
			"lillies": "actor|props/flora/reeds_pond_lush_b.xml",
			"rockLarge": "actor|geology/stone_desert_med.xml",
			"rockMedium": "actor|geology/stone_desert_med.xml",
			"bushMedium": "actor|props/flora/bush_desert_dry_a.xml",
			"bushSmall": "actor|props/flora/bush_desert_dry_a.xml",
			"tree": "actor|flora/trees/palm_date.xml"
		};
	}
	else if (g_BiomeID == g_BiomeAlpine)
	{
		// simulates an alpine lake, fairly deep.
		// this is not intended for a clear running river, or even an ocean.
		setWaterColor(0.0, 0.047, 0.286);				// dark majestic blue
		setWaterTint(0.471, 0.776, 0.863);				// light blue
		setWaterMurkiness(0.82);
		setWaterWaviness(2);

		setFogThickness(0.26);
		setFogFactor(0.4);

		setPPEffect("hdr");
		setPPSaturation(0.48);
		setPPContrast(0.53);
		setPPBloom(0.12);

		g_Terrains = {
			"mainTerrain": ["alpine_dirt_grass_50"],
			"forestFloor1": "alpine_forrestfloor",
			"forestFloor2": "alpine_forrestfloor",
			"cliff": ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"],
			"tier1Terrain": "alpine_dirt",
			"tier2Terrain": ["alpine_grass_snow_50", "alpine_dirt_snow", "alpine_dirt_snow"],
			"tier3Terrain": ["alpine_snow_a", "alpine_snow_b"],
			"tier4Terrain": "new_alpine_grass_a",
			"hill": "alpine_cliff_snow",
			"dirt": ["alpine_dirt", "alpine_grass_d"],
			"road": "new_alpine_citytile",
			"roadWild": "new_alpine_citytile",
			"shoreBlend": "alpine_shore_rocks",
			"shore": "alpine_shore_rocks_grass_50",
			"water": "alpine_shore_rocks"
		};

		g_Gaia = {
			"tree1": "gaia/flora_tree_pine",
			"tree2": "gaia/flora_tree_pine",
			"tree3": "gaia/flora_tree_pine",
			"tree4": "gaia/flora_tree_pine",
			"tree5": "gaia/flora_tree_pine",
			"fruitBush": "gaia/flora_bush_berry",
			"chicken": "gaia/fauna_chicken",
			"mainHuntableAnimal": "gaia/fauna_goat",
			"fish": "gaia/fauna_fish_tuna",
			"secondaryHuntableAnimal": "gaia/fauna_deer",
			"stoneLarge": "gaia/geology_stonemine_alpine_quarry",
			"stoneSmall": "gaia/geology_stone_alpine_a",
			"metalLarge": "gaia/geology_metal_alpine_slabs"
		};

		g_Decoratives = {
			"grass": "actor|props/flora/grass_soft_small_tall.xml",
			"grassShort": "actor|props/flora/grass_soft_large.xml",
			"reeds": "actor|props/flora/reeds_pond_dry.xml",
			"lillies": "actor|geology/stone_granite_large.xml",
			"rockLarge": "actor|geology/stone_granite_large.xml",
			"rockMedium": "actor|geology/stone_granite_med.xml",
			"bushMedium": "actor|props/flora/bush_desert_a.xml",
			"bushSmall": "actor|props/flora/bush_desert_a.xml",
			"tree": "actor|flora/trees/pine.xml"
		};
	}
	else if (g_BiomeID == g_BiomeMediterranean)
	{
		// Guess what, this is based on the colors of the mediterranean sea.
		setWaterColor(0.024,0.212,0.024);
		setWaterTint(0.133, 0.725,0.855);
		setWaterWaviness(3);
		setWaterMurkiness(0.8);

		setFogFactor(0.3);
		setFogThickness(0.25);

		setPPEffect("hdr");
		setPPContrast(0.62);
		setPPSaturation(0.51);
		setPPBloom(0.12);

		g_Terrains = {
			"mainTerrain": ["medit_grass_field_a", "medit_grass_field_b"],
			"forestFloor1": "medit_grass_field",
			"forestFloor2": "medit_grass_shrubs",
			"cliff": ["medit_cliff_grass", "medit_cliff_greek", "medit_cliff_greek_2", "medit_cliff_aegean", "medit_cliff_italia", "medit_cliff_italia_grass"],
			"tier1Terrain": "medit_grass_field_b",
			"tier2Terrain": "medit_grass_field_brown",
			"tier3Terrain": "medit_grass_field_dry",
			"tier4Terrain": "medit_grass_wild",
			"hill": ["medit_rocks_grass_shrubs", "medit_rocks_shrubs"],
			"dirt": ["medit_dirt", "medit_dirt_b"],
			"road": "medit_city_tile",
			"roadWild": "medit_city_tile",
			"shoreBlend": "medit_sand",
			"shore": "sand_grass_25",
			"water": "medit_sand_wet"
		};

		g_Gaia = {
			"chicken": "gaia/fauna_chicken",
			"mainHuntableAnimal": "gaia/fauna_deer",
			"fish": "gaia/fauna_fish",
			"secondaryHuntableAnimal": "gaia/fauna_sheep",
			"stoneLarge": "gaia/geology_stonemine_medit_quarry",
			"stoneSmall": "gaia/geology_stone_mediterranean",
			"metalLarge": "gaia/geology_metal_mediterranean_slabs"
		};

		var random_trees = randInt(3);
		if (random_trees == 0)
		{
			g_Gaia.tree1 = "gaia/flora_tree_cretan_date_palm_short";
			g_Gaia.tree2 = "gaia/flora_tree_cretan_date_palm_tall";
		}
		else if (random_trees == 1)
		{
			g_Gaia.tree1 = "gaia/flora_tree_carob";
			g_Gaia.tree2 = "gaia/flora_tree_carob";
		}
		else
		{
			g_Gaia.tree1 = "gaia/flora_tree_medit_fan_palm";
			g_Gaia.tree2 = "gaia/flora_tree_medit_fan_palm";
		}

		if (randInt(2))
			g_Gaia.tree3 = "gaia/flora_tree_apple";
		else
			g_Gaia.tree3 = "gaia/flora_tree_poplar_lombardy";

		if (randInt(2))
		{
			g_Gaia.tree4 = "gaia/flora_tree_cypress";
			g_Gaia.tree5 = "gaia/flora_tree_cypress";
		}
		else
		{
			g_Gaia.tree4 = "gaia/flora_tree_aleppo_pine";
			g_Gaia.tree5 = "gaia/flora_tree_aleppo_pine";
		}

		if (randInt(2))
			g_Gaia.fruitBush = "gaia/flora_bush_berry";
		else
			g_Gaia.fruitBush = "gaia/flora_bush_grapes";

		g_Decoratives = {
			"grass": "actor|props/flora/grass_soft_large_tall.xml",
			"grassShort": "actor|props/flora/grass_soft_large.xml",
			"reeds": "actor|props/flora/reeds_pond_lush_b.xml",
			"lillies": "actor|props/flora/water_lillies.xml",
			"rockLarge": "actor|geology/stone_granite_large.xml",
			"rockMedium": "actor|geology/stone_granite_med.xml",
			"bushMedium": "actor|props/flora/bush_medit_me.xml",
			"bushSmall": "actor|props/flora/bush_medit_sm.xml",
			"tree": "actor|flora/trees/palm_cretan_date.xml"
		};
	}
	else if (g_BiomeID == g_BiomeSavanna)
	{
		// Using the Malawi as a reference, in parts where it's not too murky from a river nearby.
		setWaterColor(0.055,0.176,0.431);
		setWaterTint(0.227,0.749,0.549);
		setWaterWaviness(1.5);
		setWaterMurkiness(0.77);

		setFogFactor(0.25);
		setFogThickness(0.15);
		setFogColor(0.847059, 0.737255, 0.482353);

		setPPEffect("hdr");
		setPPContrast(0.57031);
		setPPBloom(0.34);

		g_Terrains = {
			"mainTerrain": ["savanna_grass_a", "savanna_grass_b"],
			"forestFloor1": "savanna_forestfloor_a",
			"forestFloor2": "savanna_forestfloor_b",
			"cliff": ["savanna_cliff_a", "savanna_cliff_b"],
			"tier1Terrain": "savanna_shrubs_a",
			"tier2Terrain": "savanna_dirt_rocks_b",
			"tier3Terrain": "savanna_dirt_rocks_a",
			"tier4Terrain": "savanna_grass_a",
			"hill": ["savanna_grass_a", "savanna_grass_b"],
			"dirt": ["savanna_dirt_rocks_b", "dirt_brown_e"],
			"road": "savanna_tile_a",
			"roadWild": "savanna_tile_a",
			"shoreBlend": "savanna_riparian",
			"shore": "savanna_riparian_bank",
			"water": "savanna_riparian_wet"
		};

		g_Gaia = {
			"tree1": "gaia/flora_tree_baobab",
			"tree2": "gaia/flora_tree_baobab",
			"tree3": "gaia/flora_tree_baobab",
			"tree4": "gaia/flora_tree_baobab",
			"tree5": "gaia/flora_tree_baobab",
			"fruitBush": "gaia/flora_bush_grapes",
			"chicken": "gaia/fauna_chicken",
			"fish": "gaia/fauna_fish",
			"secondaryHuntableAnimal": "gaia/fauna_gazelle",
			"stoneLarge": "gaia/geology_stonemine_desert_quarry",
			"stoneSmall": "gaia/geology_stone_savanna_small",
			"metalLarge": "gaia/geology_metal_savanna_slabs"
		};

		var rts = randInt(1,4);
		if (rts == 1)
			g_Gaia.mainHuntableAnimal = "gaia/fauna_wildebeest";
		else if (rts == 2)
			g_Gaia.mainHuntableAnimal = "gaia/fauna_zebra";
		else if (rts == 3)
			g_Gaia.mainHuntableAnimal = "gaia/fauna_giraffe";
		else if (rts == 4)
			g_Gaia.mainHuntableAnimal = "gaia/fauna_elephant_african_bush";

		g_Decoratives = {
			"grass": "actor|props/flora/grass_savanna.xml",
			"grassShort": "actor|props/flora/grass_medit_field.xml",
			"reeds": "actor|props/flora/reeds_pond_lush_a.xml",
			"lillies": "actor|props/flora/reeds_pond_lush_b.xml",
			"rockLarge": "actor|geology/stone_savanna_med.xml",
			"rockMedium": "actor|geology/stone_savanna_med.xml",
			"bushMedium": "actor|props/flora/bush_desert_dry_a.xml",
			"bushSmall": "actor|props/flora/bush_dry_a.xml",
			"tree": "actor|flora/trees/baobab.xml"
		};
	}
	else if (g_BiomeID == g_BiomeTropic)
	{

		// Bora-Bora ish. Quite transparent, not wavy.
		// Mostly for shallow maps. Maps where the water level goes deeper should use a much darker Water Color to simulate deep water holes.
		setWaterColor(0.584,0.824,0.929);
		setWaterTint(0.569,0.965,0.945);
		setWaterWaviness(1.5);
		setWaterMurkiness(0.35);

		setFogFactor(0.4);
		setFogThickness(0.2);

		setPPEffect("hdr");
		setPPContrast(0.67);
		setPPSaturation(0.62);
		setPPBloom(0.6);

		g_Terrains = {
			"mainTerrain": ["tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_plants", "tropic_plants", "tropic_plants_b"],
			"forestFloor1": "tropic_plants_c",
			"forestFloor2": "tropic_plants_c",
			"cliff": ["tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a_plants"],
			"tier1Terrain": "tropic_grass_c",
			"tier2Terrain": "tropic_grass_plants",
			"tier3Terrain": "tropic_plants",
			"tier4Terrain": "tropic_plants_b",
			"hill": ["tropic_cliff_grass"],
			"dirt": ["tropic_dirt_a", "tropic_dirt_a_plants"],
			"road": "tropic_citytile_a",
			"roadWild": "tropic_citytile_plants",
			"shoreBlend": "temp_mud_plants",
			"shore": "tropic_beach_dry_plants",
			"water": "tropic_beach_dry"
		};

		g_Gaia = {
			"tree1": "gaia/flora_tree_toona",
			"tree2": "gaia/flora_tree_toona",
			"tree3": "gaia/flora_tree_palm_tropic",
			"tree4": "gaia/flora_tree_palm_tropic",
			"tree5": "gaia/flora_tree_palm_tropic",
			"fruitBush": "gaia/flora_bush_berry",
			"chicken": "gaia/fauna_chicken",
			"mainHuntableAnimal": "gaia/fauna_peacock",
			"fish": "gaia/fauna_fish",
			"secondaryHuntableAnimal": "gaia/fauna_tiger",
			"stoneLarge": "gaia/geology_stonemine_tropic_quarry",
			"stoneSmall": "gaia/geology_stone_tropic_a",
			"metalLarge": "gaia/geology_metal_tropic_slabs"
		};

		g_Decoratives = {
			"grass": "actor|props/flora/plant_tropic_a.xml",
			"grassShort": "actor|props/flora/plant_lg.xml",
			"reeds": "actor|props/flora/reeds_pond_lush_b.xml",
			"lillies": "actor|props/flora/water_lillies.xml",
			"rockLarge": "actor|geology/stone_granite_large.xml",
			"rockMedium": "actor|geology/stone_granite_med.xml",
			"bushMedium": "actor|props/flora/plant_tropic_large.xml",
			"bushSmall": "actor|props/flora/plant_tropic_large.xml",
			"tree": "actor|flora/trees/tree_tropic.xml"
		};
	}
	else if (g_BiomeID == g_BiomeAutumn)
	{
		// basically temperate with a reddish twist in the reflection and the tint. Also less wavy.
		// this assumes ocean settings, maps that aren't oceans should reset.
		setWaterColor(0.157, 0.149, 0.443);
		setWaterTint(0.443,0.42,0.824);
		setWaterWaviness(2.5);
		setWaterMurkiness(0.83);

		setFogFactor(0.35);
		setFogThickness(0.22);
		setFogColor(0.82,0.82, 0.73);
		setPPSaturation(0.56);
		setPPContrast(0.56);
		setPPBloom(0.38);
		setPPEffect("hdr");

		g_Terrains = {
			"mainTerrain": ["temp_grass_aut", "temp_grass_aut", "temp_grass_d_aut"],
			"forestFloor1": "temp_plants_bog_aut",
			"forestFloor2": "temp_forestfloor_aut",
			"cliff": ["temp_cliff_a", "temp_cliff_b"],
			"tier1Terrain": "temp_grass_plants_aut",
			"tier2Terrain": ["temp_grass_b_aut", "temp_grass_c_aut"],
			"tier3Terrain": ["temp_grass_b_aut", "temp_grass_long_b_aut"],
			"tier4Terrain": "temp_grass_plants_aut",
			"hill": "temp_highlands_aut",
			"dirt": ["temp_cliff_a", "temp_cliff_b"],
			"road": "temp_road_aut",
			"roadWild": "temp_road_overgrown_aut",
			"shoreBlend": "temp_grass_plants_aut",
			"shore": "temp_forestfloor_pine",
			"water": "medit_sand_wet"
		};

		g_Gaia = {
			"tree1": "gaia/flora_tree_euro_beech_aut",
			"tree2": "gaia/flora_tree_euro_beech_aut",
			"tree3": "gaia/flora_tree_pine",
			"tree4": "gaia/flora_tree_oak_aut",
			"tree5": "gaia/flora_tree_oak_aut",
			"fruitBush": "gaia/flora_bush_berry",
			"chicken": "gaia/fauna_chicken",
			"mainHuntableAnimal": "gaia/fauna_deer",
			"fish": "gaia/fauna_fish",
			"secondaryHuntableAnimal": "gaia/fauna_rabbit",
			"stoneLarge": "gaia/geology_stonemine_temperate_quarry",
			"stoneSmall": "gaia/geology_stone_temperate",
			"metalLarge": "gaia/geology_metal_temperate_slabs"
		};

		g_Decoratives = {
			"grass": "actor|props/flora/grass_soft_dry_small_tall.xml",
			"grassShort": "actor|props/flora/grass_soft_dry_large.xml",
			"reeds": "actor|props/flora/reeds_pond_dry.xml",
			"lillies": "actor|geology/stone_granite_large.xml",
			"rockLarge": "actor|geology/stone_granite_large.xml",
			"rockMedium": "actor|geology/stone_granite_med.xml",
			"bushMedium": "actor|props/flora/bush_desert_dry_a.xml",
			"bushSmall": "actor|props/flora/bush_desert_dry_a.xml",
			"tree": "actor|flora/trees/european_beech_aut.xml"
		};
	}
}

function rBiomeT1()
{
	return g_Terrains.mainTerrain;
}

function rBiomeT2()
{
	return g_Terrains.forestFloor1;
}

function rBiomeT3()
{
	return g_Terrains.forestFloor2;
}

function rBiomeT4()
{
	return g_Terrains.cliff;
}

function rBiomeT5()
{
	return g_Terrains.tier1Terrain;
}

function rBiomeT6()
{
	return g_Terrains.tier2Terrain;
}

function rBiomeT7()
{
	return g_Terrains.tier3Terrain;
}

function rBiomeT8()
{
	return g_Terrains.hill;
}

function rBiomeT9()
{
	return g_Terrains.dirt;
}

function rBiomeT10()
{
	return g_Terrains.road;
}

function rBiomeT11()
{
	return g_Terrains.roadWild;
}

function rBiomeT12()
{
	return g_Terrains.tier4Terrain;
}

function rBiomeT13()
{
	return g_Terrains.shoreBlend;
}

function rBiomeT14()
{
	return g_Terrains.shore;
}

function rBiomeT15()
{
	return g_Terrains.water;
}

function rBiomeE1()
{
	return g_Gaia.tree1;
}

function rBiomeE2()
{
	return g_Gaia.tree2;
}

function rBiomeE3()
{
	return g_Gaia.tree3;
}

function rBiomeE4()
{
	return g_Gaia.tree4;
}

function rBiomeE5()
{
	return g_Gaia.tree5;
}

function rBiomeE6()
{
	return g_Gaia.fruitBush;
}

function rBiomeE7()
{
	return g_Gaia.chicken;
}

function rBiomeE8()
{
	return g_Gaia.mainHuntableAnimal;
}

function rBiomeE9()
{
	return g_Gaia.fish;
}

function rBiomeE10()
{
	return g_Gaia.secondaryHuntableAnimal;
}

function rBiomeE11()
{
	return g_Gaia.stoneLarge;
}

function rBiomeE12()
{
	return g_Gaia.stoneSmall;
}

function rBiomeE13()
{
	return g_Gaia.metalLarge;
}

function rBiomeA1()
{
	return g_Decoratives.grass;
}

function rBiomeA2()
{
	return g_Decoratives.grassShort;
}

function rBiomeA3()
{
	return g_Decoratives.reeds;
}

function rBiomeA4()
{
	return g_Decoratives.lillies;
}

function rBiomeA5()
{
	return g_Decoratives.rockLarge;
}

function rBiomeA6()
{
	return g_Decoratives.rockMedium;
}

function rBiomeA7()
{
	return g_Decoratives.bushMedium;
}

function rBiomeA8()
{
	return g_Decoratives.bushSmall;
}

function rBiomeA9()
{
	return g_Decoratives.tree;
}
