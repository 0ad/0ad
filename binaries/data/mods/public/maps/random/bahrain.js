/**
 * Heightmap image source:
 * Imagery by Jesse Allen, NASA's Earth Observatory,
 * using data from the General Bathymetric Chart of the Oceans (GEBCO)
 * produced by the British Oceanographic Data Centre.
 * https://visibleearth.nasa.gov/view.php?id=73934
 *
 * Licensing: Public Domain, https://visibleearth.nasa.gov/useterms.php
 *
 * The heightmap image is reproduced using:
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73934/gebco_08_rev_elev_C1_grey_geo.tif
 * lat=25.574723; lon=50.65; width=1.5;
 * lat1=$(bc <<< ";scale=5;$lat-$width/2"); lon1=$(bc <<< ";scale=5;$lon+$width/2"); lat2=$(bc <<< ";scale=5;$lat+$width/2"); lon2=$(bc <<< ";scale=5;$lon-$width/2")
 * gdal_translate -projwin $lon2 $lat2 $lon1 $lat1 gebco_08_rev_elev_C1_grey_geo.tif bahrain.tif
 * convert bahrain.tif -resize 512 -contrast-stretch 0 bahrain.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setBiome("generic/desert");
setLandBiome();

function setLandBiome()
{
	g_Terrains.mainTerrain = new Array(3).fill("desert_dirt_rough_2").concat(["desert_dirt_rocks_3", "desert_sand_stones"]);
	g_Terrains.forestFloor1 = "grass_dead";
	g_Terrains.forestFloor2 = "desert_dirt_persia_1";
	g_Terrains.tier1Terrain = "desert_sand_dunes_stones";
	g_Terrains.tier2Terrain = "desert_sand_scrub";
	g_Terrains.tier3Terrain = "desert_plants_b";
	g_Terrains.tier4Terrain = "medit_dirt_dry";
}

function setIslandBiome()
{
	g_Terrains.mainTerrain = "sand";
	g_Terrains.forestFloor1 = "desert_wave";
	g_Terrains.forestFloor2 = "desert_sahara";
	g_Terrains.tier1Terrain = "sand_scrub_25";
	g_Terrains.tier2Terrain = "sand_scrub_75";
	g_Terrains.tier3Terrain = "sand_scrub_50";
	g_Terrains.tier4Terrain = "sand";
}

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
g_Gaia.woodTreasure = "gaia/treasure/wood";
g_Gaia.foodTreasure = "gaia/treasure/food_bin";
g_Gaia.shipWreck = "gaia/treasure/shipwreck_sail_boat_cut";

g_Decoratives.grass = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.rockLarge = "actor|geology/stone_savanna_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_granite_greek_small.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_desert_dry_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_medit_la_dry";

const heightScale = num => num * g_MapSettings.Size / 320;

const heightSeaGround = heightScale(-6);
const heightWaterLevel = heightScale(0);
const heightShoreline = heightScale(0.5);

var g_Map = new RandomMap(0, g_Terrains.mainTerrain);
var mapBounds = g_Map.getBounds();
var mapCenter = g_Map.getCenter();
var numPlayers = getNumPlayers();

g_Map.LoadHeightmapImage("bahrain.png", 0, 15);
Engine.SetProgress(15);

initTileClasses(["island", "shoreline"]);

g_Map.log("Lowering sea ground");
createArea(
	new MapBoundsPlacer(),
	new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 2),
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(20);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, scaleByMapSize(0.1, 0.5), 1));
Engine.SetProgress(25);

g_Map.log("Marking water");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.water),
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(30);

g_Map.log("Marking land");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.land),
	avoidClasses(g_TileClasses.water, 0));
Engine.SetProgress(35);

g_Map.log("Marking island");
var areaIsland = createArea(
	new RectPlacer(new Vector2D(fractionToTiles(0.4), mapBounds.top), new Vector2D(fractionToTiles(0.6), mapCenter.y), Infinity),
	new TileClassPainter(g_TileClasses.island),
	avoidClasses(g_TileClasses.water, 0));
Engine.SetProgress(37);

g_Map.log("Painting shoreline");
createArea(
	new MapBoundsPlacer(),
	[
		new TerrainPainter(g_Terrains.water),
		new TileClassPainter(g_TileClasses.shoreline)
	],
	new HeightConstraint(-Infinity, heightShoreline));
Engine.SetProgress(40);

g_Map.log("Painting cliffs");
createArea(
	new MapBoundsPlacer(),
	[
		new TerrainPainter(g_Terrains.cliff),
		new TileClassPainter(g_TileClasses.mountain),
	],
	[
		avoidClasses(g_TileClasses.water, 2),
		new SlopeConstraint(2, Infinity)
	]);
Engine.SetProgress(45);

if (!isNomad())
{
	g_Map.log("Placing players");
	let [playerIDs, playerPosition] = createBases(
		...playerPlacementRandom(
			sortAllPlayers(),
			[
				avoidClasses(g_TileClasses.island, 5),
				stayClasses(g_TileClasses.land, defaultPlayerBaseRadius() / 2)
			]),
		"towers");

	g_Map.log("Flatten the initial CC area");
	for (let position of playerPosition)
		createArea(
			new ClumpPlacer(diskArea(defaultPlayerBaseRadius() * 0.8), 0.95, 0.6, Infinity, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 6));
}

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
			g_TileClasses.fish, 18,
			g_TileClasses.player, 8
		],
		"stay": [g_TileClasses.water, 4],
		"sizes": ["normal"],
		"mixes": ["same"],
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
			g_TileClasses.water, 5,
			g_TileClasses.island, 2
		],
		"sizes": ["small"],
		"mixes": ["same"],
		"amounts": ["normal"]
	}
]));
Engine.SetProgress(65);

g_Map.log("Painting island");
setIslandBiome();

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

g_Map.log("Creating island mines");
for (let i = 0; i < scaleByMapSize(4, 10); ++i)
	createObjectGroupsByAreas(
		randBool() ?
			new SimpleGroup([new SimpleObject(g_Gaia.metalLarge, 1, 1, 0, 4)], true, g_TileClasses.metal) :
			new SimpleGroup([new SimpleObject(g_Gaia.stoneLarge, 1, 1, 0, 4)], true, g_TileClasses.rock),
		0,
		[avoidClasses(g_TileClasses.rock, 8, g_TileClasses.metal, 8), stayClasses(g_TileClasses.island, 4)],
		1,
		40,
		[areaIsland]);

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

Engine.SetProgress(80);

g_Map.log("Adding more decoratives");
createObjectGroups(
	new SimpleGroup(
		[
			new SimpleObject("actor|props/special/eyecandy/awning_wood_small.xml", 1, 1, 1, 7),
			new SimpleObject("actor|props/special/eyecandy/barrels_buried.xml", 1, 2, 1, 7)
		],
		true,
		g_TileClasses.dirt),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 10,
		g_TileClasses.mountain, 2,
		g_TileClasses.forest, 2),
	2 * scaleByMapSize(1, 4),
	20);
Engine.SetProgress(85);

g_Map.log("Creating treasures");
for (let treasure of [g_Gaia.woodTreasure, g_Gaia.foodTreasure])
	createObjectGroups(
		new SimpleGroup([new SimpleObject(treasure, 1, 1, 0, 2)], true),
		0,
		avoidClasses(
			g_TileClasses.water, 2,
			g_TileClasses.player, 25,
			g_TileClasses.forest, 2),
		randIntInclusive(1, numPlayers),
		20);
Engine.SetProgress(90);

g_Map.log("Creating shipwrecks");
createObjectGroups(
	new SimpleGroup([new SimpleObject(g_Gaia.shipWreck, 1, 1, 0, 1)], true),
	0,
	stayClasses(g_TileClasses.water, 2),
	randIntInclusive(0, 1),
	20);
Engine.SetProgress(95);

placePlayersNomad(
	g_Map.createTileClass(),
	[
		stayClasses(g_TileClasses.land, 5),
		avoidClasses(
			g_TileClasses.island, 0,
			g_TileClasses.forest, 2,
			g_TileClasses.rock, 4,
			g_TileClasses.metal, 4,
			g_TileClasses.berries, 1,
			g_TileClasses.animals, 1)
	]);

setSunColor(0.733, 0.746, 0.574);
setSkySet("cloudless");

// Prevent the water from disappearing on the tiny mapsize while maximizing land on greater sizes
setWaterHeight(scaleByMapSize(20, 18));
setWaterTint(0.37, 0.67, 0.73);
setWaterColor(0.24, 0.44, 0.56);
setWaterWaviness(9);
setWaterMurkiness(0.8);
setWaterType("lake");

setTerrainAmbientColor(0.521, 0.475, 0.322);

setSunRotation(Math.PI);
setSunElevation(Math.PI / 6.25);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
