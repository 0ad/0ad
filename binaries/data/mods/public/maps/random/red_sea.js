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
 * lat=22; lon=41; width=30;
 * gdal_translate -projwin $((lon-width/2)) $((lat+width/2)) $((lon+width/2)) $((lat-width/2)) gebco_08_rev_elev_C1_grey_geo.tif red_sea.tif
 * convert red_sea.tif -resize 512 -contrast-stretch 0 red_sea.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

TILE_CENTERED_HEIGHT_MAP = true;

setBiome("generic/desert");

g_Terrains.mainTerrain = new Array(4).fill("desert_sand_dunes_50").concat(["desert_sand_dunes_rocks", "desert_dirt_rough_2"]);
g_Terrains.forestFloor1 = "desert_grass_a_sand";
g_Terrains.cliff = "desert_cliff_3_dirty";
g_Terrains.forestFloor2 = "desert_grass_a_sand";
g_Terrains.tier1Terrain = "desert_dirt_rocks_2";
g_Terrains.tier2Terrain = "desert_dirt_rough";
g_Terrains.tier3Terrain = "desert_dirt_rough";
g_Terrains.tier4Terrain = "desert_sand_stones";
g_Terrains.roadWild = "road2";
g_Terrains.road = "road2";
g_Terrains.additionalDirt1 = "desert_plants_b";
g_Terrains.additionalDirt2 = "desert_sand_scrub";
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

const heightScale = num => num * g_MapSettings.Size / 320;

const heightSeaGround = heightScale(-4);
const heightReedsMin = heightScale(-2);
const heightReedsMax = heightScale(-0.5);
const heightWaterLevel = heightScale(0);
const heightShoreline = heightScale(0.5);
const heightHills = heightScale(16);

var g_Map = new RandomMap(0, g_Terrains.mainTerrain);
var mapCenter = g_Map.getCenter();

initTileClasses(["shoreline"]);

g_Map.LoadHeightmapImage("red_sea.png", 0, 25);
Engine.SetProgress(15);

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
	new DiskPlacer(fractionToTiles(0.5), mapCenter),
	new TileClassPainter(g_TileClasses.land),
	avoidClasses(g_TileClasses.water, 0));
Engine.SetProgress(35);

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
				avoidClasses(g_TileClasses.mountain, scaleByMapSize(5, 10)),
				stayClasses(g_TileClasses.land, defaultPlayerBaseRadius())
			]),
		true);

	g_Map.log("Flatten the initial CC area...");
	for (let position of playerPosition)
		createArea(
			new ClumpPlacer(diskArea(defaultPlayerBaseRadius() * 0.8), 0.95, 0.6, Infinity, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 6));
}

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
		"amounts": ["normal"]
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
		"amounts": ["normal"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 3,
			g_TileClasses.forest, 20,
			g_TileClasses.metal, 4,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 4,
			g_TileClasses.water, 2
		],
		"sizes": ["big"],
		"mixes": ["similar"],
		"amounts": ["few"]
	}
]));
Engine.SetProgress(60);

// Ensure initial forests
addElements([{
	"func": addForests,
	"avoid": [
		g_TileClasses.berries, 2,
		g_TileClasses.forest, 25,
		g_TileClasses.metal, 4,
		g_TileClasses.mountain, 5,
		g_TileClasses.player, 15,
		g_TileClasses.rock, 4,
		g_TileClasses.water, 2
	],
	"sizes": ["small"],
	"mixes": ["similar"],
	"amounts": ["tons"]
}]);
Engine.SetProgress(65);

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
			g_TileClasses.rock, 4,
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
			g_TileClasses.rock, 4,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
Engine.SetProgress(70);

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 2
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["tons"]
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
Engine.SetProgress(80);

g_Map.log("Painting dirt patches");
var dirtPatches = [
	{
		"sizes": [2, 4],
		"count": scaleByMapSize(2, 5),
		"terrain": g_Terrains.additionalDirt1
	},
	{
		"sizes": [4, 6, 8],
		"count": scaleByMapSize(4, 8),
		"terrain": g_Terrains.additionalDirt2
	}
];
for (let dirtPatch of dirtPatches)
	createPatches(
		dirtPatch.sizes,
		dirtPatch.terrain,
		[
			stayClasses(g_TileClasses.land, 6),
			avoidClasses(
				g_TileClasses.mountain, 4,
				g_TileClasses.forest, 2,
				g_TileClasses.shoreline, 2,
				g_TileClasses.player, 12)
		],
		dirtPatch.count,
		g_TileClasses.dirt,
		0.5);
Engine.SetProgress(85);

g_Map.log("Adding reeds");
createObjectGroups(
	new SimpleGroup(
		[
			new SimpleObject(g_Decoratives.reeds, 5, 12, 1, 4),
			new SimpleObject(g_Decoratives.rockMedium, 1, 2, 1, 5)
		],
		false,
		g_TileClasses.dirt),
	0,
	new HeightConstraint(heightReedsMin, heightReedsMax),
	scaleByMapSize(10, 25),
	5);
Engine.SetProgress(90);

g_Map.log("Adding dust...");
createObjectGroups(
	new SimpleGroup([new SimpleObject(g_Decoratives.dust, 1, 1, 1, 4)], false),
	0,
	[
		stayClasses(g_TileClasses.land, 5),
		avoidClasses(g_TileClasses.player, 10)
	],
	scaleByMapSize(10, 50),
	20);
Engine.SetProgress(95);

placePlayersNomad(
	g_Map.createTileClass(),
	[
		stayClasses(g_TileClasses.land, 5),
		avoidClasses(
			g_TileClasses.forest, 2,
			g_TileClasses.rock, 4,
			g_TileClasses.metal, 4,
			g_TileClasses.berries, 2,
			g_TileClasses.animals, 2,
			g_TileClasses.mountain, 2)
	]);

setWindAngle(-0.43);
setWaterTint(0.161, 0.286, 0.353);
setWaterColor(0.129, 0.176, 0.259);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("lake");

setTerrainAmbientColor(0.58, 0.443, 0.353);

setSunColor(0.733, 0.746, 0.574);
setSunRotation(Math.PI * 1.1);
setSunElevation(Math.PI / 7);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
