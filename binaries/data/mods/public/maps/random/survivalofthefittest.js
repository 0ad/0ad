Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tHill = g_Terrains.hill;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tTier4Terrain = g_Terrains.tier4Terrain;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;
const aWaypointFlag = "actor|props/special/common/waypoint_flag.xml";

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

const oTreasureSeeker = "undeletable|skirmish/units/default_support_female_citizen";

const triggerPointAttacker = "trigger/trigger_point_A";
const triggerPointTreasures = [
	"trigger/trigger_point_B",
	"trigger/trigger_point_C",
	"trigger/trigger_point_D"
];

const heightLand = 3;
const heightHill = 30;

var g_Map = new RandomMap(heightHill, tMainTerrain);

var numPlayers = getNumPlayers();
var mapSize = g_Map.getSize();
var mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clLand = g_Map.createTileClass();
var clWomen = g_Map.createTileClass();

g_Map.log("Creating central area");
createArea(
	new ClumpPlacer(diskArea(fractionToTiles(0.15)), 0.7, 0.1, 10, mapCenter),
	[
		new LayeredPainter([tMainTerrain, tMainTerrain], [3]),
		new SmoothElevationPainter(ELEVATION_SET, heightLand, 3),
		new TileClassPainter(clLand)
	]);
Engine.SetProgress(10);

var [playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.3));
var halfway = distributePointsOnCircle(numPlayers, startAngle, fractionToTiles(0.375), mapCenter)[0].map(v => v.round());
var attacker = distributePointsOnCircle(numPlayers, startAngle, fractionToTiles(0.45), mapCenter)[0].map(v => v.round());
var passage = distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.5), mapCenter)[0];

g_Map.log("Creating player bases, passages, treasure seeker woman and attacker points");
for (let  i = 0; i < numPlayers; ++i)
{
	placeStartingEntities(playerPosition[i], playerIDs[i], getStartingEntities(playerIDs[i]).filter(ent =>
		ent.Template.indexOf("civil_centre") != -1 || ent.Template.indexOf("infantry") != -1));

	placePlayerBaseDecoratives({
		"playerPosition": playerPosition[i],
		"template": aGrassShort,
		"BaseResourceClass": clBaseResource
	});

	// Passage between player and neighbor
	createArea(
		new PathPlacer(mapCenter, passage[i], scaleByMapSize(14, 24), 0.4, scaleByMapSize(3, 9), 0.2, 0.05),
		[
			new LayeredPainter([tMainTerrain, tMainTerrain], [1]),
			new SmoothElevationPainter(ELEVATION_SET, heightLand, 4)
		]);

	// Treasure seeker woman
	let femaleLocation = findLocationInDirectionBasedOnHeight(playerPosition[i], mapCenter, -3 , 3.5, 3).round();
	clWomen.add(femaleLocation);
	g_Map.placeEntityPassable(oTreasureSeeker, playerIDs[i], femaleLocation, playerAngle[i] + Math.PI);

	// Attacker spawn point
	g_Map.placeEntityAnywhere(aWaypointFlag, 0, attacker[i], Math.PI / 2);
	g_Map.placeEntityPassable(triggerPointAttacker, playerIDs[i], attacker[i], Math.PI / 2);

	// Preventing mountains in the area between player and attackers at player
	addCivicCenterAreaToClass(playerPosition[i], clPlayer);
	clPlayer.add(attacker[i]);
	clPlayer.add(halfway[i]);
}
Engine.SetProgress(20);

paintTerrainBasedOnHeight(3.12, 29, 1, tCliff);
paintTileClassBasedOnHeight(3.12, 29, 1, clHill);

for (let triggerPointTreasure of triggerPointTreasures)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(triggerPointTreasure, 1, 1, 0, 0)], true, clWomen),
		0,
		[avoidClasses(clForest, 5, clPlayer, 5, clHill, 5), stayClasses(clLand, 5)],
		scaleByMapSize(40, 140), 100
	);
Engine.SetProgress(25);

createBumps(stayClasses(clLand, 5));

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
createForests(
	[tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
	[avoidClasses(clPlayer, 20, clForest, 5, clHill, 0, clBaseResource,2, clWomen, 5), stayClasses(clLand, 4)],
	clForest,
	forestTrees);
Engine.SetProgress(30);

if (randBool())
	createHills(
		[tMainTerrain, tCliff, tHill],
		[avoidClasses(clPlayer, 20, clHill, 5, clBaseResource, 3, clWomen, 5), stayClasses(clLand, 5)],
		clHill,
		scaleByMapSize(10, 60) * numPlayers);
else
	createMountains(
		tCliff,
		[avoidClasses(clPlayer, 20, clHill, 5, clBaseResource, 3, clWomen, 5), stayClasses(clLand, 5)],
		clHill,
		scaleByMapSize(10, 60) * numPlayers);
Engine.SetProgress(40);

createHills(
	[tCliff, tCliff, tHill],
	avoidClasses(clPlayer, 20, clHill, 5, clBaseResource, 3, clWomen, 5, clLand, 5),
	clHill,
	scaleByMapSize(15, 90) * numPlayers,
	undefined,
	undefined,
	undefined,
	undefined,
	55);

Engine.SetProgress(50);

g_Map.log("Creating dirt patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]],
	[1, 1],
	[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12, clWomen, 5), stayClasses(clLand, 5)],
	scaleByMapSize(15, 45),
	clDirt);

g_Map.log("Creating grass patches");
createPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	tTier4Terrain,
	[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12, clWomen, 5), stayClasses(clLand, 5)],
	scaleByMapSize(15, 45),
	clDirt);

var planetm = 1;
if (currentBiome() == "generic/tropic")
	planetm = 8;

createDecoration(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
		[new SimpleObject(aGrass, 2,4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
		[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200)
	],
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)]);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 7, clHill, 1, clPlayer, 9), stayClasses(clLand, 7)],
	clForest,
	stragglerTrees);

g_Map.ExportMap();
