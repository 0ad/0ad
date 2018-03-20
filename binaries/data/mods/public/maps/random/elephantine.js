/**
 * Heightmap image source:
 * OpenStreetMap, available under Open Database Licence, www.openstreetmap.org/copyright
 * http://download.geofabrik.de/africa.html
 *
 * To reproduce the river image:
 * You need a gdal version that supports osm, see http://www.gdal.org/drv_osm.html
 * wget http://download.geofabrik.de/africa/egypt-latest.osm.pbf
 * lon=32.89; lat=24.09175; width=0.025;
 * lat1=$(bc <<< ";scale=5;$lat-$width/2"); lon1=$(bc <<< ";scale=5;$lon+$width/2"); lat2=$(bc <<< ";scale=5;$lat+$width/2"); lon2=$(bc <<< ";scale=5;$lon-$width/2")
 * rm elephantine.geojson; ogr2ogr -f GeoJSON elephantine.geojson -clipdst $lon1 $lat1 $lon2 $lat2 egypt-latest.osm.pbf -sql 'select * from multipolygons where natural="water"'
 * gdal_rasterize -burn 10 -ts 512 512 elephantine.geojson elephantine.tif
 * convert elephantine.tif -threshold 50% -negate elephantine.png
 *
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

TILE_CENTERED_HEIGHT_MAP = true;

const tPrimary = ["savanna_dirt_rocks_a_red", "savanna_dirt_a_red", "savanna_dirt_b_red"];
const tDirt = ["new_savanna_dirt_c", "new_savanna_dirt_d", "savanna_dirt_b_red", "savanna_dirt_plants_cracked"];
const tWater = "desert_sand_wet";
const tRoad =  "savanna_tile_a_red";
const tRoadIsland = "savanna_tile_a";
const tRoadWildIsland = "savanna_dirt_rocks_a";
const tGrass = ["savanna_shrubs_a_wetseason", "alpine_grass_b_wild", "medit_shrubs_a", "steppe_grass_green_a"];
const tForestFloorLand = "savanna_forestfloor_b_red";
const tForestFloorIsland = pickRandom(tGrass);

const oAcacia = "gaia/flora_tree_acacia";
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

const oTreasure = [
	"gaia/treasure/food_barrel",
	"gaia/treasure/food_bin",
	"gaia/treasure/wood",
	"gaia/treasure/metal",
	"gaia/treasure/stone"
];
const oBerryBush = "gaia/flora_bush_berry_desert";
const oGazelle = "gaia/fauna_gazelle";
const oRhino = "gaia/fauna_rhino";
const oWarthog = "gaia/fauna_boar";
const oGiraffe = "gaia/fauna_giraffe";
const oGiraffeInfant = "gaia/fauna_giraffe_infant";
const oElephant = "gaia/fauna_elephant_african_bush";
const oElephantInfant = "gaia/fauna_elephant_african_infant";
const oLion = "gaia/fauna_lion";
const oLioness = "gaia/fauna_lioness";
const oCrocodile = "gaia/fauna_crocodile";
const oFish = "gaia/fauna_fish";
const oHawk = "gaia/fauna_hawk";

// The main temple on elephantine was very similar looking (Greco-Roman-Egyptian):
const oWonder = "structures/ptol_wonder";
const oTemples = ["structures/kush_temple_amun", "structures/kush_temple"];
const oPyramid = "structures/kush_pyramid_large";
const oTowers = new Array(2).fill("uncapturable|structures/kush_sentry_tower").concat(["uncapturable|structures/kush_defense_tower"]);

const oHeroes = Engine.FindTemplates("units/", true).filter(templateName => templateName.startsWith("units/kush_hero_"));
const oUnits = Engine.FindTemplates("units/", false).filter(templateName =>
	templateName.startsWith("units/kush_") &&
	oHeroes.every(heroTemplateName => heroTemplateName != templateName) &&
	Engine.GetTemplate(templateName).Identity.VisibleClasses._string.split(" ").some(type => ["Soldier", "Healer", "Female"].indexOf(type) != -1));

const aRock = actorTemplate("geology/stone_savanna_med");

const aStatues = [
	"props/structures/kushites/statue_bird",
	"props/structures/kushites/statue_lion",
	"props/structures/kushites/statue_ram"
].map(actorTemplate);

const aBushesShoreline = [
	...new Array(4).fill("props/flora/ferns"),
	"props/flora/bush",
	"props/flora/bush_medit_la",
	"props/flora/bush_medit_la_lush",
	"props/flora/bush_medit_me_lush",
	"props/flora/bush_medit_sm",
	"props/flora/bush_medit_sm_lush",
	"props/flora/bush_tempe_la_lush"
].map(actorTemplate);

const aBushesIslands = aBushesShoreline.concat(new Array(3).fill(actorTemplate("props/flora/foliagebush")));

const aBushesDesert = [
	"props/flora/bush_dry_a",
	"props/flora/bush_medit_la_dry",
	"props/flora/bush_medit_me_dry",
	"props/flora/bush_medit_sm",
	"props/flora/bush_medit_sm_dry",
	"props/flora/bush_tempe_me_dry",
	"props/flora/grass_soft_dry_large_tall",
	"props/flora/grass_soft_dry_small_tall"
].map(actorTemplate);

const pForestPalmsLand = [
	tForestFloorLand,
	...oPalms.map(tree => tForestFloorLand + TERRAIN_SEPARATOR + tree),
	tForestFloorLand];

const pForest2Land = [
	tForestFloorLand,
	tForestFloorLand + TERRAIN_SEPARATOR + oAcacia,
	tForestFloorLand
];

const pForestPalmsIsland = [
	tForestFloorIsland,
	...oPalms.map(tree => tForestFloorIsland + TERRAIN_SEPARATOR + tree),
	tForestFloorIsland];

const pForest2Island = [
	tForestFloorIsland,
	tForestFloorIsland + TERRAIN_SEPARATOR + oAcacia,
	tForestFloorIsland
];

const heightScale = num => num * g_MapSettings.Size / 320;

const heightSeaGround = heightScale(-15);
const heightWaterLevel = heightScale(0);
const heightShore = heightScale(1);
const heightOffsetPath = heightScale(-4);
const minHeight = -1;
const maxHeight = 2;

const g_Map = new RandomMap(0, tPrimary);
const mapBounds = g_Map.getBounds();
const mapCenter = g_Map.getCenter();

const clWater = g_Map.createTileClass();
const clIsland  = g_Map.createTileClass();
const clCliff = g_Map.createTileClass();
const clPlayer = g_Map.createTileClass();
const clBaseResource = g_Map.createTileClass();
const clForest = g_Map.createTileClass();
const clDirt = g_Map.createTileClass();
const clRock = g_Map.createTileClass();
const clMetal = g_Map.createTileClass();
const clFood = g_Map.createTileClass();
const clTemple = g_Map.createTileClass();
const clTower = g_Map.createTileClass();
const clStatue = g_Map.createTileClass();
const clSoldier = g_Map.createTileClass();
const clTreasure = g_Map.createTileClass();
const clPath = g_Map.createTileClass();

const riverAngle = 0.22 * Math.PI;
const riverWidthBorder = fractionToTiles(0.27);
const riverWidthCenter = fractionToTiles(0.35);

const avoidCollisions = avoidClasses(
	clPlayer, 15, clWater, 1, clForest, 1, clRock, 4, clMetal, 4, clFood, 6, clPath, 1,
	clTemple, 11, clCliff, 0, clStatue, 2, clSoldier, 3, clTower, 2, clTreasure, 1);

g_Map.LoadHeightmapImage("elephantine.png", minHeight, maxHeight);
Engine.SetProgress(3);

g_Map.log("Lowering sea ground");
createArea(
	new MapBoundsPlacer(),
	[
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
		new TileClassPainter(clWater)
	],
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(6);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, scaleByMapSize(0.1, 0.5), 1));
Engine.SetProgress(10);

g_Map.log("Marking islands");
var areaIsland = createArea(
	new ConvexPolygonPlacer(
		[
			new Vector2D(mapCenter.x - riverWidthBorder / 2, mapBounds.top),
			new Vector2D(mapCenter.x - riverWidthBorder / 2, mapBounds.bottom),
			new Vector2D(mapCenter.x - riverWidthCenter / 2, mapCenter.y),
			new Vector2D(mapCenter.x + riverWidthCenter / 2, mapCenter.y),
			new Vector2D(mapCenter.x + riverWidthBorder / 2, mapBounds.top),
			new Vector2D(mapCenter.x + riverWidthBorder / 2, mapBounds.bottom)
		].map(v => v.rotateAround(riverAngle, mapCenter)),
		Infinity),
	new TileClassPainter(clIsland),
	avoidClasses(clPlayer, 0, clWater, 0));
Engine.SetProgress(13);

g_Map.log("Painting islands");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tGrass),
	stayClasses(clIsland, 0));
Engine.SetProgress(16);

g_Map.log("Painting water and shoreline");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tWater),
	new HeightConstraint(-Infinity, heightShore));
Engine.SetProgress(19);

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(riverAngle, fractionToTiles(0.62)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": avoidClasses(clWater, 4),
	"Walls": "towers",
	"CityPatch": {
		"outerTerrain": tRoad,
		"innerTerrain": tRoad
	},
	"Chicken": {
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
		"count": 2
	},
	"Decoratives": {
		"template": pickRandom(aBushesDesert)
	}
});
Engine.SetProgress(22);

g_Map.log("Creating temple");
var groupTemple = createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(g_Map.getSize() >= 320 ? [oWonder] : oTemples, 1, 1, 0, 1, riverAngle, riverAngle)], true, clTemple),
	0,
	stayClasses(clIsland, scaleByMapSize(10, 20)),
	1,
	200,
	[areaIsland]);
Engine.SetProgress(34);

g_Map.log("Creating pyramid");
var groupPyramid = createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oPyramid, 1, 1, 0, 1, riverAngle, riverAngle)], true, clTemple),
	0,
	[stayClasses(clIsland, scaleByMapSize(10, 20)), avoidClasses(clTemple, scaleByMapSize(20, 50)), avoidCollisions],
	1,
	200,
	[areaIsland]);
Engine.SetProgress(37);

g_Map.log("Painting city patches");
var cityCenters = [
	groupTemple[0] && groupTemple[0][0] && { "pos": Vector2D.from3D(groupTemple[0][0].position), "radius": 10 },
	groupPyramid[0] && groupPyramid[0][0] &&  { "pos": Vector2D.from3D(groupPyramid[0][0].position), "radius": 6 },].filter(pos => !!pos);

var areaCityPatch = cityCenters.map(cityCenter =>
	createArea(
		new DiskPlacer(cityCenter.radius, cityCenter.pos),
		new LayeredPainter([tRoadWildIsland, tRoadIsland], [2]),
		stayClasses(clIsland, 2)));
Engine.SetProgress(40);

g_Map.log("Painting city path");
if (cityCenters.length == 2)
	createArea(
		new PathPlacer(cityCenters[0].pos, cityCenters[1].pos, 4, 0.3, 4, 0.2, 0.05),
		[
			new LayeredPainter([tRoadWildIsland, tRoadIsland], [1]),
			new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetPath, 4),
			new TileClassPainter(clPath)
		]);
Engine.SetProgress(42);

createBumps(avoidClasses(clPlayer, 10, clWater, 2, clTemple, 10, clPath, 1), scaleByMapSize(10, 500), 1, 8, 4, 0.2, 3);
Engine.SetProgress(43);

g_Map.log("Marking cliffs");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(clCliff),
	[
		avoidClasses(clWater, 2),
		new SlopeConstraint(2, Infinity)
	]);
Engine.SetProgress(44);

g_Map.log("Creating stone mines");
createMines(
	[
		[new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oStoneSmall, 2, 5, 1, 3, 0, 2 * Math.PI, 1)]
	],
	avoidClasses(clWater, 4, clCliff, 4, clPlayer, 20, clRock, 10, clPath, 1),
	clRock,
	scaleByMapSize(6, 24));
Engine.SetProgress(45);

g_Map.log("Creating metal mines");
createMines(
	[
		[new SimpleObject(oMetalSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oMetalLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oMetalSmall, 2, 5, 1, 3, 0, 2 * Math.PI, 1)]
	],
	avoidClasses(clWater, 4, clCliff, 4, clPlayer, 20, clMetal, 10, clRock, 5, clPath, 1),
	clMetal,
	scaleByMapSize(6, 24));
Engine.SetProgress(46);

g_Map.log("Creating kushite towers");
createObjectGroups(
	new SimpleGroup([new RandomObject(oTowers, 1, 1, 0, 1)], true, clTower),
	0,
	[
		stayClasses(clIsland, scaleByMapSize(4, 8)),
		new NearTileClassConstraint(clTemple, 25),
		avoidClasses(clTower, 12, clPlayer, 30),
		avoidCollisions
	],
	scaleByMapSize(4, 12),
	200);
Engine.SetProgress(49);

var [forestTrees, stragglerTrees] = getTreeCounts(400, 3000, 0.7);
createForests(
	[tForestFloorLand, tForestFloorLand, tForestFloorLand, pForestPalmsLand, pForest2Land],
	[avoidCollisions, avoidClasses(clIsland, 0, clPlayer, 20, clForest, 18, clWater, 2)],
	clForest,
	forestTrees / 2);
Engine.SetProgress(52);

createForests(
	[tForestFloorIsland, tForestFloorIsland, tForestFloorIsland, pForestPalmsIsland, pForest2Island],
	[stayClasses(clIsland, 0), avoidClasses(clForest, 15, clWater, 2), avoidCollisions],
	clForest,
	forestTrees / 2);
Engine.SetProgress(55);

g_Map.log("Creating dirt patches");
createPatches(
	[scaleByMapSize(5, 15)],
	tDirt,
	avoidClasses(clWater, 0, clIsland, 0, clForest, 0, clDirt, 5, clPlayer, 12),
	scaleByMapSize(5, 30),
	clDirt);
Engine.SetProgress(58);

g_Map.log("Creating statues");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(aStatues, 1, 1, 0, 1)], true, clStatue),
	0,
	[
		stayClasses(clIsland, scaleByMapSize(8, 24)),
		new NearTileClassConstraint(clTemple, 10),
		avoidCollisions
	],
	scaleByMapSize(2, 10),
	400,
	[areaIsland]);
Engine.SetProgress(61);

g_Map.log("Creating treasure");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(oTreasure, 1, 2, 0, 1)], true, clTreasure),
	0,
	avoidCollisions,
	scaleByMapSize(4, 10),
	100,
	areaCityPatch);
Engine.SetProgress(62);

g_Map.log("Creating hero");
createObjectGroups(
	new SimpleGroup([new RandomObject(oHeroes, 1, 1, 0, 1)], true, clSoldier),
	0,
	[
		stayClasses(clIsland, scaleByMapSize(2, 24)),
		new NearTileClassConstraint(clTemple, 14),
		avoidCollisions
	],
	1,
	500);
Engine.SetProgress(64);

g_Map.log("Creating soldiers");
createObjectGroups(
	new SimpleGroup([new RandomObject(oUnits, 1, 1, 0, 1)], true, clSoldier),
	0,
	[
		new StaticConstraint(
			[
				stayClasses(clIsland, scaleByMapSize(2, 24)),
				new NearTileClassConstraint(clTemple, 20)
			]),
		avoidCollisions
	],
	scaleByMapSize(12, 60),
	200);
Engine.SetProgress(67);

g_Map.log("Creating berries");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oBerryBush, 3, 5, 1, 2)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(4, 12),
	250);
Engine.SetProgress(70);

g_Map.log("Creating rhinos");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oRhino, 1, 1, 0, 1)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(73);

g_Map.log("Creating warthog");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oWarthog, 1, 1, 0, 1)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(77);

g_Map.log("Creating gazelles");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGazelle, 5, 7, 2, 4)], true, clFood),
	0,
	[avoidClasses(clIsland, 1), avoidCollisions],
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(80);

g_Map.log("Creating giraffes");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGiraffe, 2, 3, 2, 4), new SimpleObject(oGiraffeInfant, 2, 3, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(83);

if (!isNomad())
{
	g_Map.log("Creating lions");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(oLion, 1, 2, 2, 4), new SimpleObject(oLioness, 2, 3, 2, 4)], true, clFood),
		0,
		avoidCollisions,
		scaleByMapSize(2, 10),
		50);
	Engine.SetProgress(87);
}

g_Map.log("Creating elephants");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oElephant, 2, 3, 2, 4), new SimpleObject(oElephantInfant, 2, 3, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(88);

g_Map.log("Creating crocodiles");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oCrocodile, 2, 3, 3, 5)], true, clFood),
	0,
	[
		new NearTileClassConstraint(clWater, 3),
		avoidCollisions
	],
	scaleByMapSize(2, 10),
	50);
Engine.SetProgress(89);

g_Map.log("Creating hawk");
for (let i = 0; i < scaleByMapSize(0, 2); ++i)
	g_Map.placeEntityAnywhere(oHawk, 0, mapCenter, randomAngle());
Engine.SetProgress(90);

g_Map.log("Creating fish");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oFish, 1, 2, 0, 1)], true, clFood),
	0,
	[stayClasses(clWater, 8), avoidClasses(clFood, 16)],
	scaleByMapSize(15, 80),
	50);
Engine.SetProgress(91);

createStragglerTrees(
	[oAcacia],
	avoidCollisions,
	clForest,
	stragglerTrees);
Engine.SetProgress(93);

placePlayersNomad(clPlayer, [avoidCollisions, avoidClasses(clIsland, 0)]);
Engine.SetProgress(95);

createDecoration(
	aBushesDesert.map(bush => [new SimpleObject(bush, 0, 3, 2, 4)]),
	aBushesDesert.map(bush => scaleByMapSize(20, 150) * randIntInclusive(1, 3)),
	[avoidClasses(clIsland, 0), avoidCollisions]);
Engine.SetProgress(96);

createDecoration(
	aBushesIslands.map(bush => [new SimpleObject(bush, 0, 4, 2, 4)]),
	aBushesIslands.map(bush => scaleByMapSize(20, 150) * randIntInclusive(1, 3)),
	[stayClasses(clIsland, 0), avoidCollisions]);
Engine.SetProgress(97);

createDecoration(
	[[new SimpleObject(aRock, 0, 4, 2, 4)]],
	[[scaleByMapSize(80, 500)]],
	[avoidClasses(clIsland, 0), avoidCollisions]);
Engine.SetProgress(98);

createDecoration(
	aBushesShoreline.map(bush => [new SimpleObject(bush, 0, 3, 2, 4)]),
	aBushesShoreline.map(bush => scaleByMapSize(200, 1000)),
	[new HeightConstraint(heightWaterLevel, heightShore), avoidCollisions]);
Engine.SetProgress(99);

g_Environment = {
	"SkySet": "cloudless",
	"SunColor": {
		"r": 1,
		"g": 0.964706,
		"b": 0.909804,
		"a": 0
	},
	"SunElevation": 0.908117,
	"SunRotation": -0.558369,
	"TerrainAmbientColor": {
		"r": 0.54902,
		"g": 0.419608,
		"b": 0.352941,
		"a": 0
	},
	"UnitsAmbientColor": { "r": 0.721569, "g": 0.529412, "b": 0.4, "a": 0 },
	"Fog": {
		"FogFactor": 0.00195313,
		"FogThickness": 0,
		"FogColor": {
			"r": 0.941176,
			"g": 0.917647,
			"b": 0.807843,
			"a": 0
		}
	},
	"Water": {
		"WaterBody": {
			"Type": "lake",
			"Color": {
				"r": 0.443137,
				"g": 0.341176,
				"b": 0.14902,
				"a": 0
			},
			"Tint": {
				"r": 0.705882,
				"g": 0.67451,
				"b": 0.454902,
				"a": 0
			},
			"Waviness": 8.4668,
			"Murkiness": 0.92,
			"WindAngle": 0.625864
		}
	},
	"Postproc": {
		"Brightness": 0.0234375,
		"Contrast": 1.09961,
		"Saturation": 0.828125,
		"Bloom": 0.142969,
		"PostprocEffect": "hdr"
	}
};

g_Map.ExportMap();
