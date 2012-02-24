RMS.LoadLibrary("rmgen");


//random terrain textures
var rt = randInt(1,7);
//temperate
if (rt == 1){
var tGrass = ["temp_grass_long_b"];
var tGrassPForest = "temp_forestfloor_pine";
var tGrassDForest = "temp_plants_bog";
var tCliff = ["temp_cliff_a", "temp_cliff_b"];
var tGrassA = "temp_grass_d";
var tGrassB = "temp_grass_c";
var tGrassC = "temp_grass_clovers_2";
var tHill = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
var tDirt = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
var tRoad = "temp_road";
var tRoadWild = "temp_road_overgrown";
var tGrassPatch = "temp_grass_plants";
var tShoreBlend = "temp_mud_plants";
var tShore = "sand_grass_25";
var tWater = "medit_sand_wet";

// gaia entities
var oOak = "gaia/flora_tree_oak";
var oOakLarge = "gaia/flora_tree_oak_large";
var oApple = "gaia/flora_tree_apple";
var oPine = "gaia/flora_tree_pine";
var oAleppoPine = "gaia/flora_tree_aleppo_pine";
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_deer";
var oFish = "gaia/fauna_fish";
var oSheep = "gaia/fauna_sheep";
var oStoneLarge = "gaia/geology_stonemine_medit_quarry";
var oStoneSmall = "gaia/geology_stone_mediterranean";
var oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

// decorative props
var aGrass = "actor|props/flora/grass_soft_large_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_large.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
var aLillies = "actor|props/flora/pond_lillies_large.xml";
var aRockLarge = "actor|geology/stone_granite_large.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aBushMedium = "actor|props/flora/bush_medit_me.xml";
var aBushSmall = "actor|props/flora/bush_medit_sm.xml";

// terrain + entity (for painting)

}
//snowy
else if (rt == 2)
{
setSunColour(0.550, 0.601, 0.644);				// a little darker

var tGrass = ["polar_snow_b", "snow grass 75", "snow rocks", "snow forest"];
var tGrassPForest = "polar_tundra_snow";
var tGrassDForest = "polar_tundra_snow";
var tCliff = ["polar_cliff_a", "polar_cliff_b"];
var tGrassA = "snow grass 2";
var tGrassB = "polar_snow_a";
var tGrassC = "polar_ice_snow";
var tHill = ["polar_snow_rocks", "polar_cliff_snow"];
var tDirt = ["polar_ice_b", "polar_ice_c"];
var tRoad = "new_alpine_citytile";
var tRoadWild = "polar_ice_cracked";
var tGrassPatch = "snow grass 2";
var tShoreBlend = "polar_ice";
var tShore = "snow_glacial_01";
var tWater = "polar_ice_c";

// gaia entities
var oOak = "gaia/flora_tree_pine_w";
var oOakLarge = "gaia/flora_tree_pine_w";
var oApple = "gaia/flora_tree_pine_w";
var oPine = "gaia/flora_tree_pine_w";
var oAleppoPine = "gaia/flora_tree_pine";
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_muskox";
var oFish = "gaia/fauna_fish_tuna";
var oSheep = "gaia/fauna_walrus";
var oStoneLarge = "gaia/geology_stone_alpine_a";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_alpine";

// decorative props
var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
var aRockLarge = "actor|geology/stone_granite_large.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
var aBushSmall = "actor|props/flora/bush_desert_dry_a.xml";

// terrain + entity (for painting)

}
//desert
else if (rt == 3)
{
setSunColour(0.733, 0.746, 0.574);	

var tGrass = ["desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_dunes_50", "desert_sand_smooth"];
var tGrassPForest = "forestfloor_dirty";
var tGrassDForest = "desert_forestfloor_palms";
var tCliff = ["desert_cliff_1", "desert_cliff_2", "desert_cliff_3", "desert_cliff_4", "desert_cliff_5"];
var tGrassA = ["desert_dirt_persia_1", "desert_dirt_persia_2"];
var tGrassB = "dirta";
var tGrassC = "medit_dirt_dry";
var tHill = ["desert_dirt_rocks_1", "desert_dirt_rocks_2", "desert_dirt_rocks_3"];
var tDirt = ["desert_lakebed_dry", "desert_lakebed_dry_b"];
var tRoad = "desert_city_tile";
var tRoadWild = "desert_city_tile";
var tGrassPatch = "desert_dirt_rough";
var tShoreBlend = "desert_shore_stones";
var tShore = "dirta";
var tWater = "desert_sand_wet";

// gaia entities
var oOak = "gaia/flora_tree_cretan_date_palm_short";
var oOakLarge = "gaia/flora_tree_cretan_date_palm_tall";
var oApple = "gaia/flora_tree_fig";
var oPine = "gaia/flora_tree_dead";
var oAleppoPine = "gaia/flora_tree_date_palm";
var oBerryBush = "gaia/flora_bush_grapes";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_camel";
var oFish = "gaia/fauna_fish";
var oSheep = "gaia/fauna_gazelle";
var oStoneLarge = "gaia/geology_stonemine_desert_quarry";
var oStoneSmall = "gaia/geology_stone_desert_small";
var oMetalLarge = "gaia/geology_metal_desert_slabs";

// decorative props
var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
var aRockLarge = "actor|geology/stone_desert_med.xml";
var aRockMedium = "actor|geology/stone_desert_med.xml";
var aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
var aBushSmall = "actor|props/flora/bush_desert_dry_a.xml";
// terrain + entity (for painting)
}
//alpine
else if (rt == 4)
{
var tGrass = ["alpine_dirt_grass_50"];
var tGrassPForest = "alpine_forrestfloor";
var tGrassDForest = "alpine_forrestfloor";
var tCliff = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
var tGrassA = "alpine_grass_rocky";
var tGrassB = ["alpine_grass_snow_50", "alpine_dirt_snow"];
var tGrassC = ["alpine_snow_a", "alpine_snow_b"];
var tHill = "alpine_cliff_snow";
var tDirt = ["alpine_dirt", "alpine_grass_d"];
var tRoad = "new_alpine_citytile";
var tRoadWild = "new_alpine_citytile";
var tGrassPatch = "new_alpine_grass_a";
var tShoreBlend = "alpine_shore_rocks";
var tShore = "alpine_shore_rocks_grass_50";
var tWater = "alpine_shore_rocks";

// gaia entities
var oOak = "gaia/flora_tree_pine";
var oOakLarge = "gaia/flora_tree_pine";
var oApple = "gaia/flora_tree_pine";
var oPine = "gaia/flora_tree_pine";
var oAleppoPine = "gaia/flora_tree_pine";
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_goat";
var oFish = "gaia/fauna_fish_tuna";
var oSheep = "gaia/fauna_deer";
var oStoneLarge = "gaia/geology_stone_alpine_a";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_alpine";

// decorative props
var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
var aRockLarge = "actor|geology/stone_granite_large.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
var aBushSmall = "actor|props/flora/bush_desert_dry_a.xml";

// terrain + entity (for painting)
}
//medit
else if (rt == 5){
var tGrass = ["medit_grass_field_a", "medit_grass_field_b"];
var tGrassPForest = "medit_plants_dirt";
var tGrassDForest = "medit_grass_shrubs";
var tCliff = ["medit_cliff_grass", "medit_cliff_greek", "medit_cliff_greek_2", "medit_cliff_aegean", "medit_cliff_italia", "medit_cliff_italia_grass"];
var tGrassA = "medit_grass_field_b";
var tGrassB = "medit_grass_field_brown";
var tGrassC = "medit_grass_field_dry";
var tHill = ["medit_rocks_grass_shrubs", "medit_rocks_shrubs"];
var tDirt = ["medit_dirt", "medit_dirt_b"];
var tRoad = "medit_city_tile";
var tRoadWild = "medit_city_tile";
var tGrassPatch = "medit_grass_wild";
var tShoreBlend = "medit_sand";
var tShore = "sand_grass_25";
var tWater = "medit_sand_wet";

// gaia entities
var oOak = "gaia/flora_tree_cretan_date_palm_short";
var oOakLarge = "gaia/flora_tree_medit_fan_palm";
var oApple = "gaia/flora_tree_apple";
var oPine = "gaia/flora_tree_cypress";
var oAleppoPine = "gaia/flora_tree_aleppo_pine";
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_deer";
var oFish = "gaia/fauna_fish";
var oSheep = "gaia/fauna_sheep";
var oStoneLarge = "gaia/geology_stonemine_medit_quarry";
var oStoneSmall = "gaia/geology_stone_mediterranean";
var oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

// decorative props
var aGrass = "actor|props/flora/grass_soft_large_tall.xml";
var aGrassShort = "actor|props/flora/grass_soft_large.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
var aLillies = "actor|props/flora/pond_lillies_large.xml";
var aRockLarge = "actor|geology/stone_granite_large.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aBushMedium = "actor|props/flora/bush_medit_me.xml";
var aBushSmall = "actor|props/flora/bush_medit_sm.xml";

// terrain + entity (for painting)

}
//savanah
else if (rt == 6)
{
var tGrass = ["savanna_grass_a", "savanna_grass_b"];
var tGrassPForest = "savanna_forestfloor_a";
var tGrassDForest = "savanna_forestfloor_b";
var tCliff = ["savanna_cliff_a", "savanna_cliff_b"];
var tGrassA = "savanna_shrubs_a";
var tGrassB = "savanna_dirt_rocks_b";
var tGrassC = "dirt_brown_e";
var tHill = ["savanna_grass_a", "savanna_grass_b"];
var tDirt = ["savanna_dirt_rocks_b", "dirt_brown_e"];
var tRoad = "savanna_tile_a";
var tRoadWild = "savanna_tile_a";
var tGrassPatch = "savanna_grass_a";
var tShoreBlend = "savanna_riparian";
var tShore = "savanna_riparian_bank";
var tWater = "savanna_riparian_wet";

// gaia entities
var oOak = "gaia/flora_tree_baobab";
var oOakLarge = "gaia/flora_tree_baobab";
var oApple = "gaia/flora_tree_baobab";
var oPine = "gaia/flora_tree_baobab";
var oAleppoPine = "gaia/flora_tree_baobab";
var oBerryBush = "gaia/flora_bush_grapes";
var oChicken = "gaia/fauna_chicken";
var rts = randInt(1,4);
if (rts==1){
var oDeer = "gaia/fauna_wildebeest";
}
else if (rts==2)
{
var oDeer = "gaia/fauna_zebra";
}
else if (rts==3)
{
var oDeer = "gaia/fauna_giraffe";
}
else if (rts==4)
{
var oDeer = "gaia/fauna_elephant_african_bush";
}
var oFish = "gaia/fauna_fish";
var oSheep = "gaia/fauna_gazelle";
var oStoneLarge = "gaia/geology_stonemine_desert_quarry";
var oStoneSmall = "gaia/geology_stone_savanna_small";
var oMetalLarge = "gaia/geology_metal_savanna_slabs";

// decorative props
var aGrass = "actor|props/flora/grass_savanna.xml";
var aGrassShort = "actor|props/flora/grass_medit_field.xml";
var aRockLarge = "actor|geology/stone_savanna_med.xml";
var aRockMedium = "actor|geology/stone_savanna_med.xml";
var aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
var aBushSmall = "actor|props/flora/bush_dry_a.xml";
// terrain + entity (for painting)
}
//tropic
else if (rt == 7){
var tGrass = ["tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_plants", "tropic_plants", "tropic_plants_b"];
var tGrassPForest = "tropic_plants_c";
var tGrassDForest = "tropic_plants_c";
var tCliff = ["tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a_plants"];
var tGrassA = "tropic_grass_c";
var tGrassB = "tropic_grass_plants";
var tGrassC = "tropic_plants";
var tHill = ["tropic_cliff_grass"];
var tDirt = ["tropic_dirt_a", "tropic_dirt_a_plants"];
var tRoad = "tropic_citytile_a";
var tRoadWild = "tropic_citytile_plants";
var tGrassPatch = "tropic_plants_b";
var tShoreBlend = "temp_mud_plants";
var tShore = "tropic_beach_dry";
var tWater = "tropic_beach_wet";

// gaia entities
var oOak = "gaia/flora_tree_poplar";
var oOakLarge = "gaia/flora_tree_poplar";
var oApple = "gaia/flora_tree_poplar";
var oPine = "gaia/flora_tree_cretan_date_palm_short";
var oAleppoPine = "gaia/flora_tree_cretan_date_palm_tall";
var oBerryBush = "gaia/flora_bush_berry";
var oChicken = "gaia/fauna_chicken";
var oDeer = "gaia/fauna_deer";
var oFish = "gaia/fauna_fish";
var oSheep = "gaia/fauna_tiger";
var oStoneLarge = "gaia/geology_stonemine_tropic_quarry";
var oStoneSmall = "gaia/geology_stone_tropic_a";
var oMetalLarge = "gaia/geology_metal_tropic_slabs";

// decorative props
var aGrass = "actor|props/flora/plant_tropic_a.xml";
var aGrassShort = "actor|props/flora/plant_lg.xml";
var aReeds = "actor|props/flora/reeds_pond_lush_b.xml";
var aLillies = "actor|props/flora/water_lillies.xml";
var aRockLarge = "actor|geology/stone_granite_large.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aBushMedium = "actor|props/flora/plant_tropic_large.xml";
var aBushSmall = "actor|props/flora/plant_tropic_large.xml";

// terrain + entity (for painting)

}

var pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oOak, tGrassDForest + TERRAIN_SEPARATOR + oOakLarge, tGrassDForest];
var pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oPine, tGrassPForest + TERRAIN_SEPARATOR + oAleppoPine, tGrassPForest];
const BUILDING_ANGlE = 0.75*PI;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tGrass);
	}
}

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.09 * lSize, 0.7, 0.1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater, tWater],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	-3,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = shuffleArray(playerIDs);

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat(0, TWO_PI);
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*TWO_PI/numPlayers;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
	
	// get civ specific starting entities
	var civEntities = getStartingEntities(id-1);
	
	// create the TC
	var group = new SimpleGroup(	// elements (type, min/max count, min/max distance, min/max angle)
		[new SimpleObject(civEntities[0].Template, 1,1, 0,0, BUILDING_ANGlE, BUILDING_ANGlE)],
		true, null, ix, iz
	);
	createObjectGroup(group, id);
	
	// create starting units
	var uDist = 6;
	var uSpace = 2;
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = -BUILDING_ANGlE + PI * (j - 1) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, id, (j % 2 - 1) * PI + uAngle);
		}
	}
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		group = new SimpleGroup(
			[new SimpleObject(oChicken, 5,5, 0,3)],
			true, clBaseResource, aX, aZ
		);
		createObjectGroup(group, 0);
	}
	
	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = radius - 4;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting straggler trees
	var num = hillSize / 100;
	for (var j = 0; j < num; j++)
	{
		var tAngle = randFloat(0, TWO_PI);
		var tDist = randFloat(6, radius - 2);
		var tX = round(fx + tDist * cos(tAngle));
		var tZ = round(fz + tDist * sin(tAngle));
		group = new SimpleGroup(
			[new SimpleObject(oOak, 1,3, 0,2)],
			false, clBaseResource, tX, tZ
		);
		createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	}
	
	// create grass tufts
	var num = hillSize / 250;
	for (var j = 0; j < num; j++)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = radius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aGrassShort, 2,5, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gZ
		);
		createObjectGroup(group, 0);
	}
}

RMS.SetProgress(20);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 10),
	scaleByMapSize(100, 200)
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tGrass, tCliff, tHill],		// terrains
	[1, 2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 12, clHill, 15, clWater, 2),
	scaleByMapSize(1, 4) * numPlayers
);


// calculate desired number of trees for map (based on size)
if (rt == 6)
{
var MIN_TREES = 200;
var MAX_TREES = 1250;
var P_FOREST = 0.02;
}
else if (rt == 7)
{
var MIN_TREES = 1000;
var MAX_TREES = 6000;
var P_FOREST = 0.6;
}
else
{
var MIN_TREES = 500;
var MAX_TREES = 3000;
var P_FOREST = 0.7;
}
var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]],
	[[tGrassPForest, tGrass, pForestP], [tGrassPForest, pForestP]]
];	// some variation

if (rt == 6)
{
var size = numForest / (0.5 * scaleByMapSize(2,8) * numPlayers);
}
else
{
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
}
var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0, clWater, 2),
		num
	);
}

RMS.SetProgress(50);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(55);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(65);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);


// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oOakLarge, oPine, oApple];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}

var planetm = 1;
if (rt==7)
{
	planetm = 8;
}
//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1, clDirt, 1),
	planetm * scaleByMapSize(13, 200), 50
);

rt = randInt(1,3)
if (rt==1){
setSkySet("cirrus");
}
else if (rt ==2){
setSkySet("cumulus");
}
else if (rt ==3){
setSkySet("sunny");
}
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterTint(0.447, 0.412, 0.322);				// muddy brown
setWaterReflectionTint(0.447, 0.412, 0.322);	// muddy brown
setWaterMurkiness(1.0);
setWaterReflectionTintStrength(0.677);

// Export map data

ExportMap();