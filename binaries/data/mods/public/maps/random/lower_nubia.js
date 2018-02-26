/**
 * Heightmap image source:
 * Imagery by Jesse Allen, NASA's Earth Observatory,
 * using data from the General Bathymetric Chart of the Oceans (GEBCO)
 * produced by the British Oceanographic Data Centre.
 * https://visibleearth.nasa.gov/view.php?id=73934
 * https://visibleearth.nasa.gov/view.php?id=74393
 * Licensing: Public Domain, https://visibleearth.nasa.gov/useterms.php
 *
 * Since the elevation does not correlate with water distribution in lower_nubia,
 * this map additionally uses composite photography to paint the water correctly.
 *
 * To reproduce the heightmaps, first set the coordinates:
 * lat=23.25; lon=31.75; width=6;
 * lat1=$(bc <<< ";scale=5;$lat-$width/2"); lon1=$(bc <<< ";scale=5;$lon+$width/2"); lat2=$(bc <<< ";scale=5;$lat+$width/2"); lon2=$(bc <<< ";scale=5;$lon-$width/2")
 *
 * The land heightmap image is reproduced using:
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73934/gebco_08_rev_elev_C1_grey_geo.tif
 * gdal_translate -projwin $lon2 $lat2 $lon1 $lat1 gebco_08_rev_elev_C1_grey_geo.tif lower_nubia.tif
 * convert lower_nubia.tif -resize 512 -contrast-stretch 0 lower_nubia_heightmap.png
 * convert lower_nubia_heightmap.png -threshold 25% lower_nubia_land_threshold.png
 *
 * The watermap image is reproduced using:
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/74000/74393/world.topo.200407.3x21600x21600.C1.jpg
 * gdal_translate -a_srs EPSG:4326 -a_ullr 0 90 90 0 world.topo.200407.3x21600x21600.C1.jpg world.topo.200407.3x21600x21600.C1.jpg.tif
 * gdal_translate -projwin $lon2 $lat2 $lon1 $lat1 world.topo.200407.3x21600x21600.C1.jpg.tif lower_nubia_water.tif
 * convert lower_nubia_water.tif -set colorspace Gray -resize 512 -separate -average -threshold 51% lower_nubia_water_threshold.png
 *
 * No further changes should be applied to the images to keep them easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

TILE_CENTERED_HEIGHT_MAP = true;

const tSand = "desert_sand_dunes_100";
const tPlateau = ["savanna_dirt_a", "savanna_dirt_b"];
const tNilePlants = "desert_plants_a";
const tCliffUpper = ["medit_cliff_italia", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tRoad = "savanna_tile_a";
const tWater = "desert_sand_wet";

const oAcacia = "gaia/flora_tree_acacia";
const oTreeDead = "gaia/flora_tree_dead";
const oBerryBush = "gaia/flora_bush_berry_desert";
const oPalms = [
	"gaia/flora_tree_cretan_date_palm_tall",
	"gaia/flora_tree_cretan_date_palm_short",
	"gaia/flora_tree_palm_tropic",
	"gaia/flora_tree_date_palm",
	"gaia/flora_tree_senegal_date_palm",
	"gaia/flora_tree_medit_fan_palm"
];
const oStoneLarge = "gaia/geology_stonemine_savanna_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";
const oMetalSmall = "gaia/geology_metal_desert_small";
const oWoodTreasure = "gaia/treasure/wood";
const oGazelle = "gaia/fauna_gazelle";
const oElephant = "gaia/fauna_elephant_african_bush";
const oElephantInfant = "gaia/fauna_elephant_african_infant";
const oLion = "gaia/fauna_lion";
const oLioness = "gaia/fauna_lioness";
const oHawk = "gaia/fauna_hawk";
const oPyramid = "structures/kush_pyramid_large";

const aRock = actorTemplate("geology/stone_savanna_med");
const aBushes = [
	"props/flora/bush_dry_a",
	"props/flora/bush_medit_la_dry",
	"props/flora/bush_medit_me_dry",
	"props/flora/bush_medit_sm",
	"props/flora/bush_medit_sm_dry",
	"props/flora/bush_tempe_me_dry",
	"props/flora/grass_soft_dry_large_tall",
	"props/flora/grass_soft_dry_small_tall"
].map(actorTemplate);

const heightScale = num => num * g_MapSettings.Size / 320;

const heightSeaGround = heightScale(-3);
const heightWaterLevel = heightScale(0);
const heightNileForests = heightScale(15);
const heightPlateau2 = heightScale(38);
const minHeight = -3;
const maxHeight = 150;

const g_Map = new RandomMap(0, tSand);
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

const clWater = g_Map.createTileClass();
const clCliff = g_Map.createTileClass();
const clPlayer = g_Map.createTileClass();
const clBaseResource = g_Map.createTileClass();
const clForest = g_Map.createTileClass();
const clRock = g_Map.createTileClass();
const clMetal = g_Map.createTileClass();
const clFood = g_Map.createTileClass();
const clPyramid = g_Map.createTileClass();
const clPassage = g_Map.createTileClass();

g_Map.log("Loading heightmaps");
const heightmapLand = convertHeightmap1Dto2D(Engine.LoadHeightmapImage("maps/random/lower_nubia_heightmap.png"));
const heightmapLandThreshold = convertHeightmap1Dto2D(Engine.LoadHeightmapImage("maps/random/lower_nubia_land_threshold.png"));
const heightmapWaterThreshold = convertHeightmap1Dto2D(Engine.LoadHeightmapImage("maps/random/lower_nubia_water_threshold.png"));
Engine.SetProgress(3);

g_Map.log("Composing heightmap");
var heightmapCombined = [];
for (let x = 0; x < heightmapLand.length; ++x)
{
	heightmapCombined[x] = new Float32Array(heightmapLand.length);
	for (let y = 0; y < heightmapLand.length; ++y)
		// Reduce ahistorical Lake Nasser and lakes in the valleys west of the Nile.
		// The heightmap does not correlate with water distribution in this arid climate at all.
		heightmapCombined[x][y] = heightmapLandThreshold[x][y] || heightmapWaterThreshold[x][y] ? heightmapLand[x][y] : minHeight;
}
Engine.SetProgress(6);

g_Map.log("Applying heightmap");
createArea(
	new MapBoundsPlacer(),
	new HeightmapPainter(heightmapCombined, minHeight, maxHeight));
Engine.SetProgress(9);

g_Map.log("Lowering sea ground");
createArea(
	new MapBoundsPlacer(),
	[
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 3),
		new TileClassPainter(clWater)
	],
	new HeightConstraint(-Infinity, heightSeaGround));
Engine.SetProgress(15);

g_Map.log("Creating Nile passages...");
const riverAngle = Math.PI * 3 / 4;
for (let i = 0; i < scaleByMapSize(8, 15); ++i)
{
	let x = fractionToTiles(randFloat(0, 1));
	createArea(
		new PathPlacer(
			new Vector2D(x, mapBounds.bottom).rotateAround(riverAngle, mapCenter),
			new Vector2D(x, mapBounds.top).rotateAround(riverAngle, mapCenter),
			scaleByMapSize(5, 7),
			0.2,
			5,
			0.2,
			0,
			Infinity),
		[
			new ElevationBlendingPainter(heightNileForests, 0.5),
			new SmoothingPainter(2, 1, 2),
			new TileClassPainter(clPassage)
		],
		[
			new NearTileClassConstraint(clWater, 4),
			avoidClasses(clPassage, scaleByMapSize(15, 25))
		]);
}
Engine.SetProgress(18);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, scaleByMapSize(0.5, 1), 1));
Engine.SetProgress(22);

g_Map.log("Marking water");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(clWater),
	new HeightConstraint(-Infinity, heightSeaGround));
Engine.SetProgress(28);

g_Map.log("Marking cliffs");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(clCliff),
	new SlopeConstraint(2, Infinity));
Engine.SetProgress(32);

g_Map.log("Painting water and shoreline");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tWater),
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(35);

g_Map.log("Painting plateau");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tPlateau),
	new HeightConstraint(heightPlateau2, Infinity));
Engine.SetProgress(38);

var playerIDs = [];
var playerPosition = [];
if (!isNomad())
{
	g_Map.log("Finding player locations...");
	[playerIDs, playerPosition] = playerPlacementRandom(sortAllPlayers(), avoidClasses(clWater, scaleByMapSize(8, 12), clCliff, scaleByMapSize(8, 12)));

	g_Map.log("Flatten the initial CC area...");
	for (let position of playerPosition)
		createArea(
			new ClumpPlacer(diskArea(defaultPlayerBaseRadius() * 0.8), 0.95, 0.6, Infinity, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 6));
}
Engine.SetProgress(43);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": avoidClasses(clCliff, 0, clWater, 0),
	"CityPatch": {
		"outerTerrain": tRoad,
		"innerTerrain": tRoad
	},
	"Chicken": {
		"template": oGazelle,
		"distance": 18,
		"minGroupDistance": 2,
		"maxGroupDistance": 4,
		"minGroupCount": 2,
		"maxGroupCount": 3
	},
	"Berries": {
		"template": oBerryBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oAcacia,
		"count": scaleByMapSize(3, 12),
		"minDistGroup": 2,
		"maxDistGroup": 6,
		"minDist": 15,
		"maxDist": 16
	},
	"Treasures": {
		"types": [
			{
				"template": oWoodTreasure,
				"count": 14
			}
		]
	},
	"Decoratives": {
		"template": pickRandom(aBushes)
	}
});
Engine.SetProgress(50);

g_Map.log("Painting lower cliffs");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tNilePlants),
	[
		new SlopeConstraint(2, Infinity),
		new NearTileClassConstraint(clWater, 2)
	]);
Engine.SetProgress(55);

g_Map.log("Painting upper cliffs");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tCliffUpper),
	[
		avoidClasses(clWater, 2),
		new SlopeConstraint(2, Infinity)
	]);
Engine.SetProgress(60);

g_Map.log("Creating stone mines");
createMines(
	[
		[new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oStoneSmall, 3, 6, 1, 3, 0, 2 * Math.PI, 1)]
	],
	avoidClasses(clWater, 4, clCliff, 4, clPlayer, 20, clRock, 10),
	clRock,
	scaleByMapSize(10, 30));
Engine.SetProgress(63);

g_Map.log("Creating metal mines");
createMines(
	[
		[new SimpleObject(oMetalSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oMetalLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oMetalSmall, 3, 6, 1, 3, 0, 2 * Math.PI, 1)]
	],
	avoidClasses(clWater, 4, clCliff, 4, clPlayer, 20, clMetal, 10, clRock, 5),
	clMetal,
	scaleByMapSize(10, 30));
Engine.SetProgress(67);

g_Map.log("Creating pyramid");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oPyramid, 1, 1, 1, 1)], true, clPyramid),
	0,
	[new NearTileClassConstraint(clWater, 10), avoidClasses(clWater, 6, clCliff, 6, clPlayer, 30, clMetal, 6, clRock, 6)],
	1,
	500);
Engine.SetProgress(70);

g_Map.log("Creating forests");
createObjectGroups(
	new SimpleGroup([new RandomObject(oPalms, 1, 2, 1, 1)], true, clForest),
	0,
	[
		new NearTileClassConstraint(clWater, scaleByMapSize(1, 8)),
		new HeightConstraint(heightNileForests, Infinity),
		avoidClasses(clWater, 0, clCliff, 0, clForest, 1, clPlayer, 12, clBaseResource, 5, clPyramid, 6)
	],
	scaleByMapSize(100, 1000),
	200);
Engine.SetProgress(73);

createStragglerTrees(
	[oAcacia, oTreeDead],
	avoidClasses(clWater, 10, clCliff, 1, clPlayer, 12, clBaseResource, 5, clPyramid, 6),
	clForest,
	scaleByMapSize(15, 400),
	200);
Engine.SetProgress(77);

const avoidCollisions = avoidClasses(clPlayer, 12, clBaseResource, 5, clWater, 1, clForest, 1, clRock, 4, clMetal, 4, clFood, 6, clCliff, 0, clPyramid, 6);

g_Map.log("Creating gazelles");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGazelle, 5, 7, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(80);

if (!isNomad())
{
	g_Map.log("Creating lions");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(oLion, 1, 2, 2, 4), new SimpleObject(oLioness, 2, 3, 2, 4)], true, clFood),
		0,
		avoidCollisions,
		scaleByMapSize(2, 10),
		50);
}
Engine.SetProgress(83);

g_Map.log("Creating elephants");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oElephant, 2, 3, 2, 4), new SimpleObject(oElephantInfant, 2, 3, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(86);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 2, clRock, 4, clMetal, 4, clFood, 2, clCliff, 2, clPyramid, 6));
Engine.SetProgress(90);

g_Map.log("Creating hawk");
for (let i = 0; i < scaleByMapSize(0, 2); ++i)
	g_Map.placeEntityAnywhere(oHawk, 0, mapCenter, randomAngle());
Engine.SetProgress(90);

createDecoration(
	aBushes.map(bush => [new SimpleObject(bush, 0, 3, 2, 4)]),
	aBushes.map(bush => scaleByMapSize(100, 800) * randIntInclusive(1, 3)),
	[
		new NearTileClassConstraint(clWater, 2),
		new HeightConstraint(heightWaterLevel, Infinity),
		avoidClasses(clForest, 0)
	]);
Engine.SetProgress(92);

createDecoration(
	[[new SimpleObject(aRock, 0, 4, 2, 4)]],
	[[scaleByMapSize(100, 600)]],
	avoidClasses(clWater, 0));
Engine.SetProgress(95);

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
