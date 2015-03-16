/////////////////////////////////////////////////////////////////////////////////////////////
//	definitions
/////////////////////////////////////////////////////////////////////////////////////////////

var biomeID = 1;
var rbt1 = ["temp_grass_long_b"];
var rbt2 = "temp_forestfloor_pine";
var rbt3 = "temp_plants_bog";
var rbt4 = ["temp_cliff_a", "temp_cliff_b"];
var rbt5 = "temp_grass_d";
var rbt6 = "temp_grass_c";
var rbt7 = "temp_grass_clovers_2";
var rbt8 = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
var rbt9 = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
var rbt10 = "temp_road";
var rbt11 = "temp_road_overgrown";
var rbt12 = "temp_grass_plants";
var rbt13 = "temp_mud_plants";
var rbt14 = "sand_grass_25";
var rbt15 = "medit_sand_wet";

// gaia entities
var rbe1 = "gaia/flora_tree_oak";
var rbe2 = "gaia/flora_tree_oak_large";
var rbe3 = "gaia/flora_tree_apple";
var rbe4 = "gaia/flora_tree_pine";
var rbe5 = "gaia/flora_tree_aleppo_pine";
var rbe6 = "gaia/flora_bush_berry";
var rbe7 = "gaia/fauna_chicken";
var rbe8 = "gaia/fauna_deer";
var rbe9 = "gaia/fauna_fish";
var rbe10 = "gaia/fauna_sheep";
var rbe11 = "gaia/geology_stonemine_medit_quarry";
var rbe12 = "gaia/geology_stone_mediterranean";
var rbe13 = "gaia/geology_metal_mediterranean_slabs";

// decorative props
var rba1 = "actor|props/flora/grass_soft_large_tall.xml";
var rba2 = "actor|props/flora/grass_soft_large.xml";
var rba3 = "actor|props/flora/reeds_pond_lush_a.xml";
var rba4 = "actor|props/flora/pond_lillies_large.xml";
var rba5 = "actor|geology/stone_granite_large.xml";
var rba6 = "actor|geology/stone_granite_med.xml";
var rba7 = "actor|props/flora/bush_medit_me.xml";
var rba8 = "actor|props/flora/bush_medit_sm.xml";
var rba9 = "actor|flora/trees/oak.xml";


/////////////////////////////////////////////////////////////////////////////////////////////
//	randomizeBiome: randomizes the biomeID and returns the value
/////////////////////////////////////////////////////////////////////////////////////////////

function randomizeBiome()
{

	var random_sky = randInt(1,3)
	if (random_sky==1)
		setSkySet("cirrus");
	else if (random_sky ==2)
		setSkySet("cumulus");
	else if (random_sky ==3)
		setSkySet("sunny");
	setSunRotation(randFloat(0, TWO_PI));
	setSunElevation(randFloat(PI/ 6, PI / 3));

	setUnitsAmbientColor(0.57, 0.58, 0.55);
	setTerrainAmbientColor(0.447059, 0.509804, 0.54902);
	
	biomeID = randInt(1,8);
	//temperate
	if (biomeID == 1){
		
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
		
		if (randInt(2))
		{
			rbt1 = "alpine_grass";
			rbt2 = "temp_forestfloor_pine";
			rbt3 = "temp_grass_clovers_2";
			rbt5 = "alpine_grass_a";
			rbt6 = "alpine_grass_b";
			rbt7 = "alpine_grass_c";
			rbt12 = "temp_grass_mossy";
		}
		else
		{
			rbt1 = "temp_grass_long_b";
			rbt2 = "temp_forestfloor_pine";
			rbt3 = "temp_plants_bog";
			rbt5 = "temp_grass_d";
			rbt6 = "temp_grass_c";
			rbt7 = "temp_grass_clovers_2";
			rbt12 = "temp_grass_plants";
		}
		
		rbt4 = ["temp_cliff_a", "temp_cliff_b"];
		
		rbt8 = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
		rbt9 = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
		rbt10 = "temp_road";
		rbt11 = "temp_road_overgrown";
		
		rbt13 = "temp_mud_plants";
		rbt14 = "sand_grass_25";
		rbt15 = "medit_sand_wet";

		// gaia entities
		var random_trees = randInt(3);
		
		if (random_trees == 0)
		{
			rbe1 = "gaia/flora_tree_oak";
			rbe2 = "gaia/flora_tree_oak_large";
		}
		else if (random_trees == 1)
		{
			rbe1 = "gaia/flora_tree_poplar";
			rbe2 = "gaia/flora_tree_poplar";
		}
		else
		{
			rbe1 = "gaia/flora_tree_euro_beech";
			rbe2 = "gaia/flora_tree_euro_beech";
		}
		rbe3 = "gaia/flora_tree_apple";
		random_trees = randInt(3);
		if (random_trees == 0)
		{
			rbe4 = "gaia/flora_tree_pine";
			rbe5 = "gaia/flora_tree_aleppo_pine";
		}
		else if (random_trees == 1)
		{
			rbe4 = "gaia/flora_tree_pine";
			rbe5 = "gaia/flora_tree_pine";
		}
		else
		{
			rbe4 = "gaia/flora_tree_aleppo_pine";
			rbe5 = "gaia/flora_tree_aleppo_pine";
		}
		rbe6 = "gaia/flora_bush_berry";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_deer";
		rbe9 = "gaia/fauna_fish";
		rbe10 = "gaia/fauna_sheep";
		rbe11 = "gaia/geology_stonemine_temperate_quarry";
		rbe12 = "gaia/geology_stone_temperate";
		rbe13 = "gaia/geology_metal_temperate_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_soft_large_tall.xml";
		rba2 = "actor|props/flora/grass_soft_large.xml";
		rba3 = "actor|props/flora/reeds_pond_lush_a.xml";
		rba4 = "actor|props/flora/water_lillies.xml";
		rba5 = "actor|geology/stone_granite_large.xml";
		rba6 = "actor|geology/stone_granite_med.xml";
		rba7 = "actor|props/flora/bush_medit_me.xml";
		rba8 = "actor|props/flora/bush_medit_sm.xml";
		rba9 = "actor|flora/trees/oak.xml";
	}
	//snowy
	else if (biomeID == 2)
	{
		setSunColor(0.550, 0.601, 0.644);				// a little darker
		// Water is a semi-deep blue, fairly wavy, fairly murky for an ocean.
		// this assumes ocean settings, maps that aren't oceans should reset.
		setWaterColor(0.067, 0.212, 0.361);
		setWaterTint(0.4, 0.486, 0.765);
		setWaterWaviness(5.5);
		setWaterMurkiness(0.83);

		rbt1 = ["polar_snow_b", "snow grass 75", "snow rocks", "snow forest"];
		rbt2 = "polar_tundra_snow";
		rbt3 = "polar_tundra_snow";
		rbt4 = ["alpine_cliff_a", "alpine_cliff_b"];
		rbt5 = "polar_snow_a";
		rbt6 = "polar_ice_snow";
		rbt7 = "polar_ice";
		rbt8 = ["polar_snow_rocks", "polar_cliff_snow"];
		rbt9 = "snow grass 2";
		rbt10 = "new_alpine_citytile";
		rbt11 = "polar_ice_cracked";
		rbt12 = "snow grass 2";
		rbt13 = "polar_ice";
		rbt14 = "alpine_shore_rocks_icy";
		rbt15 = "alpine_shore_rocks";

		// gaia entities
		rbe1 = "gaia/flora_tree_pine_w";
		rbe2 = "gaia/flora_tree_pine_w";
		rbe3 = "gaia/flora_tree_pine_w";
		rbe4 = "gaia/flora_tree_pine_w";
		rbe5 = "gaia/flora_tree_pine";
		rbe6 = "gaia/flora_bush_berry";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_muskox";
		rbe9 = "gaia/fauna_fish_tuna";
		rbe10 = "gaia/fauna_walrus";
		rbe11 = "gaia/geology_stonemine_alpine_quarry";
		rbe12 = "gaia/geology_stone_alpine_a";
		rbe13 = "gaia/geology_metal_alpine_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_soft_dry_small_tall.xml";
		rba2 = "actor|props/flora/grass_soft_dry_large.xml";
		rba3 = "actor|props/flora/reeds_pond_dry.xml";
		rba4 = "actor|geology/stone_granite_large.xml";
		rba5 = "actor|geology/stone_granite_large.xml";
		rba6 = "actor|geology/stone_granite_med.xml";
		rba7 = "actor|props/flora/bush_desert_dry_a.xml";
		rba8 = "actor|props/flora/bush_desert_dry_a.xml";
		rba9 = "actor|flora/trees/pine_w.xml";
			
		setFogFactor(0.6);
		setFogThickness(0.21);
		setPPSaturation(0.37);
		setPPEffect("hdr");
	}
	//desert
	else if (biomeID == 3)
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
		
		rbt1 = ["desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_dunes_50", "desert_sand_smooth"];
		rbt2 = "forestfloor_dirty";
		rbt3 = "desert_forestfloor_palms";
		rbt4 = ["desert_cliff_1", "desert_cliff_2", "desert_cliff_3", "desert_cliff_4", "desert_cliff_5"];
		rbt5 = "desert_dirt_rough";
		rbt6 = "desert_dirt_rocks_1";
		rbt7 = "desert_dirt_rocks_2";
		rbt8 = ["desert_dirt_rocks_1", "desert_dirt_rocks_2", "desert_dirt_rocks_3"];
		rbt9 = ["desert_lakebed_dry", "desert_lakebed_dry_b"];
		rbt10 = "desert_city_tile";
		rbt11 = "desert_city_tile";
		rbt12 = "desert_dirt_rough";
		rbt13 = "desert_shore_stones";
		rbt14 = "desert_sand_smooth";
		rbt15 = "desert_sand_wet";

		// gaia entities
		if (randInt(2))
		{
			rbe1 = "gaia/flora_tree_cretan_date_palm_short";
			rbe2 = "gaia/flora_tree_cretan_date_palm_tall";
		}
		else
		{
			rbe1 = "gaia/flora_tree_date_palm";
			rbe2 = "gaia/flora_tree_date_palm";
		}
		rbe3 = "gaia/flora_tree_fig";
		if (randInt(2))
		{
			rbe4 = "gaia/flora_tree_tamarix";
			rbe5 = "gaia/flora_tree_tamarix";
		}
		else
		{
			rbe4 = "gaia/flora_tree_senegal_date_palm";
			rbe5 = "gaia/flora_tree_senegal_date_palm";
		}
		rbe6 = "gaia/flora_bush_grapes";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_camel";
		rbe9 = "gaia/fauna_fish";
		rbe10 = "gaia/fauna_gazelle";
		rbe11 = "gaia/geology_stonemine_desert_quarry";
		rbe12 = "gaia/geology_stone_desert_small";
		rbe13 = "gaia/geology_metal_desert_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_soft_dry_small_tall.xml";
		rba2 = "actor|props/flora/grass_soft_dry_large.xml";
		rba3 = "actor|props/flora/reeds_pond_lush_a.xml";
		rba4 = "actor|props/flora/reeds_pond_lush_b.xml";
		rba5 = "actor|geology/stone_desert_med.xml";
		rba6 = "actor|geology/stone_desert_med.xml";
		rba7 = "actor|props/flora/bush_desert_dry_a.xml";
		rba8 = "actor|props/flora/bush_desert_dry_a.xml";
		rba9 = "actor|flora/trees/palm_date.xml";
	}
	//alpine
	else if (biomeID == 4)
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
		
		rbt1 = ["alpine_dirt_grass_50"];
		rbt2 = "alpine_forrestfloor";
		rbt3 = "alpine_forrestfloor";
		rbt4 = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
		rbt5 = "alpine_dirt";
		rbt6 = ["alpine_grass_snow_50", "alpine_dirt_snow", "alpine_dirt_snow"];
		rbt7 = ["alpine_snow_a", "alpine_snow_b"];
		rbt8 = "alpine_cliff_snow";
		rbt9 = ["alpine_dirt", "alpine_grass_d"];
		rbt10 = "new_alpine_citytile";
		rbt11 = "new_alpine_citytile";
		rbt12 = "new_alpine_grass_a";
		rbt13 = "alpine_shore_rocks";
		rbt14 = "alpine_shore_rocks_grass_50";
		rbt15 = "alpine_shore_rocks";

		// gaia entities
		rbe1 = "gaia/flora_tree_pine";
		rbe2 = "gaia/flora_tree_pine";
		rbe3 = "gaia/flora_tree_pine";
		rbe4 = "gaia/flora_tree_pine";
		rbe5 = "gaia/flora_tree_pine";
		rbe6 = "gaia/flora_bush_berry";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_goat";
		rbe9 = "gaia/fauna_fish_tuna";
		rbe10 = "gaia/fauna_deer";
		rbe11 = "gaia/geology_stonemine_alpine_quarry";
		rbe12 = "gaia/geology_stone_alpine_a";
		rbe13 = "gaia/geology_metal_alpine_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_soft_small_tall.xml";
		rba2 = "actor|props/flora/grass_soft_large.xml";
		rba3 = "actor|props/flora/reeds_pond_dry.xml";
		rba4 = "actor|geology/stone_granite_large.xml";
		rba5 = "actor|geology/stone_granite_large.xml";
		rba6 = "actor|geology/stone_granite_med.xml";
		rba7 = "actor|props/flora/bush_desert_a.xml";
		rba8 = "actor|props/flora/bush_desert_a.xml";
		rba9 = "actor|flora/trees/pine.xml";
	}
	//medit
	else if (biomeID == 5)
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
		
		rbt1 = ["medit_grass_field_a", "medit_grass_field_b"];
		rbt2 = "medit_grass_field";
		rbt3 = "medit_grass_shrubs";
		rbt4 = ["medit_cliff_grass", "medit_cliff_greek", "medit_cliff_greek_2", "medit_cliff_aegean", "medit_cliff_italia", "medit_cliff_italia_grass"];
		rbt5 = "medit_grass_field_b";
		rbt6 = "medit_grass_field_brown";
		rbt7 = "medit_grass_field_dry";
		rbt8 = ["medit_rocks_grass_shrubs", "medit_rocks_shrubs"];
		rbt9 = ["medit_dirt", "medit_dirt_b"];
		rbt10 = "medit_city_tile";
		rbt11 = "medit_city_tile";
		rbt12 = "medit_grass_wild";
		rbt13 = "medit_sand";
		rbt14 = "sand_grass_25";
		rbt15 = "medit_sand_wet";

		// gaia entities
		var random_trees = randInt(3);
		
		if (random_trees == 0)
		{
			rbe1 = "gaia/flora_tree_cretan_date_palm_short";
			rbe2 = "gaia/flora_tree_cretan_date_palm_tall";
		}
		else if (random_trees == 1)
		{
			rbe1 = "gaia/flora_tree_carob";
			rbe2 = "gaia/flora_tree_carob";
		}
		else
		{
			rbe1 = "gaia/flora_tree_medit_fan_palm";
			rbe2 = "gaia/flora_tree_medit_fan_palm";
		}
		
		if (randInt(2))
		{
			rbe3 = "gaia/flora_tree_apple";
		}
		else
		{
			rbe3 = "gaia/flora_tree_poplar_lombardy";
		}
		
		if (randInt(2))
		{
			rbe4 = "gaia/flora_tree_cypress";
			rbe5 = "gaia/flora_tree_cypress";
		}
		else
		{
			rbe4 = "gaia/flora_tree_aleppo_pine";
			rbe5 = "gaia/flora_tree_aleppo_pine";
		}
		
		if (randInt(2))
			rbe6 = "gaia/flora_bush_berry";
		else
			rbe6 = "gaia/flora_bush_grapes";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_deer";
		rbe9 = "gaia/fauna_fish";
		rbe10 = "gaia/fauna_sheep";
		rbe11 = "gaia/geology_stonemine_medit_quarry";
		rbe12 = "gaia/geology_stone_mediterranean";
		rbe13 = "gaia/geology_metal_mediterranean_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_soft_large_tall.xml";
		rba2 = "actor|props/flora/grass_soft_large.xml";
		rba3 = "actor|props/flora/reeds_pond_lush_b.xml";
		rba4 = "actor|props/flora/water_lillies.xml";
		rba5 = "actor|geology/stone_granite_large.xml";
		rba6 = "actor|geology/stone_granite_med.xml";
		rba7 = "actor|props/flora/bush_medit_me.xml";
		rba8 = "actor|props/flora/bush_medit_sm.xml";
		rba9 = "actor|flora/trees/palm_cretan_date.xml";
	}
	//savanah
	else if (biomeID == 6)
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
		
		rbt1 = ["savanna_grass_a", "savanna_grass_b"];
		rbt2 = "savanna_forestfloor_a";
		rbt3 = "savanna_forestfloor_b";
		rbt4 = ["savanna_cliff_a", "savanna_cliff_b"];
		rbt5 = "savanna_shrubs_a";
		rbt6 = "savanna_dirt_rocks_b";
		rbt7 = "savanna_dirt_rocks_a";
		rbt8 = ["savanna_grass_a", "savanna_grass_b"];
		rbt9 = ["savanna_dirt_rocks_b", "dirt_brown_e"];
		rbt10 = "savanna_tile_a";
		rbt11 = "savanna_tile_a";
		rbt12 = "savanna_grass_a";
		rbt13 = "savanna_riparian";
		rbt14 = "savanna_riparian_bank";
		rbt15 = "savanna_riparian_wet";

		// gaia entities
		rbe1 = "gaia/flora_tree_baobab";
		rbe2 = "gaia/flora_tree_baobab";
		rbe3 = "gaia/flora_tree_baobab";
		rbe4 = "gaia/flora_tree_baobab";
		rbe5 = "gaia/flora_tree_baobab";
		rbe6 = "gaia/flora_bush_grapes";
		rbe7 = "gaia/fauna_chicken";
		var rts = randInt(1,4);
		if (rts==1){
			rbe8 = "gaia/fauna_wildebeest";
		}
		else if (rts==2)
		{
			rbe8 = "gaia/fauna_zebra";
		}
		else if (rts==3)
		{
			rbe8 = "gaia/fauna_giraffe";
		}
		else if (rts==4)
		{
			rbe8 = "gaia/fauna_elephant_african_bush";
		}
		rbe9 = "gaia/fauna_fish";
		rbe10 = "gaia/fauna_gazelle";
		rbe11 = "gaia/geology_stonemine_desert_quarry";
		rbe12 = "gaia/geology_stone_savanna_small";
		rbe13 = "gaia/geology_metal_savanna_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_savanna.xml";
		rba2 = "actor|props/flora/grass_medit_field.xml";
		rba3 = "actor|props/flora/reeds_pond_lush_a.xml";
		rba4 = "actor|props/flora/reeds_pond_lush_b.xml";
		rba5 = "actor|geology/stone_savanna_med.xml";
		rba6 = "actor|geology/stone_savanna_med.xml";
		rba7 = "actor|props/flora/bush_desert_dry_a.xml";
		rba8 = "actor|props/flora/bush_dry_a.xml";
		rba9 = "actor|flora/trees/baobab.xml";
	}
	//tropic
	else if (biomeID == 7)
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
		
		rbt1 = ["tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_plants", "tropic_plants", "tropic_plants_b"];
		rbt2 = "tropic_plants_c";
		rbt3 = "tropic_plants_c";
		rbt4 = ["tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a_plants"];
		rbt5 = "tropic_grass_c";
		rbt6 = "tropic_grass_plants";
		rbt7 = "tropic_plants";
		rbt8 = ["tropic_cliff_grass"];
		rbt9 = ["tropic_dirt_a", "tropic_dirt_a_plants"];
		rbt10 = "tropic_citytile_a";
		rbt11 = "tropic_citytile_plants";
		rbt12 = "tropic_plants_b";
		rbt13 = "temp_mud_plants";
		rbt14 = "tropic_beach_dry_plants";
		rbt15 = "tropic_beach_dry";

		// gaia entities
		rbe1 = "gaia/flora_tree_toona";
		rbe2 = "gaia/flora_tree_toona";
		rbe3 = "gaia/flora_tree_palm_tropic";
		rbe4 = "gaia/flora_tree_palm_tropic";
		rbe5 = "gaia/flora_tree_palm_tropic";
		rbe6 = "gaia/flora_bush_berry";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_peacock";
		rbe9 = "gaia/fauna_fish";
		rbe10 = "gaia/fauna_tiger";
		rbe11 = "gaia/geology_stonemine_tropic_quarry";
		rbe12 = "gaia/geology_stone_tropic_a";
		rbe13 = "gaia/geology_metal_tropic_slabs";

		// decorative props
		rba1 = "actor|props/flora/plant_tropic_a.xml";
		rba2 = "actor|props/flora/plant_lg.xml";
		rba3 = "actor|props/flora/reeds_pond_lush_b.xml";
		rba4 = "actor|props/flora/water_lillies.xml";
		rba5 = "actor|geology/stone_granite_large.xml";
		rba6 = "actor|geology/stone_granite_med.xml";
		rba7 = "actor|props/flora/plant_tropic_large.xml";
		rba8 = "actor|props/flora/plant_tropic_large.xml";
		rba9 = "actor|flora/trees/tree_tropic.xml";
	}
	//autumn
	else if (biomeID == 8)
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
		
		rbt1 = ["temp_grass_aut", "temp_grass_aut", "temp_grass_d_aut"];
		rbt2 = "temp_plants_bog_aut";
		rbt3 = "temp_forestfloor_aut";
		rbt4 = ["temp_cliff_a", "temp_cliff_b"];
		rbt5 = "temp_grass_plants_aut";
		rbt6 = ["temp_grass_b_aut", "temp_grass_c_aut"];
		rbt7 = ["temp_grass_b_aut", "temp_grass_long_b_aut"];
		rbt8 = "temp_highlands_aut";
		rbt9 = ["temp_cliff_a", "temp_cliff_b"];
		rbt10 = "temp_road_aut";
		rbt11 = "temp_road_overgrown_aut";
		rbt12 = "temp_grass_plants_aut";
		rbt13 = "temp_grass_plants_aut";
		rbt14 = "temp_forestfloor_pine";
		rbt15 = "medit_sand_wet";

		// gaia entities
		rbe1 = "gaia/flora_tree_euro_beech_aut";
		rbe2 = "gaia/flora_tree_euro_beech_aut";
		rbe3 = "gaia/flora_tree_pine";
		rbe4 = "gaia/flora_tree_oak_aut";
		rbe5 = "gaia/flora_tree_oak_aut";
		rbe6 = "gaia/flora_bush_berry";
		rbe7 = "gaia/fauna_chicken";
		rbe8 = "gaia/fauna_deer";
		rbe9 = "gaia/fauna_fish";
		rbe10 = "gaia/fauna_rabbit";
		rbe11 = "gaia/geology_stonemine_temperate_quarry";
		rbe12 = "gaia/geology_stone_temperate";
		rbe13 = "gaia/geology_metal_temperate_slabs";

		// decorative props
		rba1 = "actor|props/flora/grass_soft_dry_small_tall.xml";
		rba2 = "actor|props/flora/grass_soft_dry_large.xml";
		rba3 = "actor|props/flora/reeds_pond_dry.xml";
		rba4 = "actor|geology/stone_granite_large.xml";
		rba5 = "actor|geology/stone_granite_large.xml";
		rba6 = "actor|geology/stone_granite_med.xml";
		rba7 = "actor|props/flora/bush_desert_dry_a.xml";
		rba8 = "actor|props/flora/bush_desert_dry_a.xml";
		rba9 = "actor|flora/trees/european_beech_aut.xml";
	}
	return biomeID;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//	These functions return the values needed for a randomized biome
/////////////////////////////////////////////////////////////////////////////////////////////

function rBiomeT1()
{
	return rbt1;
}

function rBiomeT2()
{
	return rbt2;
}

function rBiomeT3()
{
	return rbt3;
}

function rBiomeT4()
{
	return rbt4;
}

function rBiomeT5()
{
	return rbt5;
}

function rBiomeT6()
{
	return rbt6;
}

function rBiomeT7()
{
	return rbt7;
}

function rBiomeT8()
{
	return rbt8;
}

function rBiomeT9()
{
	return rbt9;
}

function rBiomeT10()
{
	return rbt10;
}

function rBiomeT11()
{
	return rbt11;
}

function rBiomeT12()
{
	return rbt12;
}

function rBiomeT13()
{
	return rbt13;
}

function rBiomeT14()
{
	return rbt14;
}

function rBiomeT15()
{
	return rbt15;
}

function rBiomeE1()
{
	return rbe1;
}

function rBiomeE2()
{
	return rbe2;
}

function rBiomeE3()
{
	return rbe3;
}

function rBiomeE4()
{
	return rbe4;
}

function rBiomeE5()
{
	return rbe5;
}

function rBiomeE6()
{
	return rbe6;
}

function rBiomeE7()
{
	return rbe7;
}

function rBiomeE8()
{
	return rbe8;
}

function rBiomeE9()
{
	return rbe9;
}

function rBiomeE10()
{
	return rbe10;
}

function rBiomeE11()
{
	return rbe11;
}

function rBiomeE12()
{
	return rbe12;
}

function rBiomeE13()
{
	return rbe13;
}

function rBiomeA1()
{
	return rba1;
}

function rBiomeA2()
{
	return rba2;
}

function rBiomeA3()
{
	return rba3;
}

function rBiomeA4()
{
	return rba4;
}

function rBiomeA5()
{
	return rba5;
}

function rBiomeA6()
{
	return rba6;
}

function rBiomeA7()
{
	return rba7;
}

function rBiomeA8()
{
	return rba8;
}

function rBiomeA9()
{
	return rba9;
}