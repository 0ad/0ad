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
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73934/gebco_08_rev_elev_C2_grey_geo.tif
 * lat=-3.177437; lon=35.574687; width=0.7
 * lat1=$(bc <<< ";scale=5;$lat-$width/2"); lon1=$(bc <<< ";scale=5;$lon+$width/2"); lat2=$(bc <<< ";scale=5;$lat+$width/2"); lon2=$(bc <<< ";scale=5;$lon-$width/2")
 * gdal_translate -projwin $lon2 $lat2 $lon1 $lat1 gebco_08_rev_elev_C2_grey_geo.tif ngorongoro.tif
 * convert ngorongoro.tif -resize 512 -contrast-stretch 0 ngorongoro.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setBiome("generic/savanna");

// ["dirta","savanna_wash_a","savanna_dirt_b","savanna_riparian_bank","savanna_grass_b","grass b soft dirt 50","grass1_spring","grass_field","grass1_spring","savanna_grass_a_wetseason","savanna_grass_b_wetseason","savanna_grass_a","new_savanna_grass_a","new_savanna_grass_b","new_savanna_grass_c","steppe_grass_dirt_66","peat_temp"];

g_Terrains.roadWild = "savanna_riparian_dry";
g_Terrains.road = "road2";

g_Gaia.metalLarge = "gaia/geology_metal_savanna_slabs";
g_Gaia.metalSmall = "gaia/geology_metal_tropic";
g_Gaia.fish = "gaia/fauna_fish_tilapia";
g_Gaia.tree1 = "gaia/flora_tree_baobab";
g_Gaia.tree2 = "gaia/flora_tree_baobab";
g_Gaia.tree3 = "gaia/flora_tree_baobab";
g_Gaia.tree4 = "gaia/flora_tree_baobab";
g_Gaia.tree5 = "gaia/flora_tree_baobab";

g_Decoratives.grass = "actor|props/flora/grass_savanna.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_soft_dry_tuft_a.xml";
g_Decoratives.rockLarge = "actor|geology/stone_savanna_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_savanna_med.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_desert_dry_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_dry_a.xml";

const heightScale = num => num * g_MapSettings.Size / 320;

const heightHighlands = heightScale(45);
const heightEden = heightScale(60);
const heightMax = 150;

function setBiomeLowlands()
{
	g_Gaia.mainHuntableAnimal = "gaia/fauna_giraffe";
	g_Gaia.secondaryHuntableAnimal = "gaia/fauna_zebra";

	g_Terrains.mainTerrain = "savanna_riparian_bank";
	g_Terrains.forestFloor1 = "savanna_dirt_rocks_b";
	g_Terrains.forestFloor2 = "savanna_dirt_rocks_c";
	g_Terrains.tier1Terrain = "savanna_dirt_rocks_a";
	g_Terrains.tier2Terrain = "savanna_grass_a";
	g_Terrains.tier3Terrain = "savanna_grass_b";
	g_Terrains.tier4Terrain = "savanna_forest_floor_a";
}

function setBiomeHighlands()
{

	g_Gaia.mainHuntableAnimal = "gaia/fauna_lioness";
	g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_lion";

	g_Terrains.mainTerrain = "savanna_grass_a_wetseason";
	g_Terrains.forestFloor1 = "savanna_grass_a";
	g_Terrains.forestFloor2 = "savanna_grass_b";
	g_Terrains.tier1Terrain = "savanna_grass_a_wetseason";
	g_Terrains.tier2Terrain = "savanna_grass_b_wetseason";
	g_Terrains.tier3Terrain = "savanna_shrubs_a_wetseason";
	g_Terrains.tier4Terrain = "savanna_shrubs_b";
}

function setBiomeEden()
{
	g_Gaia.mainHuntableAnimal = "gaia/fauna_rhino";
	g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_elephant_african_bush";
}

var g_Map = new RandomMap(0, g_Terrains.mainTerrain);
var mapCenter = g_Map.getCenter();

initTileClasses(["eden", "highlands"]);

g_Map.LoadHeightmapImage("ngorongoro.png", 0, heightMax);
Engine.SetProgress(15);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, scaleByMapSize(0.1, 0.5), 1));
Engine.SetProgress(25);

g_Map.log("Marking land");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.land));
Engine.SetProgress(40);

g_Map.log("Marking eden");
createArea(
	new DiskPlacer(fractionToTiles(0.14), mapCenter),
	new TileClassPainter(g_TileClasses.eden),
	new HeightConstraint(-Infinity, heightEden));
Engine.SetProgress(45);

g_Map.log("Marking highlands");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.highlands),
	[
		new HeightConstraint(heightHighlands, Infinity),
		avoidClasses(g_TileClasses.eden, 0)
	]);
Engine.SetProgress(50);

g_Map.log("Painting cliffs");
createArea(
	new MapBoundsPlacer(),
	[
		new TerrainPainter(g_Terrains.cliff),
		new TileClassPainter(g_TileClasses.mountain),
	],
	new SlopeConstraint(2, Infinity));
Engine.SetProgress(45);

if (!isNomad())
{
	g_Map.log("Placing players");
	let [playerIDs, playerPosition] = createBases(
		...playerPlacementRandom(
			sortAllPlayers(),
			[
				avoidClasses(
					g_TileClasses.mountain, 5,
					g_TileClasses.highlands, 5,
					g_TileClasses.eden, 5),
				stayClasses(g_TileClasses.land, defaultPlayerBaseRadius())
			]),
		true);

	g_Map.log("Flatten the initial CC area...");
	for (let position of playerPosition)
		createArea(
			new ClumpPlacer(diskArea(defaultPlayerBaseRadius() * 0.8), 0.95, 0.6, Infinity, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 6));
}

log("Render lowlands...");
setBiomeLowlands();
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
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
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	}
]);

addElements(shuffleArray([
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2

		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["scarce"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["normal"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["big"],
		"mixes": ["similar"],
		"amounts": ["many"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["normal"],
		"mixes": ["unique"],
		"amounts": ["many"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 4,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.eden, 2,
			g_TileClasses.highlands, 2
		],
		"sizes": ["big"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
Engine.SetProgress(60);

log("Render highlands...");
setBiomeHighlands();
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	}
]);

addElements(shuffleArray([
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["many"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2
		],
		"stay": [g_TileClasses.highlands, 2],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
Engine.SetProgress(70);

log("Render eden...");
setBiomeEden();
addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
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
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 3,
			g_TileClasses.metal, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 8,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["scarce"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 2,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 8,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2
		],
		"stay": [g_TileClasses.eden, 2],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["scarce"]
	}
]));
Engine.SetProgress(80);

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

setTerrainAmbientColor(0.521, 0.475, 0.322);

setSunColor(0.733, 0.746, 0.574);
setSunRotation(Math.PI);
setSunElevation(1/2);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
