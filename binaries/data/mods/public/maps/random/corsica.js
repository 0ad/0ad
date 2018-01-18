Engine.LoadLibrary("rmgen");

var tGrass = ["medit_grass_field", "medit_grass_field_b", "temp_grass_c"];
var tLushGrass = ["medit_grass_field","medit_grass_field_a"];

var tSteepCliffs = ["temp_cliff_b", "temp_cliff_a"];
var tCliffs = ["temp_cliff_b", "medit_cliff_italia", "medit_cliff_italia_grass"];
var tHill = ["medit_cliff_italia_grass","medit_cliff_italia_grass", "medit_grass_field", "medit_grass_field", "temp_grass"];
var tMountain = ["medit_cliff_italia_grass","medit_cliff_italia"];

var tRoad = ["medit_city_tile","medit_rocks_grass","medit_grass_field_b"];
var tRoadWild = ["medit_rocks_grass","medit_grass_field_b"];

var tShoreBlend = ["medit_sand_wet","medit_rocks_wet"];
var tShore = ["medit_rocks","medit_sand","medit_sand"];
var tSandTransition = ["medit_sand","medit_rocks_grass","medit_rocks_grass","medit_rocks_grass"];
var tVeryDeepWater = ["medit_sea_depths","medit_sea_coral_deep"];
var tDeepWater = ["medit_sea_coral_deep","tropic_ocean_coral"];
var tCreekWater = "medit_sea_coral_plants";

var ePine = "gaia/flora_tree_aleppo_pine";
var ePalmTall = "gaia/flora_tree_cretan_date_palm_tall";
var eFanPalm = "gaia/flora_tree_medit_fan_palm";
var eApple = "gaia/flora_tree_apple";
var eBush = "gaia/flora_bush_berry";
var eFish = "gaia/fauna_fish";
var ePig = "gaia/fauna_pig";
var eStoneMine = "gaia/geology_stonemine_medit_quarry";
var eMetalMine = "gaia/geology_metal_mediterranean_slabs";

var aRock = "actor|geology/stone_granite_med.xml";
var aLargeRock = "actor|geology/stone_granite_large.xml";
var aBushA = "actor|props/flora/bush_medit_sm_lush.xml";
var aBushB = "actor|props/flora/bush_medit_me_lush.xml";
var aPlantA = "actor|props/flora/plant_medit_artichoke.xml";
var aPlantB = "actor|props/flora/grass_tufts_a.xml";
var aPlantC = "actor|props/flora/grass_soft_tuft_a.xml";

var aStandingStone = "actor|props/special/eyecandy/standing_stones.xml";

var heightSeaGround = -8;
var heightCreeks = -5;
var heightBeaches = -1;
var heightMain = 5;

var heightOffsetMainRelief = 30;
var heightOffsetLevel1 = 9;
var heightOffsetLevel2 = 8;
var heightOffsetBumps = 2;
var heightOffsetAntiBumps = -5;

InitMap(heightSeaGround, tVeryDeepWater);

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapCenter = getMapCenter();

var clIsland = createTileClass();
var clCreek = createTileClass();
var clWater = createTileClass();
var clCliffs = createTileClass();
var clForest = createTileClass();
var clShore = createTileClass();
var clPlayer = createTileClass();
var clBaseResource = createTileClass();
var clPassage = createTileClass();
var clSettlement = createTileClass();

var radiusBeach = fractionToTiles(0.57);
var radiusCreeks = fractionToTiles(0.52);
var radiusIsland = fractionToTiles(0.4);
var radiusLevel1 = fractionToTiles(0.35);
var radiusPlayer = fractionToTiles(0.25);
var radiusLevel2 = fractionToTiles(0.2);

var creeksArea = () => randBool() ? randFloat(10, 50) : scaleByMapSize(75, 100) + randFloat(0, 20);

var nbCreeks = scaleByMapSize(6, 15);
var nbSubIsland = 5;
var nbBeaches = scaleByMapSize(2, 5);
var nbPassagesLevel1 = scaleByMapSize(4, 8);
var nbPassagesLevel2 = scaleByMapSize(2, 4);

log("Creating Corsica and Sardinia...");
var swapAngle = randBool() ? Math.PI / 2 : 0;
var islandLocations = [new Vector2D(0.05, 0.05), new Vector2D(0.95, 0.95)].map(v => v.mult(mapSize).rotateAround(-swapAngle, mapCenter));

for (let island = 0; island < 2; ++island)
{
	log("Creating island area...");
	createArea(
		new ClumpPlacer(diskArea(radiusIsland), 1, 0.5, 10, islandLocations[island].x, islandLocations[island].y),
		[
			new LayeredPainter([tCliffs, tGrass], [2]),
			new SmoothElevationPainter(ELEVATION_SET, heightMain, 0),
			paintClass(clIsland)
		]);

	log("Creating subislands...");
	for (let i = 0; i < nbSubIsland + 1; ++i)
	{
		let angle = Math.PI * (island + i / (nbSubIsland * 2)) + swapAngle;
		let location = Vector2D.add(islandLocations[island], new Vector2D(radiusIsland, 0).rotate(-angle));
		createArea(
			new ClumpPlacer(fractionToSize(0.05) / 2, 0.6, 0.03, 10, location.x, location.y),
			[
				new LayeredPainter([tCliffs, tGrass], [2]),
				new SmoothElevationPainter(ELEVATION_SET, heightMain, 1),
				paintClass(clIsland)
			]);
	}

	log("Creating creeks...");
	for (let i = 0; i < nbCreeks + 1; ++i)
	{
		let angle = Math.PI * (island + i * (1 / (nbCreeks * 2))) + swapAngle;
		let location = Vector2D.add(islandLocations[island], new Vector2D(radiusCreeks, 0).rotate(-angle));
		createArea(
			new ClumpPlacer(creeksArea(), 0.4, 0.01, 10, location.x, location.y),
			[
				new TerrainPainter(tSteepCliffs),
				new SmoothElevationPainter(ELEVATION_SET, heightCreeks, 0),
				paintClass(clCreek)
			]);
	}

	log("Creating beaches...");
	for (let i = 0; i < nbBeaches + 1; ++i)
	{
		let angle = Math.PI * (island + (i / (nbBeaches * 2.5)) + 1 / (nbBeaches * 6) + randFloat(-1, 1) / (nbBeaches * 7)) + swapAngle;
		let start = Vector2D.add(islandLocations[island], new Vector2D(radiusIsland, 0).rotate(-angle));
		let end = Vector2D.add(islandLocations[island], new Vector2D(radiusBeach, 0).rotate(-angle));

		createArea(
			new ClumpPlacer(130, 0.7, 0.8, 10, Math.round((start.x + end.x * 3) / 4), Math.round((start.y + end.y * 3) / 4)),
			new SmoothElevationPainter(ELEVATION_SET, heightBeaches, 5));

		createPassage({
			"start": start,
			"end": end,
			"startWidth": 18,
			"endWidth": 25,
			"smoothWidth": 4,
			"tileClass": clShore
		});
	}

	log("Creating main relief...");
	createArea(
		new ClumpPlacer(diskArea(radiusIsland), 1, 0.2, 10, islandLocations[island].x, islandLocations[island].y),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetMainRelief, fractionToTiles(0.45)));

	log("Creating first level plateau...");
	createArea(
		new ClumpPlacer(diskArea(radiusLevel1), 0.95, 0.02, 10, islandLocations[island].x, islandLocations[island].y),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetLevel1, 1));

	log("Creating first level passages...");
	for (let i = 0; i <= nbPassagesLevel1; ++i)
	{
		let angle = Math.PI * (i / 7 + 1 / 9 + island) + swapAngle;
		createPassage({
			"start": Vector2D.add(islandLocations[island], new Vector2D(radiusLevel1 + 10, 0).rotate(-angle)),
			"end": Vector2D.add(islandLocations[island], new Vector2D(radiusLevel1 - 4, 0).rotate(-angle)),
			"startWidth": 10,
			"endWidth": 6,
			"smoothWidth": 3,
			"tileClass": clPassage
		});
	}

	if (mapSize > 150)
	{
		log("Creating second level plateau...");
		createArea(
			new ClumpPlacer(diskArea(radiusLevel2), 0.98, 0.04, 10, islandLocations[island].x, islandLocations[island].y),
			[
				new LayeredPainter([tCliffs, tGrass], [2]),
				new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetLevel2, 1)
			]);

		log("Creating second level passages...");
		for (let i = 0; i < nbPassagesLevel2; ++i)
		{
			let angle = Math.PI * (i / (2 * nbPassagesLevel2) + 1 / (4 * nbPassagesLevel2) + island) + swapAngle;
			createPassage({
				"start": Vector2D.add(islandLocations[island], new Vector2D(radiusLevel2 + 3, 0).rotate(-angle)),
				"end": Vector2D.add(islandLocations[island], new Vector2D(radiusLevel2 - 6, 0).rotate(-angle)),
				"startWidth": 4,
				"endWidth": 6,
				"smoothWidth": 2,
				"tileClass": clPassage
			});
		}
	}
}
Engine.SetProgress(30);

log("Determining player locations...");
var playerIDs = sortAllPlayers();
var playerPosition = [];
var playerAngle = [];
var p = 0;
for (let island = 0; island < 2; ++island)
{
	let playersPerIsland = island == 0 ? Math.ceil(numPlayers / 2) : Math.floor(numPlayers / 2);

	for (let i = 0; i < playersPerIsland; ++i)
	{
		playerAngle[p] = Math.PI * ((i + 0.5) / (2 * playersPerIsland) + island) + swapAngle;
		playerPosition[p] = Vector2D.add(islandLocations[island], new Vector2D(radiusPlayer).rotate(-playerAngle[p]));
		++p;
	}
}

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": false,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad,
		"coherence": 0.8,
		"radius": 6,
		"painters": [
			paintClass(clSettlement)
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
			{ "template": eStoneMine }
		]
	}
	// Sufficient starting trees around, no decoratives
});
Engine.SetProgress(40);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(70, 0.6, 0.1, 4),
	[new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBumps, 3)],
	[
		stayClasses(clIsland, 2),
		avoidClasses(clPlayer, 6, clPassage, 2)
	],
	scaleByMapSize(20, 100),
	5);

log("Creating anti bumps...");
createAreas(
	new ClumpPlacer(120, 0.3, 0.1, 4),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetAntiBumps, 6),
	avoidClasses(clPlayer, 6, clPassage, 2, clIsland, 2),
	scaleByMapSize(20, 100),
	5);

log("Painting water...");
paintTileClassBasedOnHeight(-Infinity, 0, Elevation_ExcludeMin_ExcludeMax, clWater);

log("Painting land...");
for (let mapX = 0; mapX < mapSize; ++mapX)
	for (let mapZ = 0; mapZ < mapSize; ++mapZ)
	{
		let terrain = getCosricaSardiniaTerrain(mapX, mapZ);
		if (!terrain)
			continue;

		createTerrain(terrain).place(mapX, mapZ);

		if (terrain == tCliffs || terrain == tSteepCliffs)
			addToClass(mapX, mapZ, clCliffs);
	}

function getCosricaSardiniaTerrain(mapX, mapZ)
{
	let isWater = getTileClass(clWater).countMembersInRadius(mapX, mapZ, 3);
	let isShore = getTileClass(clShore).countMembersInRadius(mapX, mapZ, 2);
	let isPassage = getTileClass(clPassage).countMembersInRadius(mapX, mapZ, 2);
	let isSettlement = getTileClass(clSettlement).countMembersInRadius(mapX, mapZ, 2);

	if (isSettlement)
		return undefined;

	let height = getHeight(mapX, mapZ);
	let heightDiff = getHeightDiff(mapX, mapZ);

	if (height >= 0.5 && height < 1.5 && isShore)
		return tSandTransition;

	// Paint land cliffs and grass
	if (height >= 1 && !isWater)
	{
		if (isPassage)
			return tGrass;

		if (heightDiff >= 10)
			return height > 25 ? tSteepCliffs : tCliffs;

		if (height < 17)
			return tGrass;

		if (heightDiff < 5)
			return tHill;

		return tMountain;
	}

	if (heightDiff >= 9)
		return tCliffs;

	if (height >= 1.5)
		return undefined;

	if (height >= -0.75)
		return tShore;

	if (height >= -3)
		return tShoreBlend;

	if (height >= -6)
		return tCreekWater;

	if (height > -10 && heightDiff < 6)
		return tDeepWater;

	return undefined;
}

Engine.SetProgress(65);

log("Creating mines...");
for (let mine of [eMetalMine, eStoneMine])
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[
				new SimpleObject(mine, 1,1, 0,0),
				new SimpleObject(aBushB, 1,1, 2,2),
				new SimpleObject(aBushA, 0,2, 1,3)
			],
			true,
			clBaseResource),
		0,
		[
			stayClasses(clIsland, 1),
			avoidClasses(
				clWater, 3,
				clPlayer, 6,
				clBaseResource, 4,
				clPassage, 2,
				clCliffs, 1)
		],
		scaleByMapSize(6, 25),
		1000);

log("Creating grass patches...");
createAreas(
	new ClumpPlacer(20, 0.3, 0.06, 0.5),
	[
		new TerrainPainter(tLushGrass),
		paintClass(clForest)
	],
	avoidClasses(
		clWater, 1,
		clPlayer, 6,
		clBaseResource, 3,
		clCliffs, 1),
	scaleByMapSize(10, 40));

log("Creating forests...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(ePine, 3, 6, 1, 3),
			new SimpleObject(ePalmTall, 1, 3, 1, 3),
			new SimpleObject(eFanPalm, 0, 2, 0, 2),
			new SimpleObject(eApple, 0, 1, 1, 2)
		],
		true,
		clForest),
	0,
	[
		stayClasses(clIsland, 3),
		avoidClasses(
			clWater, 1,
			clForest, 0,
			clPlayer, 3,
			clBaseResource, 4,
			clPassage, 2,
			clCliffs, 2)
	],
	scaleByMapSize(350, 2500),
	100);

Engine.SetProgress(75);

log("Creating small decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aRock, 1, 3, 0, 1),
			new SimpleObject(aStandingStone, 0, 2, 0, 3)
		],
		true),
	0,
	avoidClasses(
		clWater, 0,
		clForest, 0,
		clPlayer, 6,
		clBaseResource, 4,
		clPassage, 2),
	scaleByMapSize(16, 262),
	50);

log("Creating large decorative rocks...");
var rocksGroup = new SimpleGroup(
	[
		new SimpleObject(aLargeRock, 1, 2, 0, 1),
		new SimpleObject(aRock, 1, 3, 0, 2)
	],
	true);

createObjectGroupsDeprecated(
	rocksGroup,
	0,
	avoidClasses(
		clWater, 0,
		clForest, 0,
		clPlayer, 6,
		clBaseResource, 4,
		clPassage, 2),
	scaleByMapSize(8, 131),
	50);

createObjectGroupsDeprecated(
	rocksGroup,
	0,
	borderClasses(clWater, 5, 10),
	scaleByMapSize(100, 800),
	500);

log("Creating decorative plants...");
var plantGroups = [
	new SimpleGroup(
		[
			new SimpleObject(aPlantA, 3, 7, 0, 3),
			new SimpleObject(aPlantB, 3,6, 0, 3),
			new SimpleObject(aPlantC, 1,4, 0, 4)
		],
		true),
	new SimpleGroup(
		[
			new SimpleObject(aPlantB, 5, 20, 0, 5),
			new SimpleObject(aPlantC, 4,10, 0,4)
		],
		true)
];
for (let group of plantGroups)
	createObjectGroupsDeprecated(
		group,
		0,
		avoidClasses(
			clWater, 0,
			clBaseResource, 4,
			clShore, 3),
		scaleByMapSize(100, 600),
		50);

Engine.SetProgress(80);

log("Creating animals...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(ePig, 2,4, 0,3)]),
	0,
	avoidClasses(
		clWater, 3,
		clBaseResource, 4,
		clPlayer, 6),
	scaleByMapSize(20, 100),
	50);

log("Creating fish...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(eFish, 1,2, 0,3)]),
	0,
	[
		stayClasses(clWater, 3),
		avoidClasses(clCreek, 3, clShore, 3)
	],
	scaleByMapSize(50, 150),
	100);

Engine.SetProgress(95);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clBaseResource, 4, clCliffs, 4));

setSkySet(pickRandom(["cumulus", "sunny"]));

setSunColor(0.8, 0.66, 0.48);
setSunElevation(0.828932);
setSunRotation((swapAngle ? 0.288 : 0.788) * Math.PI);

setTerrainAmbientColor(0.564706,0.543726,0.419608);
setUnitsAmbientColor(0.53,0.55,0.45);
setWaterColor(0.2,0.294,0.49);
setWaterTint(0.208, 0.659, 0.925);
setWaterMurkiness(0.72);
setWaterWaviness(2.0);
setWaterType("ocean");

ExportMap();

// no need for preliminary rounding
function getHeightDiff(x1, z1)
{
	var height = getHeight(Math.round(x1),Math.round(z1));
	var diff = 0;
	if (z1 + 1 < mapSize)
		diff += Math.abs(getHeight(Math.round(x1),Math.round(z1+1)) - height);
	if (x1 + 1 < mapSize && z1 + 1 < mapSize)
		diff += Math.abs(getHeight(Math.round(x1+1),Math.round(z1+1)) - height);
	if (x1 + 1 < mapSize)
		diff += Math.abs(getHeight(Math.round(x1+1),Math.round(z1)) - height);
	if (x1 + 1 < mapSize && z1 - 1 >= 0)
		diff += Math.abs(getHeight(Math.round(x1+1),Math.round(z1-1)) - height);
	if (z1 - 1 >= 0)
		diff += Math.abs(getHeight(Math.round(x1),Math.round(z1-1)) - height);
	if (x1 - 1 >= 0 && z1 - 1 >= 0)
		diff += Math.abs(getHeight(Math.round(x1-1),Math.round(z1-1)) - height);
	if (x1 - 1 >= 0)
		diff += Math.abs(getHeight(Math.round(x1-1),Math.round(z1)) - height);
	if (x1 - 1 >= 0 && z1 + 1 < mapSize)
		diff += Math.abs(getHeight(Math.round(x1-1),Math.round(z1+1)) - height);
	return diff;
}
