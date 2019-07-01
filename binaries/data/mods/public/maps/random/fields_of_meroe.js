Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

if (g_MapSettings.Biome)
	setSelectedBiome();
else
	setBiome("fields_of_meroe/dry");

const tMainDirt = g_Terrains.mainDirt;
const tSecondaryDirt = g_Terrains.secondaryDirt;
const tDirt = g_Terrains.dirt;
const tLush = "desert_grass_a";
const tSLush = "desert_grass_a_sand";
const tFarmland = "desert_farmland";
const tRoad = "savanna_tile_a";
const tRoadWild = "desert_city_tile";
const tRiverBank = "savanna_riparian_wet";
const tForestFloor = "savanna_forestfloor_b";

const oBush = g_Gaia.berry;
const oBaobab = "gaia/flora_tree_baobab";
const oAcacia = "gaia/flora_tree_acacia";
const oDatePalm = "gaia/flora_tree_date_palm";
const oSDatePalm = "gaia/flora_tree_cretan_date_palm_short";
const oGazelle = "gaia/fauna_gazelle";
const oGiraffe = "gaia/fauna_giraffe";
const oLion = "gaia/fauna_lion";
const oFish = "gaia/fauna_fish";
const oHawk = "gaia/fauna_hawk";
const oStoneLarge = "gaia/geology_stonemine_savanna_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";
const oMetalSmall = "gaia/geology_metal_desert_small";

const oHouse = "structures/kush_house";
const oFarmstead = "structures/kush_farmstead";
const oField = "structures/kush_field";
const oPyramid = "structures/kush_pyramid_small";
const oPyramidLarge = "structures/kush_pyramid_large";
const oKushUnits = isNomad() ?
	"units/kush_support_female_citizen" :
	"units/kush_infantry_javelinist_merc_e";

const aRain = g_Decoratives.rain;
const aBushA = g_Decoratives.bushA;
const aBushB = g_Decoratives.bushB;
const aBushes = [aBushA, aBushB];
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aRockA = g_Decoratives.rock;
const aRockB = "actor|geology/shoreline_large.xml";
const aRockC = "actor|geology/shoreline_small.xml";

const pForestP = [tForestFloor + TERRAIN_SEPARATOR + oAcacia, tForestFloor];

const heightSeaGround = g_Heights.seaGround;
const heightReedsDepth = -2.5;
const heightCataract = -1;
const heightShore = 1;
const heightLand = 2;
const heightDunes = 11;
const heightOffsetBump = 1.4;
const heightOffsetBumpPassage = 4;

const g_Map = new RandomMap(heightLand, tMainDirt);

const numPlayers = getNumPlayers();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clKushiteVillages = g_Map.createTileClass();
var clRiver = g_Map.createTileClass();
var clShore = g_Map.createTileClass();
var clDunes = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clRain = g_Map.createTileClass();
var clCataract = g_Map.createTileClass();

var kushVillageBuildings = {
	"houseA": { "template": oHouse, "offset": new Vector2D(5, 5) },
	"houseB": { "template": oHouse, "offset": new Vector2D(5, 0) },
	"houseC": { "template": oHouse, "offset": new Vector2D(5, -5) },
	"farmstead": { "template": oFarmstead, "offset": new Vector2D(-5, 0) },
	"fieldA": { "template": oField, "offset": new Vector2D(-5, 5) },
	"fieldB": { "template": oField, "offset": new Vector2D(-5, -5) },
	"pyramid": { "template": oPyramid, "offset": new Vector2D(0, -5) }
};

const riverTextures = [
	{
		"left": fractionToTiles(0),
		"right": fractionToTiles(0.04),
		"terrain": tLush,
		"tileClass": clShore
	},
	{
		"left": fractionToTiles(0.04),
		"right": fractionToTiles(0.06),
		"terrain": tSLush,
		"tileClass": clShore
	}
];

const riverAngle = Math.PI/5;

paintRiver({
	"parallel": false,
	"start": new Vector2D(fractionToTiles(0.25), mapBounds.top).rotateAround(riverAngle, mapCenter),
	"end": new Vector2D(fractionToTiles(0.25), mapBounds.bottom).rotateAround(riverAngle, mapCenter),
	"width": scaleByMapSize(12, 36),
	"fadeDist": scaleByMapSize(3, 12),
	"deviation": 1,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightShore,
	"meanderShort": 14,
	"meanderLong": 18,
	"waterFunc": (position, height, z) => {
		clRiver.add(position);
		createTerrain(tRiverBank).place(position);
	},
	"landFunc": (position, shoreDist1, shoreDist2) => {
		for (let riv of riverTextures)
			if (riv.left < +shoreDist1 && +shoreDist1 < riv.right || riv.left < -shoreDist2 && -shoreDist2 < riv.right)
				{
					riv.tileClass.add(position);
					if (riv.terrain)
						createTerrain(riv.terrain).place(position);
				}
	}
});
Engine.SetProgress(10);

g_Map.log("Creating cataracts");
for (let x of [fractionToTiles(randFloat(0.15, 0.25)), fractionToTiles(randFloat(0.75, 0.85))])
{
	let anglePassage = riverAngle + Math.PI / 2 * randFloat(0.8, 1.2);

	let areaPassage = createArea(
		new PathPlacer(
			new Vector2D(x, mapBounds.bottom).rotateAround(anglePassage, mapCenter),
			new Vector2D(x, mapBounds.top).rotateAround(anglePassage, mapCenter),
			scaleByMapSize(20, 30),
			0,
			1,
			0,
			0,
			Infinity),
		[
			new SmoothElevationPainter(ELEVATION_SET, heightCataract, 2),
			new TileClassPainter(clCataract)
		],
		new HeightConstraint(-Infinity, 0));

	createAreasInAreas(
		new ClumpPlacer(4, 0.4, 0.6, 0.5),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBumpPassage, 2),
		undefined,
		scaleByMapSize(15, 30),
		20,
		[areaPassage]);

	createObjectGroupsByAreas(
		new SimpleGroup([new SimpleObject(aReeds, 2, 4, 0, 1)], true),
		0,
		undefined,
		scaleByMapSize(20, 50),
		20,
		[areaPassage]);
}

var [playerIDs, playerPosition] = playerPlacementRandom(sortAllPlayers(), avoidClasses(clRiver, 15, clPlayer, 30));
placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad,
		"radius": 10,
		"width": 3,
		"painters": [new TileClassPainter(clPlayer)]
	},
	"Chicken": {
	},
	"Berries": {
		"template": oBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{
				"type": "stone_formation",
				"template": oStoneSmall,
				"terrain": tSecondaryDirt
			}
		],
		"groupElements": [new RandomObject(aBushes, 2, 4, 2, 3)]
	},
	"Trees": {
		"template": pickRandom([oBaobab, oAcacia]),
		"count": 3
	}
});
Engine.SetProgress(15);

g_Map.log("Getting random coordinates for Kushite settlements");
var kushiteTownPositions = [];
for (let retryCount = 0; retryCount < scaleByMapSize(3, 10); ++retryCount)
{
	let coordinate = g_Map.randomCoordinate(true);
	if (new AndConstraint(avoidClasses(clPlayer, 40, clForest, 5, clKushiteVillages, 50, clRiver, 15)).allows(coordinate))
	{
		kushiteTownPositions.push(coordinate);
		createArea(
			new ClumpPlacer(40, 0.6, 0.3, Infinity, coordinate),
			[
				new TerrainPainter(tRoad),
				new TileClassPainter(clKushiteVillages)
			]);
	}
}

g_Map.log("Placing the Kushite buildings");
for (let coordinate of kushiteTownPositions)
{
	for (let building in kushVillageBuildings)
		g_Map.placeEntityPassable(kushVillageBuildings[building].template, 0, Vector2D.add(coordinate, kushVillageBuildings[building].offset), Math.PI);

	createObjectGroup(new SimpleGroup([new SimpleObject(oKushUnits, 5, 7, 1, 2)], true, clKushiteVillages, coordinate), 0);
}

g_Map.log("Creating kushite pyramids");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oPyramidLarge, 1, 1, 0, 1)], true, clKushiteVillages),
	0,
	avoidClasses(clPlayer, 20, clForest, 5, clKushiteVillages, 30, clRiver, 10),
	scaleByMapSize(1, 7),
	200);
Engine.SetProgress(20);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	new StaticConstraint(avoidClasses(clPlayer, 5, clKushiteVillages, 10, clRiver, 20)),
	scaleByMapSize(300, 800));

g_Map.log("Creating dunes");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(5, 15)), 0.5),
	[
		new SmoothElevationPainter(ELEVATION_SET, heightDunes, 2),
		new TileClassPainter(clDunes)
	],
	avoidClasses(clPlayer, 3, clRiver, 20, clDunes, 10, clKushiteVillages, 10),
	scaleByMapSize(1, 3) * numPlayers * 3);

Engine.SetProgress(25);

var [forestTrees, stragglerTrees] = getTreeCounts(400, 2000, 0.7);
createForests(
	[tMainDirt[0], tForestFloor, tForestFloor, pForestP, pForestP],
	avoidClasses(clPlayer, 20, clForest, 20, clDunes, 2, clRiver, 20, clKushiteVillages, 10),
	clForest,
	forestTrees);
Engine.SetProgress(40);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		new LayeredPainter([tSecondaryDirt, tDirt], [1]),
		avoidClasses(clDunes, 0, clForest, 0, clPlayer, 5, clRiver, 10),
		scaleByMapSize(50, 90));

g_Map.log("Creating patches of farmland");
for (let size of [scaleByMapSize(30, 40), scaleByMapSize(35, 50)])
	createAreas(
		new ClumpPlacer(size, 0.4, 0.6),
		new TerrainPainter(tFarmland),
		avoidClasses(clDunes, 3, clForest, 3, clPlayer, 5, clKushiteVillages, 5, clRiver, 10),
		scaleByMapSize(1, 10));
Engine.SetProgress(60);

g_Map.log("Creating stone mines");
createObjectGroups(
	new SimpleGroup(
		[
			new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1),
			new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)
		], true, clRock),
	0,
	avoidClasses(clRiver, 4, clCataract, 4, clPlayer, 20, clRock, 15, clKushiteVillages, 5, clDunes, 2, clForest, 4),
	scaleByMapSize(2, 8),
	50);

g_Map.log("Creating small stone quarries");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3, 0, 2 * Math.PI, 1)], true, clRock),
	0,
	avoidClasses(clRiver, 4, clCataract, 4, clPlayer, 20, clRock, 15, clKushiteVillages, 5, clDunes, 2, clForest, 4),
	scaleByMapSize(2, 8),
	50);

g_Map.log("Creating metal mines");
createObjectGroups(
	new SimpleGroup(
		[
			new SimpleObject(oMetalSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1),
			new SimpleObject(oMetalLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)
		], true, clMetal),
	0,
	avoidClasses(clRiver, 4, clCataract, 4, clPlayer, 20, clRock, 10, clMetal, 15, clKushiteVillages, 5, clDunes, 2, clForest, 4),
	scaleByMapSize(2, 8),
	50);

g_Map.log("Creating small metal quarries");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oMetalSmall, 2, 5, 1, 3, 0, 2 * Math.PI, 1)], true, clMetal),
	0,
	avoidClasses(clRiver, 4, clCataract, 4, clPlayer, 20, clRock, 10, clMetal, 15, clKushiteVillages, 5, clDunes, 2, clForest, 4),
	scaleByMapSize(2, 8),
	50);
Engine.SetProgress(70);

g_Map.log("Creating gazelle");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGazelle, 4, 6, 1, 4)], true, clFood),
	0,
	avoidClasses(clForest, 0, clKushiteVillages, 10, clPlayer, 5, clDunes, 1, clFood, 25, clRiver, 2, clMetal, 4, clRock, 4),
	2 * numPlayers,
	50);

g_Map.log("Creating giraffe");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGiraffe, 4, 6, 1, 4)], true, clFood),
	0,
	avoidClasses(clForest, 0, clKushiteVillages, 10, clPlayer, 5, clDunes, 1, clFood, 25, clRiver, 2, clMetal, 4, clRock, 4),
	2 * numPlayers,
	50);

g_Map.log("Creating lions");
if (!isNomad())
	createObjectGroups(
		new SimpleGroup([new SimpleObject(oLion, 2, 3, 0, 2)], true, clFood),
		0,
		avoidClasses(clForest, 0, clKushiteVillages, 10, clPlayer, 5, clDunes, 1, clFood, 25, clRiver, 2, clMetal, 4, clRock, 4),
		3 * numPlayers,
		50);

g_Map.log("Creating hawk");
for (let i = 0; i < scaleByMapSize(1, 3); ++i)
	g_Map.placeEntityAnywhere(oHawk, 0, mapCenter, randomAngle());

g_Map.log("Creating fish");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oFish, 1, 2, 0, 1)], true, clFood),
	0,
	[stayClasses(clRiver, 4), avoidClasses(clFood, 16, clCataract, 10)],
	scaleByMapSize(15, 80),
	50);
Engine.SetProgress(80);

createStragglerTrees(
	[oBaobab, oAcacia],
	avoidClasses(clForest, 3, clFood, 1, clDunes, 1, clPlayer, 1, clMetal, 6, clRock, 6, clRiver, 15, clKushiteVillages, 15),
	clForest,
	stragglerTrees);

createStragglerTrees(
	[oBaobab, oAcacia],
	avoidClasses(clForest, 1, clFood, 1, clDunes, 3, clPlayer, 1, clMetal, 6, clRock, 6, clRiver, 15, clKushiteVillages, 15),
	clForest,
	stragglerTrees * (isNomad() ? 3 : 1));

createStragglerTrees(
	[oDatePalm, oSDatePalm],
	[avoidClasses(clPlayer, 5, clFood, 1), stayClasses(clShore, 2)],
	clForest,
	stragglerTrees * 10);

Engine.SetProgress(90);

g_Map.log("Creating reeds on the shore");
createObjectGroups(
	new SimpleGroup([new SimpleObject(aReeds, 3, 5, 0, 1)], true),
	0,
	[
		new HeightConstraint(heightReedsDepth, heightShore),
		avoidClasses(clCataract, 2)
	],
	scaleByMapSize(500, 1000),
	50);

g_Map.log("Creating small decorative rocks");
createObjectGroups(
	new SimpleGroup([new SimpleObject(aRockA, 2, 4, 0, 1)], true),
	0,
	avoidClasses(clForest, 0, clPlayer, 0, clDunes, 0, clRiver, 5, clCataract, 5, clMetal, 4, clRock, 4),
	scaleByMapSize(16, 262),
	50);
createObjectGroups(
	new SimpleGroup([new SimpleObject(aRockB, 1, 2, 0, 1), new SimpleObject(aRockC, 1, 3, 0, 1)], true),
	0,
	[
		new NearTileClassConstraint(clCataract, 5),
		new HeightConstraint(-Infinity, heightShore)
	],
	scaleByMapSize(30, 50),
	50);

g_Map.log("Creating bushes");
createObjectGroups(
	new SimpleGroup([new SimpleObject(aBushB, 1, 2, 0, 1), new SimpleObject(aBushA, 1, 3, 0, 2)], true),
	0,
	avoidClasses(clForest, 0, clPlayer, 0, clDunes, 0, clRiver, 15, clMetal, 4, clRock, 4),
	scaleByMapSize(50, 500),
	50);
Engine.SetProgress(95);

g_Map.log("Creating rain drops");
if (aRain)
	createObjectGroups(
		new SimpleGroup([new SimpleObject(aRain, 1, 1, 1, 4)], true, clRain),
		0,
		avoidClasses(clRain, 5),
		scaleByMapSize(60, 200));
Engine.SetProgress(98);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clKushiteVillages, 18, clMetal, 4, clRock, 4, clDunes, 4, clFood, 2, clRiver, 5));

setSunElevation(Math.PI / 8);
setSunRotation(randomAngle());
setSunColor(0.746, 0.718, 0.539);
setWaterColor(0.292, 0.347, 0.691);
setWaterTint(0.550, 0.543, 0.437);

setFogColor(0.8, 0.76, 0.61);
setFogThickness(0.2);
setFogFactor(0.2);

setPPEffect("hdr");
setPPContrast(0.65);
setPPSaturation(0.42);
setPPBloom(0.6);

g_Map.ExportMap();
