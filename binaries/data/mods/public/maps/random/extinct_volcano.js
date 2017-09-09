RMS.LoadLibrary("rmgen");

const tHillDark = "cliff volcanic light";
const tHillMedium1 = "ocean_rock_a";
const tHillMedium2 = "ocean_rock_b";
const tHillVeryDark = ["cliff volcanic coarse", "cave_walls"];
const tRoad = "road1";
const tRoadWild = "road1";
const tForestFloor1 = tHillMedium1;
const tForestFloor2 = tHillMedium2;
const tGrassA = "cliff volcanic light";
const tGrassB = "ocean_rock_a";
const tGrass3 = "temp_grass_plants";
const tGrassPatchBlend = "temp_grass_long_b";
const tGrassPatch = ["temp_grass_d", "temp_grass_clovers"];
const tShoreBlend = "cliff volcanic light";
const tShore = "ocean_rock_a";
const tWater = "ocean_rock_b";

// Gaia entities
const oTree = "gaia/flora_tree_dead";
const oTree2 = "gaia/flora_tree_euro_beech";
const oTree3 = "gaia/flora_tree_oak";
const oTree4 = "gaia/flora_tree_oak_dead";
const oBush = "gaia/flora_bush_temperate";
const oFruitBush = "gaia/flora_bush_berry";
const oRabbit = "gaia/fauna_rabbit";
const oGoat = "gaia/fauna_goat";
const oBear = "gaia/fauna_bear";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";
const oTower = "other/palisades_rocks_fort";

// Decorative props
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_tempe_me.xml";
const aBushSmall = "actor|props/flora/bush_tempe_sm.xml";
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRain = "actor|particle/rain_shower.xml";

const pForestD = [
	tForestFloor1 + TERRAIN_SEPARATOR + oTree,
	tForestFloor2 + TERRAIN_SEPARATOR + oTree2,
	tForestFloor1
];

const pForestP = [
	tForestFloor1 + TERRAIN_SEPARATOR + oTree3,
	tForestFloor2 + TERRAIN_SEPARATOR + oTree4,
	tForestFloor1
];

InitMap();

var P_FOREST = 0.7;
var totalTrees = scaleByMapSize(1200, 3000);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize * mapSize;

var clPlayer = createTileClass();
var clHill = createTileClass();
var clFood = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clGrass = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clBaseResource = createTileClass();
var clBumps = createTileClass();
var clTower = createTileClass();
var clRain = createTileClass();

var ccMountainHeight = 25;

var playerIDs = sortAllPlayers();

// Place players
var startAngle = randFloat(0, 2 * PI);
for (let i = 0; i < numPlayers; ++i)
{
	let playerAngle = startAngle + i * 2 * PI / numPlayers;
	let playerX = 0.5 + 0.35 * Math.cos(playerAngle);
	let playerZ = 0.5 + 0.35 * Math.sin(playerAngle);

	let id = playerIDs[i];
	log("Creating base for player " + id + "...");
	let radius = scaleByMapSize(15, 25);

	let fx = fractionToTiles(playerX);
	let fz = fractionToTiles(playerZ);
	let ix = Math.round(fx);
	let iz = Math.round(fz);

	// This one consists of many bumps, creating an omnidirectional ramp
	createMountain(
		ccMountainHeight,
		Math.floor(scaleByMapSize(15, 15)),
		Math.floor(scaleByMapSize(15, 15)),
		Math.floor(scaleByMapSize(4, 10)),
		avoidClasses(),
		ix,
		iz,
		tHillDark,
		clPlayer,
		14);

	// Flatten the initial CC area
	let hillSize = PI * radius * radius;
	createArea(
		new ClumpPlacer(hillSize, 0.95, 0.6, 10, ix, iz),
		[
			new LayeredPainter([tHillVeryDark, tHillMedium1], [radius]),
			new SmoothElevationPainter(ELEVATION_SET, ccMountainHeight, radius),
			paintClass(clPlayer)
		],
		null);

	// Create the city patch
	let cityRadius = radius / 3;
	createArea(
		new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz),
		new LayeredPainter([tRoadWild, tRoad], [1]),
		null);

	placeCivDefaultEntities(fx, fz, id, { 'iberWall': 'towers' });

	// Create metal mine
	let mAngle = randFloat(0, 2 * PI);
	let mDist = 12;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
			true,
			clBaseResource,
			Math.round(fx + mDist * Math.cos(mAngle)),
			Math.round(fz + mDist * Math.sin(mAngle))),
		0);

	// Create stone mines
	mAngle += randFloat(PI/4, PI/3);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oStoneLarge, 1, 1, 0, 2)],
			true,
			clBaseResource,
			Math.round(fx + mDist * Math.cos(mAngle)),
			Math.round(fz + mDist * Math.sin(mAngle))),
		0);

	placeDefaultChicken(fx, fz, clBaseResource);

	// Create berry bushes
	mAngle += randFloat(PI/4, PI/2);
	let bbDist = 12;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oFruitBush, 5, 5, 0, 3)],
			true,
			clBaseResource,
			Math.round(fx + bbDist * Math.cos(mAngle)),
			Math.round(fz + bbDist * Math.sin(mAngle))),
		0);

	// Create starting trees
	let num = Math.floor(hillSize / 60);
	let tries = 20;
	for (let x = 0; x < tries; ++x)
	{
		let tAngle = randFloat(0, 2 * PI);
		let tDist = randFloat(10, 12);
		if (createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(oTree2, num, num, 0, 3)],
				false,
				clBaseResource,
				Math.round(fx + tDist * Math.cos(tAngle)),
				Math.round(fz + tDist * Math.sin(tAngle))),
			0,
			avoidClasses(clBaseResource, 3)))
		{
			break;
		}
	}
}
RMS.SetProgress(15);

createVolcano(0.5, 0.5, clHill, tHillVeryDark, undefined, false, ELEVATION_SET);
RMS.SetProgress(20);

log("Creating lakes...");
createAreas(
	new ChainPlacer(5, 6, Math.floor(scaleByMapSize(10, 14)), 0.1),
	[
		new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, -4, 3),
		paintClass(clWater)
	],
	avoidClasses(clPlayer, 0, clHill, 2, clWater, 12),
	Math.round(scaleByMapSize(6, 12)));
RMS.SetProgress(25);

createBumps(avoidClasses(clPlayer, 0, clHill, 0), scaleByMapSize(50, 300), 1, 10, 3, 0, scaleByMapSize(4, 10));
paintTileClassBasedOnHeight(10, 100, 0, clBumps);
RMS.SetProgress(30);

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tHillDark, tHillDark, tHillDark], [2, 2]),
		new SmoothElevationPainter(ELEVATION_SET, 18, 2),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 0, clHill, 15, clWater, 2, clBaseResource, 2),
	scaleByMapSize(2, 8) * numPlayers);
RMS.SetProgress(35);

log("Creating forests...");
var types = [
	[[tGrassB, tGrassA, pForestD], [tGrassB, pForestD]],
	[[tGrassB, tGrassA, pForestP], [tGrassB, pForestP]]
];
var size = numForest / (scaleByMapSize(4, 12) * numPlayers);
var num = Math.floor(size / types.length);
for (let i = 0; i < types.length; ++i)
	createAreas(
		new ClumpPlacer(numForest / num, 0.1, 0.1, 1),
		[
			new LayeredPainter(types[i], [2]),
			paintClass(clForest)
		],
		avoidClasses(
			clPlayer, 4,
			clForest, 10,
			clHill, 0,
			clWater, 2),
		num);
RMS.SetProgress(40);

log("Creating hill patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	for (let type of [[tHillMedium1, tHillDark], [tHillDark, tHillMedium2], [tHillMedium1, tHillMedium2]])
		createAreas(
			new ClumpPlacer(size, 0.3, 0.06, 0.5),
			[
				new LayeredPainter(type, [1]),
				paintClass(clGrass)
			],
			avoidClasses(
				clWater, 3,
				clForest, 0,
				clHill, 0,
				clBumps, 0,
				clPlayer, 0),
			scaleByMapSize(20, 80));
RMS.SetProgress(45);

log("Creating grass patches...");
createLayeredPatches(
	[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
	[tGrassPatchBlend, tGrassPatch],
	[1],
	avoidClasses(
		clWater, 1,
		clForest, 0,
		clHill, 0,
		clGrass, 5,
		clBumps, 0,
		clPlayer, 0),
	clDirt);
RMS.SetProgress(50);

log("Creating stone mines...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oStoneSmall, 0, 2, 0, 4),
			new SimpleObject(oStoneLarge, 1, 1, 0, 4)
		],
		true,
		clRock),
	0,
	[
		stayClasses(clBumps, 1),
		avoidClasses(
			clWater, 3,
			clForest, 1,
			clPlayer, 0,
			clRock, 15,
			clHill, 0)
	],
	100,
	100);
RMS.SetProgress(55);

log("Creating small stone quarries...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
	0,
	[
		stayClasses(clBumps, 1),
		avoidClasses(
			clWater, 3,
			clForest, 1,
			clPlayer, 0,
			clRock, 15,
			clHill, 0)
	],
	100,
	100);
RMS.SetProgress(60);

log("Creating metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[
		stayClasses(clBumps, 1),
		avoidClasses(
			clWater, 3,
			clForest, 1,
			clPlayer, 0,
			clMetal, 15,
			clRock, 10,
			clHill, 0)
	],
	100,
	100);
RMS.SetProgress(65);

log("Creating towers...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oTower, 1, 1, 0, 4)], true, clTower),
	0,
	[
		stayClasses(clBumps, 3),
		avoidClasses(
			clMetal, 5,
			clRock, 5,
			clHill, 0,
			clTower, 60,
			clPlayer, 10,
			clForest, 2)
	],
	500,
	1);
RMS.SetProgress(67);

createDecoration(
		[
			[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
			[
				new SimpleObject(aGrass, 2, 4, 0, 1.8),
				new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)
			],
			[
				new SimpleObject(aBushMedium, 1, 2, 0, 2),
				new SimpleObject(aBushSmall, 2, 4, 0, 2)
			]
		],
		[
			scaleByMapSize(15, 200),
			scaleByMapSize(15, 200),
			scaleByMapSize(15, 200)
		],
		[
			stayClasses(clGrass, 0),
			avoidClasses(
				clWater, 0,
				clForest, 0,
				clPlayer, 0,
				clHill, 0)
		]);
RMS.SetProgress(70);

createDecoration(
		[
			[
				new SimpleObject(aRockMedium, 1, 3, 0, 1)
			],
			[
				new SimpleObject(aRockLarge, 1, 2, 0, 1),
				new SimpleObject(aRockMedium, 1, 3, 0, 2)
			]
		],
		[
			scaleByMapSize(15, 250),
			scaleByMapSize(15, 150)
		],
		avoidClasses(
			clWater, 0,
			clForest, 0,
			clPlayer, 0,
			clHill, 0
		));
RMS.SetProgress(75);

createFood(
	[
		[new SimpleObject(oRabbit, 5, 7, 2, 4)],
		[new SimpleObject(oGoat, 3, 5, 2, 4)]
	],
	[
		scaleByMapSize(1, 6) * numPlayers,
		scaleByMapSize(3, 10) * numPlayers
	],
	[
		avoidClasses(
			clWater, 1,
			clForest, 0,
			clPlayer, 0,
			clHill, 1,
			clFood, 20)
	],
	clFood);
RMS.SetProgress(78);

createFood(
	[
		[new SimpleObject(oBear, 1, 1, 0, 2)]
	],
	[
		3 * numPlayers
	],
	[
		avoidClasses(
			clWater, 1,
			clForest, 0,
			clPlayer, 0,
			clHill, 1,
			clFood, 20
		),
		stayClasses(clForest, 2)
	],
	clFood);
RMS.SetProgress(81);

createFood(
	[
		[new SimpleObject(oFruitBush, 1, 2, 0, 4)]
	],
	[
		3 * numPlayers
	],
	[stayClasses(clGrass, 1), avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 1, clFood, 10)],
	clFood);
RMS.SetProgress(85);

log("Creating straggler trees and bushes...");
var types = [oTree, oTree2, oTree3, oTree4, oBush];
var num = Math.floor(numStragglers / types.length);
for (let type of types)
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[new SimpleObject(type, 1, 1, 0, 3)],
			true,
			clForest),
		0,
		[
			stayClasses(clGrass, 1),
			avoidClasses(
				clWater, 5,
				clForest, 1,
				clHill, 1,
				clPlayer, 0,
				clMetal, 1,
				clRock, 1)
		],
		num);
RMS.SetProgress(90);

log("Creating straggler bushes...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBush, 1, 3, 0, 3)],
		true,
		clForest
	),
	0,
	[
		stayClasses(clGrass, 3),
		avoidClasses(
			clWater, 1,
			clForest, 1,
			clPlayer, 0,
			clMetal, 1,
			clRock, 1)
	],
	numStragglers);
RMS.SetProgress(95);

log("Creating rain drops...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(aRain, 2, 2, 1, 4)],
		true,
		clRain),
	0,
	avoidClasses(clRain, 5),
	scaleByMapSize(80, 250));
RMS.SetProgress(95);

setSkySet("rain");
setWaterType("lake");
setWaterWaviness(2);
setWaterColor(0.1, 0.13, 0.15);
setWaterTint(0.058, 0.05, 0.035);
setWaterMurkiness(0.9);

setPPEffect("hdr");

ExportMap();
