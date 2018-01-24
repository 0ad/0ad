Engine.LoadLibrary("rmgen");
TILE_CENTERED_HEIGHT_MAP = true;

const tGrassSpecific = ["new_alpine_grass_d","new_alpine_grass_d", "new_alpine_grass_e"];
const tGrass = ["new_alpine_grass_d", "new_alpine_grass_b", "new_alpine_grass_e"];
const tGrassMidRange = ["new_alpine_grass_b", "alpine_grass_a"];
const tGrassHighRange = ["new_alpine_grass_a", "alpine_grass_a", "alpine_grass_rocky"];
const tHighRocks = ["alpine_cliff_b", "alpine_cliff_c","alpine_cliff_c", "alpine_grass_rocky"];
const tSnowedRocks = ["alpine_cliff_b", "alpine_cliff_snow"];
const tTopSnow = ["alpine_snow_rocky","alpine_snow_a"];
const tTopSnowOnly = ["alpine_snow_a"];
const tDirtyGrass = ["new_alpine_grass_d","alpine_grass_d","alpine_grass_c", "alpine_grass_b"];
const tLushGrass = ["new_alpine_grass_a","new_alpine_grass_d"];
const tMidRangeCliffs = ["alpine_cliff_b","alpine_cliff_c"];
const tHighRangeCliffs = ["alpine_mountainside","alpine_cliff_snow" ];
const tSand = ["beach_c", "beach_d"];
const tSandTransition = ["beach_scrub_50_"];
const tWater = ["sand_wet_a","sand_wet_b","sand_wet_b","sand_wet_b"];
const tGrassLandForest = "alpine_forrestfloor";
const tGrassLandForest2 = "alpine_grass_d";
const tForestTransition = ["new_alpine_grass_d", "new_alpine_grass_b","alpine_grass_d"];
const tRoad = "new_alpine_citytile";
const tRoadWild = "new_alpine_citytile";

const oBeech = "gaia/flora_tree_euro_beech";
const oPine = "gaia/flora_tree_aleppo_pine";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oRabbit = "gaia/fauna_rabbit";
const oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
const oStoneSmall = "gaia/geology_stone_alpine_a";
const oMetalLarge = "gaia/geology_metal_alpine_slabs";

const aGrass = "actor|props/flora/grass_soft_small_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForestLand = [tGrassLandForest + TERRAIN_SEPARATOR + oPine,tGrassLandForest + TERRAIN_SEPARATOR + oBeech,
				   tGrassLandForest2 + TERRAIN_SEPARATOR + oPine,tGrassLandForest2 + TERRAIN_SEPARATOR + oBeech,
				   tGrassLandForest,tGrassLandForest2,tGrassLandForest2,tGrassLandForest2];
const pForestLandLight = [tGrassLandForest + TERRAIN_SEPARATOR + oPine,tGrassLandForest + TERRAIN_SEPARATOR + oBeech,
				   tGrassLandForest2 + TERRAIN_SEPARATOR + oPine,tGrassLandForest2 + TERRAIN_SEPARATOR + oBeech,
				   tGrassLandForest,tGrassLandForest2,tForestTransition,tGrassLandForest2,
					tGrassLandForest,tForestTransition,tGrassLandForest2,tForestTransition,
						tGrassLandForest2,tGrassLandForest2,tGrassLandForest2,tGrassLandForest2];
const pForestLandVeryLight = [ tGrassLandForest2 + TERRAIN_SEPARATOR + oPine,tGrassLandForest2 + TERRAIN_SEPARATOR + oBeech,
						tForestTransition,tGrassLandForest2,tForestTransition,tForestTransition,tForestTransition,
						tGrassLandForest,tForestTransition,tGrassLandForest2,tForestTransition,
						tGrassLandForest2,tGrassLandForest2,tGrassLandForest2,tGrassLandForest2];

const heightInit = -100;
const heightOcean = -22;
const heightBase = -6;
const heightWaterLevel = 8;
const heightPyreneans = 15;
const heightGrass = 6;
const heightGrassMidRange = 18;
const heightGrassHighRange = 30;
const heightPassage = scaleByMapSize(25, 40);
const heightHighRocks = heightPassage + 5;
const heightSnowedRocks = heightHighRocks + 10;
const heightMountain = heightHighRocks + 20;

const heightOffsetHill = 7;
const heightOffsetHillRandom = 2;

InitMap(heightInit, tGrass);

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();

var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clPass = createTileClass();
var clPyrenneans = createTileClass();
var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();

var startAngle = randomAngle();
var oceanAngle = startAngle + randFloat(-1, 1) * Math.PI / 12;

var mountainLength = fractionToTiles(0.68);
var mountainWidth = scaleByMapSize(15, 55);

var mountainPeaks = 100 * scaleByMapSize(1, 10);
var mountainOffset = randFloat(-1, 1) * scaleByMapSize(1, 12);

var passageLength = scaleByMapSize(8, 50);

var terrainPerHeight = [
	{
		"maxHeight": heightGrass,
		"steepness": 5,
		"terrainGround": tGrass,
		"terrainSteep": tMidRangeCliffs
	},
	{
		"maxHeight": heightGrassMidRange,
		"steepness": 8,
		"terrainGround": tGrassMidRange,
		"terrainSteep": tMidRangeCliffs
	},
	{
		"maxHeight": heightGrassHighRange,
		"steepness": 8,
		"terrainGround": tGrassHighRange,
		"terrainSteep": tMidRangeCliffs
	},
	{
		"maxHeight": heightHighRocks,
		"steepness": 8,
		"terrainGround": tHighRocks,
		"terrainSteep": tHighRangeCliffs
	},
	{
		"maxHeight": heightSnowedRocks,
		"steepness": 7,
		"terrainGround": tSnowedRocks,
		"terrainSteep": tHighRangeCliffs
	},
	{
		"maxHeight": Infinity,
		"steepness": 6,
		"terrainGround": tTopSnowOnly,
		"terrainSteep": tTopSnow
	}
];

log("Creating initial sinusoidal noise...");
var baseHeights = [];
for (var ix = 0; ix < mapSize; ix++)
{
	baseHeights.push([]);
	for (var iz = 0; iz < mapSize; iz++)
	{
		let position = new Vector2D(ix, iz);
		if (g_Map.inMapBounds(position))
		{
			let height = heightBase + randFloat(-1, 1) + scaleByMapSize(1, 3) * (Math.cos(ix / scaleByMapSize(5, 30)) + Math.sin(iz / scaleByMapSize(5, 30)));
			g_Map.setHeight(position, height);
			baseHeights[ix].push(height);
		}
		else
			baseHeights[ix].push(heightInit);
	}
}

placePlayerBases({
	"PlayerPlacement": [primeSortAllPlayers(), ...playerPlacementCustomAngle(
		fractionToTiles(0.35),
		mapCenter,
		i => oceanAngle + Math.PI * (i % 2 ? 1 : -1) * ((1/2 + 1/3 * (2/numPlayers * (i + 1 - i % 2) - 1))))],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
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
		"template": oPine
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(30);

log("Creating the pyreneans...");
var mountainVec = new Vector2D(mountainLength, 0).rotate(-startAngle);
var mountainStart = Vector2D.sub(mapCenter, Vector2D.div(mountainVec, 2));
var mountainDirection = mountainVec.clone().normalize();
createPyreneans();
paintTileClassBasedOnHeight(heightPyreneans, Infinity, Elevation_ExcludeMin_ExcludeMax, clPyrenneans);
Engine.SetProgress(40);

/**
 * Generates the mountain peak noise.
 *
 * @param {number} x - between 0 and 1
 * @returns {number} between 0 and 1
 */
function sigmoid(x, peakPosition)
{
	return 1 / (1 + Math.exp(x)) *
		// If we're too far from the border, we flatten
		(0.2 - Math.max(0, Math.abs(0.5 - peakPosition) - 0.3)) * 5;
}

function createPyreneans()
{
	for (let peak = 0; peak < mountainPeaks; ++peak)
	{
		let peakPosition = peak / mountainPeaks;
		let peakHeight = randFloat(0, 10);

		for (let distance = 0; distance < mountainWidth; distance += 1/3)
		{
			let rest = 2 * (1 - distance / mountainWidth);

			let sigmoidX =
				- 1 * (rest - 1.9) +
				- 4 *
					(rest - randFloat(0.9, 1.1)) *
					(rest - randFloat(0.9, 1.1)) *
					(rest - randFloat(0.9, 1.1));

			for (let direction of [-1, 1])
			{
				let pos = Vector2D.sum([
					Vector2D.add(mountainStart, Vector2D.mult(mountainDirection, peakPosition * mountainLength)),
					new Vector2D(mountainOffset, 0).rotate(-peakPosition * Math.PI * 4),
					new Vector2D(distance, 0).rotate(-startAngle - direction * Math.PI / 2)
				]).round();

				g_Map.setHeight(pos, baseHeights[pos.x][pos.y] + (heightMountain + peakHeight + randFloat(-9, 9)) * sigmoid(sigmoidX, peakPosition));
			}
		}
	}
}

log("Smoothing pyreneans...");
for (let ix = 1; ix < mapSize - 1; ++ix)
	for (let iz = 1; iz < mapSize - 1; ++iz)
	{
		let position = new Vector2D(ix, iz);
		if (g_Map.validHeight(position) && getTileClass(clPyrenneans).countMembersInRadius(ix, iz, 1))
		{
			let height = g_Map.getHeight(position);
			let index = 1 / (1 + Math.max(0, height / 7));
			g_Map.setHeight(position, height * (1 - index) + g_Map.getAverageHeight(position) * index);
		}
	}
Engine.SetProgress(48);

log("Creating passages...");
var passageLocation = 0.35;
var passageVec = mountainDirection.perpendicular().mult(passageLength);

for (let passLoc of [passageLocation, 1 - passageLocation])
	for (let direction of [1, -1])
	{
		let passageStart = Vector2D.add(mountainStart, Vector2D.mult(mountainVec, passLoc));
		let passageEnd = Vector2D.add(passageStart, Vector2D.mult(passageVec, direction));

		createPassage({
			"start": passageStart,
			"end": passageEnd,
			"startHeight": heightPassage,
			"startWidth": 7,
			"endWidth": 7,
			"smoothWidth": 2,
			"tileClass": clPass
		});
	}
Engine.SetProgress(50);

log("Smoothing the mountains...");
for (let ix = 1; ix < mapSize - 1; ++ix)
	for (let iz = 1; iz < mapSize - 1; ++iz)
	{
		let position = new Vector2D(ix, iz);
		if (g_Map.inMapBounds(position) && getTileClass(clPyrenneans).countMembersInRadius(ix, iz, 1))
		{
			let heightNeighbor = g_Map.getAverageHeight(position);
			let index = 1 / (1 + Math.max(0, (g_Map.getHeight(position) - 10) / 7));
			g_Map.setHeight(position, g_Map.getHeight(position) * (1 - index) + heightNeighbor * index);
		}
	}

log("Creating oceans...");
for (let ocean of distributePointsOnCircle(2, oceanAngle, fractionToTiles(0.48), mapCenter)[0])
	createArea(
		new ClumpPlacer(diskArea(fractionToTiles(0.18)), 0.9, 0.05, 10, ocean),
		[
			new ElevationPainter(heightOcean),
			paintClass(clWater)
		]);

log("Smoothing around the water...");
var smoothDist = 5;
for (let ix = 1; ix < mapSize - 1; ++ix)
	for (let iz = 1; iz < mapSize - 1; ++iz)
	{
		let position = new Vector2D(ix, iz);
		if (!g_Map.inMapBounds(position) || !getTileClass(clWater).countMembersInRadius(ix, iz, smoothDist))
			continue;
		let averageHeight = 0;
		let todivide = 0;
		for (let xx = -smoothDist; xx <= smoothDist; ++xx)
			for (let yy = -smoothDist; yy <= smoothDist; ++yy)
			{
				let smoothPos = Vector2D.add(position, new Vector2D(xx, yy));
				if (g_Map.inMapBounds(smoothPos) && (xx != 0 || yy != 0))
				{
					averageHeight += g_Map.getHeight(smoothPos) / (Math.abs(xx) + Math.abs(yy));
					todivide += 1 / (Math.abs(xx) + Math.abs(yy));
				}
			}
		g_Map.setHeight(position, (averageHeight + 2 * g_Map.getHeight(position)) / (todivide + 2));
	}
Engine.SetProgress(55);

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(60, 120), 0.3, 0.06, 5),
	[
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetHill, 4, heightOffsetHillRandom),
		new TerrainPainter(tGrassSpecific),
		paintClass(clHill)
	],
	avoidClasses(clWater, 5, clPlayer, 20, clBaseResource, 6, clPyrenneans, 2), scaleByMapSize(5, 35));

log("Creating forests...");
var types = [[tForestTransition, pForestLandVeryLight, pForestLandLight, pForestLand]];
var size = scaleByMapSize(40, 115) * Math.PI;
var num = Math.floor(scaleByMapSize(8,40) / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(size, 0.2, 0.1, 1),
		[
			new LayeredPainter(type, [scaleByMapSize(1, 2), scaleByMapSize(3, 6), scaleByMapSize(3, 6)]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 20, clPyrenneans,0, clForest, 7, clWater, 2),
		num);
Engine.SetProgress(60);

log("Creating lone trees...");
var num = scaleByMapSize(80,400);

var group = new SimpleGroup([new SimpleObject(oPine, 1,2, 1,3),new SimpleObject(oBeech, 1,2, 1,3)], true, clForest);
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 1, clPlayer, 8,clPyrenneans, 1), num, 20 );

log("Painting the map...");
for (let x = 0; x < mapSize; ++x)
	for (let z = 0; z < mapSize; ++z)
	{
		let position = new Vector2D(x, z);
		let height = g_Map.getHeight(position);
		let heightDiff = g_Map.getSlope(position);

		if (getTileClass(clPyrenneans).countMembersInRadius(x, z, 2))
		{
			let layer = terrainPerHeight.find(layer => height < layer.maxHeight);
			createTerrain(heightDiff > layer.steepness ? layer.terrainSteep : layer.terrainGround).place(position);
		}

		let terrainShore = getShoreTerrain(height, heightDiff, x, z);
		if (terrainShore)
			createTerrain(terrainShore).place(position);
	}

function getShoreTerrain(height, heightDiff, x, z)
{
	if (height <= -14)
		return tWater;

	if (height <= -2 && getTileClass(clWater).countMembersInRadius(x, z, 2))
		return heightDiff < 2.5 ? tSand : tMidRangeCliffs;

	if (height <= 0 && getTileClass(clWater).countMembersInRadius(x, z, 3))
		return heightDiff < 2.5 ? tSandTransition : tMidRangeCliffs;

	return undefined;
}

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 20), scaleByMapSize(5, 40), scaleByMapSize(8, 60)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new TerrainPainter(tDirtyGrass),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 3, clForest, 0, clPyrenneans,5, clHill, 0, clDirt, 5, clPlayer, 6),
		scaleByMapSize(15, 45));

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tLushGrass),
		avoidClasses(clWater, 3, clForest, 0, clPyrenneans,5, clHill, 0, clDirt, 5, clPlayer, 6),
		scaleByMapSize(15, 45));

Engine.SetProgress(70);

// making more in dirt areas so as to appear different
log("Creating small grass tufts...");
var group = new SimpleGroup( [new SimpleObject(aGrassShort, 1,2, 0,1, -Math.PI / 8, Math.PI / 8)] );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 2, clHill, 2, clPlayer, 5, clDirt, 0, clPyrenneans,2), scaleByMapSize(13, 200) );
createObjectGroupsDeprecated(group, 0, stayClasses(clDirt,1), scaleByMapSize(13, 200),10);

log("Creating large grass tufts...");
group = new SimpleGroup( [new SimpleObject(aGrass, 2,4, 0,1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)] );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clHill, 2, clPlayer, 5, clDirt, 1, clForest, 0, clPyrenneans,2), scaleByMapSize(13, 200) );
createObjectGroupsDeprecated(group, 0, stayClasses(clDirt,1), scaleByMapSize(13, 200),10);
Engine.SetProgress(75);

log("Creating bushes...");
group = new SimpleGroup( [new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)] );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 2, clPlayer, 1, clPyrenneans, 1), scaleByMapSize(13, 200), 50 );

Engine.SetProgress(80);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 8, clPyrenneans, 1),  scaleByMapSize(4,16), 100 );

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 8, clPyrenneans, 1),  scaleByMapSize(4,16), 100 );

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 8, clRock, 5, clPyrenneans, 1), scaleByMapSize(4,16), 100  );

Engine.SetProgress(85);

log("Creating small decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRockMedium, 1,3, 0,1)], true );
createObjectGroupsDeprecated( group, 0, avoidClasses(clWater, 0, clForest, 0, clPlayer, 0), scaleByMapSize(16, 262), 50 );

log("Creating large decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)], true );
createObjectGroupsDeprecated( group, 0,  avoidClasses(clWater, 0, clForest, 0, clPlayer, 0), scaleByMapSize(8, 131), 50 );

Engine.SetProgress(90);

log("Creating deer...");
group = new SimpleGroup( [new SimpleObject(oDeer, 5,7, 0,4)], true, clFood );
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clPyrenneans, 1, clFood, 15),  3 * numPlayers, 50 );

log("Creating rabbit...");
group = new SimpleGroup( [new SimpleObject(oRabbit, 2,3, 0,2)], true, clFood );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clPyrenneans, 1, clFood,15), 3 * numPlayers, 50 );

log("Creating berry bush...");
group = new SimpleGroup( [new SimpleObject(oBerryBush, 5,7, 0,4)],true, clFood );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clPyrenneans, 1, clFood, 10), randIntInclusive(1, 4) * numPlayers + 2, 50);

log("Creating fish...");
group = new SimpleGroup( [new SimpleObject(oFish, 2,3, 0,2)], true, clFood );
createObjectGroupsDeprecated(group, 0, [avoidClasses(clFood, 15), stayClasses(clWater, 6)], 20 * numPlayers, 60 );

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clPyrenneans, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSunElevation(Math.PI * randFloat(1/5, 1/3));
setSunRotation(randomAngle());

setSkySet("cumulus");
setSunColor(0.73,0.73,0.65);
setTerrainAmbientColor(0.45,0.45,0.50);
setUnitsAmbientColor(0.4,0.4,0.4);
setWaterColor(0.263, 0.353, 0.616);
setWaterTint(0.104, 0.172, 0.563);
setWaterWaviness(5.0);
setWaterType("ocean");
setWaterMurkiness(0.83);
setWaterHeight(heightWaterLevel);

ExportMap();
