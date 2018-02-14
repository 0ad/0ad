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
 * lat=41; lon=28; width=5;
 * gdal_translate -projwin $((lon-width/2)) $((lat+width/2)) $((lon+width/2)) $((lat-width/2)) gebco_08_rev_elev_C1_grey_geo.tif marmara.tif
 * convert marmara.tif -resize 512 -contrast-stretch 0 marmara.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setBiome("generic/mediterranean");

g_Terrains.mainTerrain = ["grass_mediterranean_dry_1024test", "grass_field_dry","new_savanna_grass_b"];
g_Terrains.forestFloor1 = "steppe_grass_dirt_66";
g_Terrains.forestFloor2 = "steppe_dirt_a";
g_Terrains.tier1Terrain = "medit_grass_field_b";
g_Terrains.tier2Terrain = "medit_grass_field_dry";
g_Terrains.tier3Terrain = "medit_shrubs_golden";
g_Terrains.tier4Terrain = "steppe_dirt_b";
g_Terrains.cliff = "medit_cliff_a";
g_Terrains.roadWild = "road_med_a";
g_Terrains.road = "road2";
g_Terrains.water = "medit_sand_messy";

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

const heightScale = num => num * g_MapSettings.Size / 320;

const heightSeaGround = heightScale(scaleByMapSize(-6, -4));
const heightWaterLevel = heightScale(0);
const heightShoreline = heightScale(0);

var g_Map = new RandomMap(0, g_Terrains.mainTerrain);
var mapCenter = g_Map.getCenter();

initTileClasses(["shoreline"]);

g_Map.LoadHeightmapImage("marmara.png", 0, 10);
Engine.SetProgress(15);

g_Map.log("Lowering sea ground");
createArea(
	new MapBoundsPlacer(),
	// Keep water impassable on all mapsizes
	new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, scaleByMapSize(1, 3)),
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(20);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, scaleByMapSize(0.1, 0.2), 1));
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
				avoidClasses(g_TileClasses.mountain, 5),
				stayClasses(g_TileClasses.land, scaleByMapSize(6, 25))
			]),
		true);

	g_Map.log("Flatten the initial CC area...");
	for (let position of playerPosition)
		createArea(
			new ClumpPlacer(diskArea(defaultPlayerBaseRadius() * 0.8), 0.95, 0.6, Infinity, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 6));
}
Engine.SetProgress(50);

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
Engine.SetProgress(60);

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
		"amounts": ["many"]
	}
]));
Engine.SetProgress(70);

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
		"amounts": ["many"]
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
Engine.SetProgress(80);

log("Adding reeds...");
createObjectGroups(
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
	scaleByMapSize(50, 400),
	2);
Engine.SetProgress(85);

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

setSunColor(0.753, 0.586, 0.584);
setSkySet("sunset");

// Inverted so that water appears on tiny maps and passages are maximized on larger maps
setWaterHeight(scaleByMapSize(20, 18));
setWaterTint(0.25, 0.67, 0.65);
setWaterColor(0.18, 0.36, 0.39);
setWaterWaviness(8);
setWaterMurkiness(0.99);
setWaterType("lake");

setTerrainAmbientColor(0.521, 0.475, 0.322);

setSunRotation(Math.PI * 0.85);
setSunElevation(Math.PI / 14);

setFogFactor(0.15);
setFogThickness(0);
setFogColor(0.64, 0.5, 0.35);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
