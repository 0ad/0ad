RMS.LoadLibrary("rmgen");

const tGrass = ["tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_plants", "tropic_plants", "tropic_plants_b"];
const tGrassA = "tropic_plants_c";
const tGrassB = "tropic_plants_c";
const tGrassC = "tropic_grass_c";
const tForestFloor = "tropic_grass_plants";
const tCliff = ["tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a_plants"];
const tPlants = "tropic_plants";
const tRoad = "tropic_citytile_a";
const tRoadWild = "tropic_citytile_plants";
const tShoreBlend = "tropic_beach_dry_plants";
const tShore = "tropic_beach_dry";
const tWater = "tropic_beach_wet";

const oTree = "gaia/flora_tree_toona";
const oPalm1 = "gaia/flora_tree_palm_tropic";
const oPalm2 = "gaia/flora_tree_palm_tropical";
const oStoneLarge = "gaia/geology_stonemine_tropic_quarry";
const oStoneSmall = "gaia/geology_stone_tropic_a";
const oMetalLarge = "gaia/geology_metal_tropic_slabs";
const oFish = "gaia/fauna_fish";
const oDeer = "gaia/fauna_deer";
const oTiger = "gaia/fauna_tiger";
const oBoar = "gaia/fauna_boar";
const oPeacock = "gaia/fauna_peacock";
const oBush = "gaia/flora_bush_berry";
const oSpearman = "units/maur_infantry_spearman_b";
const oArcher = "units/maur_infantry_archer_b";
const oArcherElephant = "units/maur_elephant_archer_b";

const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBush1 = "actor|props/flora/plant_tropic_a.xml";
const aBush2 = "actor|props/flora/plant_lg.xml";
const aBush3 = "actor|props/flora/plant_tropic_large.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oTree, tForestFloor];
const pForestP1 = [tForestFloor + TERRAIN_SEPARATOR + oPalm1, tForestFloor];
const pForestP2 = [tForestFloor + TERRAIN_SEPARATOR + oPalm2, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clPlayerTerritory = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clGaia = createTileClass();
var clStrip = [];

log("Creating terrain...");
for (let ix = 0; ix < mapSize; ++ix)
	for (let iz = 0; iz < mapSize; ++iz)
		setHeight(ix, iz, -8);
var connectPlayers = randBool();

// Map layout
var stripWidthsLeft = connectPlayers ?
	[[0.03, 0.09], [0.14, 0.25], [0.36, 0.46]] : 
	[[0, 0.06], [0.12, 0.23], [0.33, 0.43]];

var playerPosLeft = (stripWidthsLeft[2][0] + stripWidthsLeft[2][1]) / 2;

// Mirror
var stripWidthsRight = clone(stripWidthsLeft);
stripWidthsRight.reverse();
stripWidthsRight = stripWidthsRight.map(strip => [1 - strip[1], 1 - strip[0]]);

var stripWidths = stripWidthsLeft.concat(stripWidthsRight);
var playerPos = [playerPosLeft, 1 - playerPosLeft];

for (let i = 0; i < stripWidths.length; ++i)
{
	clStrip[i] = createTileClass();

	let isPlayerStrip = i == 2 || i == 3;
	for (let j = 0; j < scaleByMapSize(20, 100); ++j)
		createArea(
			new ChainPlacer(
				1,
				Math.floor(scaleByMapSize(3, connectPlayers && isPlayerStrip ? 8 : 7)),
				Math.floor(scaleByMapSize(30, 60)),
				1,
				Math.floor(randFloat(...stripWidths[i]) * mapSize),
				Math.floor(randFloat(0, 1) * mapSize)),
			[
				new LayeredPainter([tGrass, tGrass], [2]),
				new SmoothElevationPainter(ELEVATION_SET, 3, 3),
				paintClass(clStrip[i])
			],
			null);
}
RMS.SetProgress(20);

// Randomize player order
var playerIDs = [];
for (let i = 0; i < numPlayers; ++i)
	playerIDs.push(i+1);
playerIDs = sortPlayers(playerIDs);

// Either left vs right or top vs bottom
var leftVSRight = randBool();

for (let i = 0; i < numPlayers; ++i)
{
	let playerX;
	let playerZ;

	if (leftVSRight)
	{
		let left = i < numPlayers / 2;
		playerX = playerPos[left ? 0 : 1];
		playerZ = 2 * (left ? i + 1 : numPlayers - i - 0.5) / (numPlayers + 1 + numPlayers % 2);
	}
	else
	{
		playerX = playerPos[i % 2];
		playerZ = (i + 1) / (numPlayers + 1);
	}

	log("Creating base for player " + playerIDs[i] + "...");
	let radius = scaleByMapSize(12, 20);

	let fx = fractionToTiles(playerX);
	let fz = fractionToTiles(playerZ);
	let ix = Math.round(fx);
	let iz = Math.round(fz);

	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);

	// Create the main island
	createArea(
		new ChainPlacer(1, 6, 40, 1, ix, iz, 0, [Math.floor(radius)]),
		[
			new LayeredPainter([tGrass, tGrass, tGrass], [1, 4]),
			new SmoothElevationPainter(ELEVATION_SET, 3, 4),
			paintClass(clPlayerTerritory)
		], null);

	// Create the city patch
	let cityRadius = radius / 3;
	createArea(
		new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz),
		new LayeredPainter([tRoadWild, tRoad], [1]),
		null);

	placeCivDefaultEntities(fx, fz, playerIDs[i],  { 'iberWall': 'towers' });

	placeDefaultChicken(fx, fz, clBaseResource, undefined, oPeacock);

	// Create berry bushes
	let angle = randFloat(0, 2 * PI);
	let dist = 12;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oBush, 5, 5, 0, 3)],
			true,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))),
		0);

	// Create metal mine
	angle += randFloat(PI/8, PI/4);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
			true,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))),
		0);

	// Create stone mines
	angle += randFloat(PI/8, PI/4);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oStoneLarge, 1, 1, 0, 2)],
			true,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))),
		0);

	// Create starting trees
	let num = 40;
	let tAngle = randFloat(-PI/3, 4*PI/3);
	let tDist = randFloat(12, 13);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oTree, num, num, 0, 3)],
			false,
			clBaseResource,
			Math.round(fx + tDist * Math.cos(tAngle)),
			Math.round(fz + tDist * Math.sin(tAngle))),
		0,
		avoidClasses(clBaseResource, 2));
}
RMS.SetProgress(35);

log("Creating gaia...");
for (let i = 0; i < 2; ++i)
	for (let j = 0; j < scaleByMapSize(1, 8); ++j)
		createObjectGroupsDeprecated(
			new SimpleGroup(
				[
					new SimpleObject(oSpearman, 8, 12, 2, 3),
					new SimpleObject(oArcher, 8, 12, 2, 3),
					new SimpleObject(oArcherElephant, 2, 3, 4, 5)
				],
				true,
				clGaia),
			0,
			[
				avoidClasses(
					clWater, 2,
					clForest, 1,
					clPlayerTerritory, 0,
					clHill, 1,
					clGaia, 15),
				stayClasses(clStrip[i == 0 ? 0 : stripWidths.length - 1], 1)
			],
			scaleByMapSize(5, 10),
			50);

paintTerrainBasedOnHeight(-10, 0, 1, tWater);
paintTileClassBasedOnHeight(-10, 0, 1, clWater);
paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTerrainBasedOnHeight(0, 1, 1, tShore);
RMS.SetProgress(40);

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new LayeredPainter([tCliff, tGrass], [3]),
		new SmoothElevationPainter(ELEVATION_SET, 25, 3),
		paintClass(clHill)
	],
	[
		avoidClasses(
			clPlayerTerritory, 0,
			clHill, 5,
			clGaia, 1,
			clWater, 2)
	],
	scaleByMapSize(1, 5));

log("Creating bumps...");
createBumps(avoidClasses(clPlayer, 8, clWater, 2), scaleByMapSize(20, 150), 2, 8, 4, 1, 4);
RMS.SetProgress(50);

log("Creating forests...");
var P_FOREST = 0.7;
var totalTrees = scaleByMapSize(1000, 4000);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

var types = [
	[[tGrass, tGrass, tGrass, tGrass, pForestD], [tGrass, tGrass, tGrass, pForestD]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP1], [tGrass, tGrass, tGrass, pForestP1]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP2], [tGrass, tGrass, tGrass, pForestP2]]
];
var size = numForest / (scaleByMapSize(3, 6) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(
			1,
			Math.floor(scaleByMapSize(3, 5)),
			numForest / (num * Math.floor(scaleByMapSize(2, 4))),
			0.5),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(
			clPlayer, 12,
			clForest, 6,
			clHill, 0,
			clGaia, 1,
			clWater, 2),
		num);

log("Creating straggler trees...");
var types = [oTree, oPalm1, oPalm2];
var num = Math.floor(numStragglers / types.length);
for (let type of types)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(type, 1, 1, 0, 3)], true, clForest),
		0,
		avoidClasses(
			clWater, 5,
			clForest, 1,
			clHill, 1,
			clPlayer, 8,
			clBaseResource, 4,
			clGaia, 1,
			clMetal, 4,
			clRock, 4),
		num);
RMS.SetProgress(60);

log("Creating grass patches...");
var sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
for (let i = 0; i < sizes.length; ++i)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), sizes[i], 0.5),
		[
			new LayeredPainter([tGrassC, tGrassA, tGrassB], [2, 1]),
			paintClass(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clGaia, 1,
			clPlayerTerritory, 0,
			clDirt, 16),
		scaleByMapSize(20, 80));

log("Creating dirt patches...");
var sizes = [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)];
for (let i = 0; i < sizes.length; ++i)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), sizes[i], 0.5),
		[
			new LayeredPainter([tPlants, tPlants], [1]),
			paintClass(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clGaia, 1,
			clPlayerTerritory, 0,
			clDirt, 16),
		scaleByMapSize(20, 80));

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
	avoidClasses(
		clWater, 3,
		clForest, 1,
		clPlayerTerritory, 0,
		clGaia, 1,
		clRock, 10,
		clHill, 1),
		9 * scaleByMapSize(1, 4),
		100);

log("Creating small stone mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 1,
		clPlayerTerritory, 0,
		clGaia, 1,
		clRock, 10,
		clHill, 1),
		9 * scaleByMapSize(1, 4),
	100);

log("Creating metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 1,
		clPlayerTerritory, 0,
		clGaia, 1,
		clMetal, 10,
		clRock, 5,
		clHill, 1),
	9 * scaleByMapSize(1, 4),
	100);

log("Creating small decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aRockMedium, 1, 3, 0, 1)], true),
	0,
	avoidClasses(
		clWater, 2,
		clForest, 1,
		clGaia, 1,
		clPlayer, 8,
		clBaseResource, 4,
		clHill, 0),
	3 * scaleByMapSize(16, 262),
	50);

log("Creating large decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup([
			new SimpleObject(aRockLarge, 1, 2, 0, 1),
			new SimpleObject(aRockMedium, 1, 3, 0, 2)
		],
		true),
	0,
	avoidClasses(
		clWater, 2,
		clForest, 1,
		clGaia, 1,
		clPlayer, 8,
		clBaseResource, 4,
		clHill, 0),
	3 * scaleByMapSize(8, 131),
	50);

log("Creating small grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aBush1, 1, 2, 0, 1, -PI/8, PI/8)]),
	0,
	avoidClasses(
		clWater, 4,
		clHill, 2,
		clPlayer, 8,
		clGaia, 1,
		clBaseResource, 4,
		clDirt, 0),
	8 * scaleByMapSize(13, 200));
RMS.SetProgress(70);

log("Creating large grass tufts...");
	createObjectGroupsDeprecated(
		new SimpleGroup([
			new SimpleObject(aBush2, 2, 4, 0, 1.8, -PI/8, PI/8),
			new SimpleObject(aBush1, 3, 6, 1.2, 2.5, -PI/8, PI/8)
		]),
		0,
		avoidClasses(
			clWater, 4,
			clHill, 2,
			clGaia, 1,
			clPlayer, 8,
			clBaseResource, 4,
			clDirt, 1,
			clForest, 0),
		8 * scaleByMapSize(13, 200));
RMS.SetProgress(85);

log("Creating bushes...");
	createObjectGroupsDeprecated(
		new SimpleGroup([
			new SimpleObject(aBush3, 1, 2, 0, 2),
			new SimpleObject(aBush2, 2, 4, 0, 2)
		]), 0,
		avoidClasses(
			clWater, 4,
			clHill, 1,
			clPlayerTerritory, 0,
			clGaia, 1,
			clDirt, 1),
		8 * scaleByMapSize(13, 200), 50);

log("Creating deer...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oDeer, 5, 7, 0, 4)], true, clFood),
		0,
		avoidClasses(
			clWater, 4,
			clForest, 0,
			clPlayerTerritory, 0,
			clGaia, 1,
			clHill, 1,
			clFood, 20),
		3 * numPlayers,
		50);

log("Creating boar...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oBoar, 2, 4, 0, 4)], true, clFood),
		0,
		avoidClasses(
			clWater, 4,
			clForest, 0,
			clPlayerTerritory, 0,
			clGaia, 1,
			clHill, 1,
			clFood, 20),
		3 * numPlayers,
		50);

log("Creating tigers...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oTiger, 1, 1, 0, 4)], true, clFood),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 0,
		clPlayerTerritory, 0,
		clGaia, 1,
		clHill, 1,
		clFood, 20),
	3 * numPlayers,
	50);
RMS.SetProgress(95);

log("Creating berry bush...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oBush, 5, 7, 0, 4)], true, clFood),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 0,
		clPlayerTerritory, 0,
		clGaia, 1,
		clHill, 1,
		clFood, 10),
		randIntInclusive(1, 4) * numPlayers + 2,
		50);

log("Creating fish...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oFish, 2, 3, 0, 2)], true, clFood),
	0,
	[avoidClasses(clFood, 15), stayClasses(clWater, 4)],
	200,
	100);

setSunColor(0.6, 0.6, 0.6);
setSunElevation(PI/ 3);

setWaterColor(0.424, 0.534, 0.639);
setWaterTint(0.369, 0.765, 0.745);
setWaterWaviness(1.0);
setWaterType("default");
setWaterMurkiness(0.35);

setFogFactor(0.03);
setFogThickness(0.2);

setPPEffect("hdr");
setPPContrast(0.7);
setPPSaturation(0.65);
setPPBloom(0.6);

setSkySet("stratus");
ExportMap();
