/**
 * Heightmap image source:
 * Imagery by Jesse Allen, NASA's Earth Observatory,
 * using data from the General Bathymetric Chart of the Oceans (GEBCO)
 * produced by the British Oceanographic Data Centre.
 * https://visibleearth.nasa.gov/view.php?id=73934
 *
 * Licensing: Public Domain, https://visibleearth.nasa.gov/useteEngine.php
 *
 * The heightmap image is reproduced using:
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73934/gebco_08_rev_elev_C1_grey_geo.tif
 * lat=41.1; lon=14.25; width=1.4;
 * lat1=$(bc <<< ";scale=5;$lat-$width/2"); lon1=$(bc <<< ";scale=5;$lon+$width/2"); lat2=$(bc <<< ";scale=5;$lat+$width/2"); lon2=$(bc <<< ";scale=5;$lon-$width/2")
 * gdal_translate -projwin $lon2 $lat2 $lon1 $lat1 gebco_08_rev_elev_C1_grey_geo.tif pompeii.tif
 * convert pompeii.tif -resize 512 -contrast-stretch 0 pompeii.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setBiome("generic/mediterranean");

g_Terrains.lavaOuter = "LavaTest06";
g_Terrains.lavaInner = "LavaTest05";
g_Terrains.lavaCenter = "LavaTest04";
g_Terrains.mainTerrain = "ocean_rock_a";
g_Terrains.forestFloor1 = "dirt_burned";
g_Terrains.forestFloor2 = "shoreline_stoney_a";
g_Terrains.tier1Terrain = "rock_metamorphic";
g_Terrains.tier2Terrain = "fissures";
g_Terrains.tier3Terrain = "LavaTest06";
g_Terrains.tier4Terrain = "ocean_rock_b";
g_Terrains.roadWild = "road1";
g_Terrains.road = "road1";
g_Terrains.water = "ocean_rock_a";
g_Terrains.cliff = "ocean_rock_b";

g_Gaia.mainHuntableAnimal = "gaia/fauna_goat";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_hawk";
g_Gaia.fruitBush = "gaia/fauna_chicken";
g_Gaia.fish = "gaia/fauna_fish";
g_Gaia.tree1 = "gaia/flora_tree_dead";
g_Gaia.tree2 = "gaia/flora_tree_oak_dead";
g_Gaia.tree3 = "gaia/flora_tree_dead";
g_Gaia.tree4 = "gaia/flora_tree_oak_dead";
g_Gaia.tree5 = "gaia/flora_tree_dead";
g_Gaia.stoneSmall = "gaia/geology_stone_alpine_a";
g_Gaia.columnsDoric = "gaia/ruins/column_doric";
g_Gaia.romanStatue = "gaia/ruins/stone_statues_roman";
g_Gaia.unfinishedTemple = "gaia/ruins/unfinished_greek_temple";
g_Gaia.dock = "structures/rome_dock";
g_Gaia.dockRubble = "rubble/rubble_rome_dock";

g_Decoratives.smoke1 = "actor|particle/smoke_volcano.xml";
g_Decoratives.smoke2 = "actor|particle/smoke_curved.xml";
g_Decoratives.grass = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_soft_dry_tuft_a.xml";
g_Decoratives.bushMedium = "actor|props/special/eyecandy/barrels_buried.xml";
g_Decoratives.bushSmall = "actor|props/special/eyecandy/handcart_1_broken.xml";
g_Decoratives.skeleton = "actor|props/special/eyecandy/skeleton.xml";
g_Decoratives.shipwrecks = [
	"actor|props/special/eyecandy/shipwreck_hull.xml",
	"actor|props/special/eyecandy/shipwreck_ram_side.xml",
	"actor|props/special/eyecandy/shipwreck_sail_boat.xml",
	"actor|props/special/eyecandy/shipwreck_sail_boat_cut.xml",
	"actor|props/special/eyecandy/barrels_floating.xml"
];
g_Decoratives.statues = [
	"actor|props/special/eyecandy/statue_aphrodite_huge.xml",
	"actor|props/special/eyecandy/sele_colonnade.xml",
	"actor|props/special/eyecandy/well_1_b.xml",
	"actor|props/special/eyecandy/anvil.xml",
	"actor|props/special/eyecandy/wheel_laying.xml",
	"actor|props/special/eyecandy/vase_rome_a.xml"
];

const heightScale = num => num * g_MapSettings.Size / 320;

const heightSeaGround = heightScale(-30);
const heightShorelineMin = heightScale(-1);
const heightShorelineMax = heightScale(0);
const heightWaterLevel = heightScale(0);
const heightLavaVesuv = heightScale(38);
const heightMountains = 140;

var g_Map = new RandomMap(0, g_Terrains.mainTerrain);
var mapCenter = g_Map.getCenter();

initTileClasses(["decorative", "lava", "dock"]);

g_Map.LoadHeightmapImage("pompeii.png", 0, heightMountains);
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
	new SmoothingPainter(1, 0.8, 1));
Engine.SetProgress(25);

g_Map.log("Marking water");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.water),
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(30);

g_Map.log("Marking shoreline");
var areaShoreline = createArea(
	new MapBoundsPlacer(),
	undefined,
	[
		new HeightConstraint(heightShorelineMin, heightShorelineMax),
		new NearTileClassConstraint(g_TileClasses.water, 2)
	]);
Engine.SetProgress(35);

g_Map.log("Marking land");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.land),
	avoidClasses(g_TileClasses.water, 0));
Engine.SetProgress(40);

g_Map.log("Marking dock search location");
var areaDockStart = createArea(
	new DiskPlacer(fractionToTiles(0.5) - 10, mapCenter),
	undefined,
	stayClasses(g_TileClasses.land, 6));

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

g_Map.log("Painting lava");
var areaVesuv = createArea(
	new RectPlacer(new Vector2D(mapCenter.x, fractionToTiles(0.3)), new Vector2D(fractionToTiles(0.7), fractionToTiles(0.15))),
	[
		new LayeredPainter([g_Terrains.lavaOuter,g_Terrains.lavaInner, g_Terrains.lavaCenter], [scaleByMapSize(1, 3), 2]),
		new TileClassPainter(g_TileClasses.lava)
	],
	new HeightConstraint(heightLavaVesuv, Infinity));
Engine.SetProgress(20);

g_Map.log("Adding smoke...");
createObjectGroupsByAreas(
	new SimpleGroup(
		[
			new SimpleObject(g_Decoratives.smoke1, 1, 1, 0, 4),
			new SimpleObject(g_Decoratives.smoke2, 2, 2, 0, 4)
		],
		false),
	0,
	stayClasses(g_TileClasses.lava, 0),
	scaleByMapSize(4, 12),
	20,
	[areaVesuv]);

g_Map.log("Creating docks");
for (let i = 0; i < scaleByMapSize(2, 4); ++i)
{
	let positionLand = pickRandom(areaDockStart.points);
	let dockPosition = areaShoreline.getClosestPointTo(positionLand);

	if (!avoidClasses(g_TileClasses.mountain, scaleByMapSize(4, 6), g_TileClasses.dock, 10).allows(dockPosition))
	{
		--i;
		continue;
	}
	g_Map.placeEntityPassable(randBool(0.4) ? g_Gaia.dock : g_Gaia.dockRubble, 0, dockPosition, -positionLand.angleTo(dockPosition) + Math.PI / 2);
	g_TileClasses.dock.add(dockPosition);
}
Engine.SetProgress(10);

if (!isNomad())
{
	g_Map.log("Placing players");
	let [playerIDs, playerPosition] = createBases(
		...playerPlacementRandom(
			sortAllPlayers(),
			[
				avoidClasses(g_TileClasses.mountain, 5),
				stayClasses(g_TileClasses.land, scaleByMapSize(5, 15))
			]),
		false);

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
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.lava, 2,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.lava, 2,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	}
]);
Engine.SetProgress(50);

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
			g_TileClasses.lava, 5,
			g_TileClasses.water, 3
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
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.lava, 5,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
Engine.SetProgress(60);

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.lava, 10,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
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
		"amounts": ["few"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.lava, 5,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
Engine.SetProgress(65);

g_Map.log("Adding gatherable stone statues...");
createObjectGroups(
	new SimpleGroup(
		[new SimpleObject(g_Gaia.romanStatue, 1, 1, 1, 4)],
		true,
		g_TileClasses.metal
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 20,
		g_TileClasses.mountain, 3,
		g_TileClasses.forest, 2,
		g_TileClasses.lava, 5,
		g_TileClasses.metal, 20
	),
	5 * scaleByMapSize(1, 4),
	50);
Engine.SetProgress(75);

g_Map.log("Adding stone ruins...");
createObjectGroups(
	new SimpleGroup(
		[
			new SimpleObject(g_Gaia.unfinishedTemple, 0, 1, 1, 4),
			new SimpleObject(g_Gaia.columnsDoric, 1, 1, 1, 4)
		],
		true,
		g_TileClasses.decorative
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 20,
		g_TileClasses.mountain, 5,
		g_TileClasses.forest, 2,
		g_TileClasses.lava, 5,
		g_TileClasses.decorative, 20
	),
	scaleByMapSize(1, 4),
	20);
Engine.SetProgress(80);

g_Map.log("Adding shipwrecks...");
createObjectGroups(
	new SimpleGroup(g_Decoratives.shipwrecks.map(shipwreck => new SimpleObject(shipwreck, 0, 1, 1, 20)), true, g_TileClasses.decorative),
	0,
	[
		avoidClasses(g_TileClasses.decorative, 20),
		stayClasses(g_TileClasses.water, 0)
	],
	scaleByMapSize(1, 5),
	20);
Engine.SetProgress(85);

g_Map.log("Adding more statues...");
createObjectGroups(
	new SimpleGroup(g_Decoratives.statues.map(ruin => new SimpleObject(ruin, 0, 1, 1, 20)), true, g_TileClasses.decorative),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 20,
		g_TileClasses.mountain, 2,
		g_TileClasses.forest, 2,
		g_TileClasses.lava, 5,
		g_TileClasses.decorative, 20
	),
	scaleByMapSize(3, 15),
	30);
Engine.SetProgress(90);

g_Map.log("Adding skeletons...");
createObjectGroups(
	new SimpleGroup(
		[new SimpleObject(g_Decoratives.skeleton, 3, 10, 1, 7)],
		true,
		g_TileClasses.dirt
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 10,
		g_TileClasses.mountain, 2,
		g_TileClasses.forest, 2,
		g_TileClasses.decorative, 2
	),
	scaleByMapSize(1, 5),
	50);
Engine.SetProgress(95);

placePlayersNomad(
	g_Map.createTileClass(),
	[
		stayClasses(g_TileClasses.land, 5),
		avoidClasses(
			g_TileClasses.forest, 1,
			g_TileClasses.rock, 4,
			g_TileClasses.metal, 4,
			g_TileClasses.animals, 2,
			g_TileClasses.mountain, 2)
	]);

setWaterTint(0.5, 0.5, 0.5);
setWaterColor(0.3, 0.3, 0.3);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("lake");

setTerrainAmbientColor(0.3, 0.3, 0.3);
setUnitsAmbientColor(0.3, 0.3, 0.3);

setSunColor(0.8, 0.8, 0.8);
setSunRotation(Math.PI);
setSunElevation(1/2);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setSkySet("stormy");

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
