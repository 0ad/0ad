Engine.LoadLibrary("rmgen");

const tSand = ["desert_sand_dunes_100", "desert_dirt_cracks","desert_sand_smooth", "desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_smooth"];
const tDune = ["desert_sand_dunes_50"];
const tForestFloor = "desert_forestfloor_palms";
const tDirt = ["desert_dirt_rough","desert_dirt_rough","desert_dirt_rough", "desert_dirt_rough_2", "desert_dirt_rocks_2"];
const tRoad = "desert_city_tile";;
const tRoadWild = "desert_city_tile";;
const tShore = "dirta";
const tWater = "desert_sand_wet";

const ePalmShort = "gaia/flora_tree_cretan_date_palm_short";
const ePalmTall = "gaia/flora_tree_cretan_date_palm_tall";
const eBush = "gaia/flora_bush_grapes";
const eCamel = "gaia/fauna_camel";
const eGazelle = "gaia/fauna_gazelle";
const eLion = "gaia/fauna_lion";
const eLioness = "gaia/fauna_lioness";
const eStoneMine = "gaia/geology_stonemine_desert_quarry";
const eMetalMine = "gaia/geology_metal_desert_slabs";

const aFlower1 = "actor|props/flora/decals_flowers_daisies.xml";
const aWaterFlower = "actor|props/flora/water_lillies.xml";
const aReedsA = "actor|props/flora/reeds_pond_lush_a.xml";
const aReedsB = "actor|props/flora/reeds_pond_lush_b.xml";
const aRock = "actor|geology/stone_desert_med.xml";
const aBushA = "actor|props/flora/bush_desert_dry_a.xml";
const aBushB = "actor|props/flora/bush_desert_dry_a.xml";
const aSand = "actor|particle/blowing_sand.xml";

const pForestMain = [tForestFloor + TERRAIN_SEPARATOR + ePalmShort, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor];
const pOasisForestLight = [tForestFloor + TERRAIN_SEPARATOR + ePalmShort, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor,tForestFloor,tForestFloor
					,tForestFloor,tForestFloor,tForestFloor,tForestFloor];

const heightSeaGround = -3;
const heightFloraMin = -2.5
const heightFloraReedsMax = -1.9;
const heightFloraMax = -1;
const heightLand = 1;
const heightSand = 3.4;
const heightOasisPath = 4;
const heightOffsetBump = 4;
const heightOffsetDune = 18;

var g_Map = new RandomMap(heightLand, tSand);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clOasis = g_Map.createTileClass();
var clPassage = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

var waterRadius = scaleByMapSize(7, 50)
var shoreDistance = scaleByMapSize(4, 10);
var forestDistance = scaleByMapSize(6, 20);

var [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

log("Creating small oasis near the players...")
var forestDist = 1.2 * defaultPlayerBaseRadius();
for (let i = 0; i < numPlayers; ++i)
{
	// Create starting batches of wood
	let forestPosition;
	let forestAngle;

	do {
		forestAngle = Math.PI / 3 * randFloat(1, 2);
		forestPosition = Vector2D.add(playerPosition[i], new Vector2D(forestDist, 0).rotate(-forestAngle));
	} while (
		!createArea(
			new ClumpPlacer(70, 1, 0.5, 10, forestPosition),
			[
				new LayeredPainter([tForestFloor, pForestMain], [0]),
				paintClass(clBaseResource)
			],
			avoidClasses(clBaseResource, 0)));

	log("Creating the water patch explaining the forest for player " + playerIDs[i] + "...");
	let waterPosition;
	do {
		let waterAngle = forestAngle + randFloat(1, 5) / 3 * Math.PI;
		waterPosition = Vector2D.add(forestPosition, new Vector2D(6, 0).rotate(-waterAngle)).round();

		let flowerPosition = Vector2D.add(forestPosition, new Vector2D(3, 0).rotate(-waterAngle)).round();
		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(aFlower1, 1, 5, 0, 3)],
				true,
				undefined,
				flowerPosition.x,
				flowerPosition.y),
			0);

		let reedsPosition = Vector2D.add(forestPosition, new Vector2D(5, 0).rotate(-waterAngle)).round();
		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(aReedsA, 1, 3, 0, 0)],
				true,
				undefined,
				reedsPosition.x,
				reedsPosition.y),
			0);

	} while (
		!createArea(
			new ClumpPlacer(60, 0.9, 0.4, 5, waterPosition),
			[
				new LayeredPainter([tShore, tWater], [1]),
				new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 3)
			],
			avoidClasses(clBaseResource, 0)));
}
Engine.SetProgress(20);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad,
		"painters": [
			paintClass(clPlayer)
		]
	},
	"Chicken": {
	},
	"Berries": {
		"template": eBush
	},
	"Mines": {
		"types": [
			{ "template": eMetalMine },
			{ "template": eStoneMine },
		],
		"distance": defaultPlayerBaseRadius(),
		"maxAngle": Math.PI / 2,
		"groupElements": shuffleArray([aBushA, aBushB, ePalmShort, ePalmTall]).map(t => new SimpleObject(t, 1, 1, 3, 4))
	}
	// Starting trees were set above
	// No decoratives
});
Engine.SetProgress(30);

log("Creating central oasis...");
createArea(
	new ClumpPlacer(diskArea(forestDistance + shoreDistance + waterRadius), 0.8, 0.2, 10, mapCenter),
	[
		new LayeredPainter([pOasisForestLight, tWater], [forestDistance]),
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, forestDistance + shoreDistance),
		paintClass(clOasis)
	]);

Engine.SetProgress(40);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 3),
	avoidClasses(clPlayer, 10, clBaseResource, 6, clOasis, 4),
	scaleByMapSize(30, 70));

log("Creating dirt patches...");
createAreas(
	new ClumpPlacer(80, 0.3, 0.06, 1),
	new TerrainPainter(tDirt),
	avoidClasses(clPlayer, 10, clBaseResource, 6, clOasis, 4, clForest, 4),
	scaleByMapSize(15, 50));

log("Creating dunes...");
createAreas(
	new ClumpPlacer(120, 0.3, 0.06, 1),
	[
		new TerrainPainter(tDune),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetDune, 30)
	],
	avoidClasses(clPlayer, 10, clBaseResource, 6, clOasis, 4, clForest, 4),
	scaleByMapSize(15, 50));

Engine.SetProgress(50);

if (mapSize > 150 && randBool())
{
	log("Creating path though the oasis...");
	let pathWidth = scaleByMapSize(7, 18);
	let points = distributePointsOnCircle(2, randomAngle(), waterRadius + shoreDistance + forestDistance + pathWidth, mapCenter)[0];
	createArea(
		new PathPlacer(points[0], points[1], pathWidth, 0.4, 1, 0.2, 0),
		[
			new TerrainPainter(tSand),
			new SmoothElevationPainter(ELEVATION_SET, heightOasisPath, 5),
			paintClass(clPassage)
		]);
}
log("Creating some straggler trees around the passage...");
var group = new SimpleGroup([new SimpleObject(ePalmTall, 1,1, 0,0),new SimpleObject(ePalmShort, 1, 2, 1, 2), new SimpleObject(aBushA, 0,2, 1,3)], true, clForest);
createObjectGroupsDeprecated(group, 0, stayClasses(clPassage, 3), scaleByMapSize(60, 250), 100);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(eStoneMine, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 3,3),new SimpleObject(ePalmTall, 0,1, 3,3)
						 ,new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 10, clForest, 1, clPlayer, 30, clRock, 10,clBaseResource, 2, clHill, 1),
	scaleByMapSize(6,25), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 2,3),new SimpleObject(ePalmTall, 0,1, 2,2)
						 ,new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 10, clForest, 1, clPlayer, 30, clMetal, 10,clBaseResource, 2, clRock, 10, clHill, 1),
	scaleByMapSize(6,25), 100
);
Engine.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRock, 2,4, 0,2)], true, undefined );
createObjectGroupsDeprecated(group, 0, avoidClasses(clOasis, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), 30, scaleByMapSize(10, 50));

Engine.SetProgress(70);

log("Creating camels...");
group = new SimpleGroup(
	[new SimpleObject(eCamel, 1,2, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	1 * numPlayers, 50
);
Engine.SetProgress(75);

log("Creating gazelles...");
group = new SimpleGroup(
	[new SimpleObject(eGazelle, 2,4, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	1 * numPlayers, 50
);
Engine.SetProgress(85);

log("Creating oasis animals...");
for (let i = 0; i < scaleByMapSize(5, 30); ++i)
{
	let animalPos = Vector2D.add(mapCenter, new Vector2D(forestDistance + shoreDistance + waterRadius, 0).rotate(randomAngle()));

	createObjectGroup(
		new RandomGroup(
			[
				new SimpleObject(eLion, 1, 2, 0, 4),
				new SimpleObject(eLioness, 1, 2, 2, 4),
				new SimpleObject(eGazelle, 4, 6, 1, 5),
				new SimpleObject(eCamel, 1, 2, 1, 5)
			],
			true,
			clFood,
			animalPos.x,
			animalPos.y),
		0);
}
Engine.SetProgress(90);

log("Creating bushes...");
var group = new SimpleGroup(
	[new SimpleObject(aBushB, 1,2, 0,2), new SimpleObject(aBushA, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clOasis, 2, clHill, 1, clPlayer, 1, clPassage, 1),
	scaleByMapSize(10, 40), 20
);

log("Creating sand blows and beautifications");
for (var sandx = 0; sandx < mapSize; sandx += 4)
	for (var sandz = 0; sandz < mapSize; sandz += 4)
	{
		let position = new Vector2D(sandx, sandz);
		let height = g_Map.getHeight(position);

		if (height > heightSand)
		{
			if (randBool((height - heightSand) / 1.4))
			{
				group = new SimpleGroup( [new SimpleObject(aSand, 0,1, 0,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}
		}
		else if (height > heightFloraMin && height < heightFloraMax)
		{
			if (randBool(0.4))
			{
				group = new SimpleGroup( [new SimpleObject(aWaterFlower, 1,4, 1,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}
			else if (randBool(0.7) && height < heightFloraReedsMax)
			{
				group = new SimpleGroup( [new SimpleObject(aReedsA, 5,12, 0,2),new SimpleObject(aReedsB, 5,12, 0,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}

			if (getTileClass(clPassage).countMembersInRadius(sandx, sandz, 2) > 0)
			{
				if (randBool(0.4))
				{
					group = new SimpleGroup( [new SimpleObject(aWaterFlower, 1,4, 1,2)], true, undefined, sandx,sandz );
					createObjectGroup(group, 0);
				}
				else if (randBool(0.7) && height < heightFloraReedsMax)
				{
					group = new SimpleGroup( [new SimpleObject(aReedsA, 5,12, 0,2),new SimpleObject(aReedsB, 5,12, 0,2)], true, undefined, sandx,sandz );
					createObjectGroup(group, 0);
				}
			}
		}
	}

placePlayersNomad(clPlayer, avoidClasses(clOasis, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet("sunny");
setSunColor(0.914,0.827,0.639);
setSunRotation(Math.PI/3);
setSunElevation(0.5);
setWaterColor(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1.0);
setWaterType("clap");
setWaterMurkiness(0.5);
setTerrainAmbientColor(0.45, 0.5, 0.6);
setUnitsAmbientColor(0.501961, 0.501961, 0.501961);

g_Map.ExportMap();
