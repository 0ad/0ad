RMS.LoadLibrary("rmgen");

const tGrassA = "savanna_shrubs_a_wetseason";
const tGrassB = "savanna_shrubs_a";
const tCliff = "savanna_cliff_a";
const tHill = "savanna_grass_a_wetseason";
const tMud = "savanna_mud_a";
const tShoreBlend = "savanna_grass_b_wetseason";
const tShore = "savanna_riparian_wet";
const tWater = "savanna_mud_a";
const tCityTile = "savanna_tile_a";

const oBush = "gaia/flora_bush_temperate";
const oBaobab = "gaia/flora_tree_baobab";
const oToona = "gaia/flora_tree_toona";
const oBerryBush = "gaia/flora_bush_berry";
const oGazelle = "gaia/fauna_gazelle";
const oZebra = "gaia/fauna_zebra";
const oWildebeest = "gaia/fauna_wildebeest";
const oLion = "gaia/fauna_lion";
const oRhino = "gaia/fauna_rhino";
const oCrocodile = "gaia/fauna_crocodile";
const oElephant = "gaia/fauna_elephant_north_african";
const oElephantInfant = "gaia/fauna_elephant_african_infant";
const oLioness = "gaia/fauna_lioness";
const oRabbit = "gaia/fauna_rabbit";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

const aGrass = "actor|props/flora/grass_field_lush_tall.xml";
const aGrass2 = "actor|props/flora/grass_tropic_field_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aReeds2 = "actor|props/flora/reeds_pond_lush_b.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aBushMedium = "actor|props/flora/bush_tropic_b.xml";
const aBushSmall = "actor|props/flora/bush_tropic_a.xml";
const aShrub = "actor|props/flora/shrub_tropic_plant_flower.xml";
const aFlower = "actor|props/flora/flower_bright.xml";
const aPalm = "actor|props/flora/shrub_fanpalm.xml";

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clForest = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

for (let i = 0; i < numPlayers; ++i)
{
	let id = playerIDs[i];
	log("Creating base for player " + id + "...");
	let radius = scaleByMapSize(15, 25);

	// Get the x and z in tiles
	let fx = fractionToTiles(playerX[i]);
	let fz = fractionToTiles(playerZ[i]);
	let ix = Math.round(fx);
	let iz = Math.round(fz);

	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// Create the city patch
	let cityRadius = radius / 3;
	createArea(
		new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz),
		new LayeredPainter([tCityTile, tCityTile], [1]),
		null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// Create berry bushes
	let bbAngle = randFloat(0, 2 * PI);
	let bbDist = 12;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oBerryBush, 5, 5, 0, 3)],
			true,
			clBaseResource,
			Math.round(fx + bbDist * Math.cos(bbAngle)),
			Math.round(fz + bbDist * Math.sin(bbAngle))),
		0);

	// Create metal mine
	let mAngle = bbAngle;
	while (abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, 2 * PI);

	let mDist = radius - 4;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
			true,
			clBaseResource,
			Math.round(fx + mDist * Math.cos(mAngle)),
			Math.round(fz + mDist * Math.sin(mAngle))),
		0);

	// Create stone mine
	mAngle += randFloat(PI/8, PI/4);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oStoneLarge, 1, 1, 0, 2)],
			true,
			clBaseResource,
			Math.round(fx + mDist * Math.cos(mAngle)),
			Math.round(fz + mDist * Math.sin(mAngle))),
		0);

	// Create starting trees
	let hillSize = PI * radius * radius;
	let num = Math.floor(hillSize / 100);
	let tAngle = randFloat(-PI/3, 4 * PI/3);
	let tDist = 12;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oBaobab, num, num, 0, 5)],
			false,
			clBaseResource,
			Math.round(fx + tDist * Math.cos(tAngle)),
			Math.round(fz + tDist * Math.sin(tAngle))),
		0,
		avoidClasses(clBaseResource, 2));

	// Create grass tufts
	num = hillSize / 250;
	for (let j = 0; j < num; ++j)
	{
		let gAngle = randFloat(0, 2 * PI);
		let gDist = radius - randIntInclusive(5, 11);
		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(aGrassShort, 2, 5, 0, 1, -PI/8, PI/8)],
				false,
				clBaseResource,
				Math.round(fx + gDist * Math.cos(gAngle)),
				Math.round(fz + gDist * Math.sin(gAngle))),
			0);
	}
}
RMS.SetProgress(15);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.6, 0.1, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800));

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, 15, 2),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 0),
	scaleByMapSize(1, 4) * numPlayers * 3);

log("Creating marshes...");
for (let i = 0; i < 2; ++i)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(6, 12)), Math.floor(scaleByMapSize(15, 60)), 0.8),
		[
			new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
			new SmoothElevationPainter(ELEVATION_SET, -2, 3),
			paintClass(clWater)
		],
		avoidClasses(clPlayer, 25, clWater, Math.round(scaleByMapSize(7, 16) * randFloat(0.8, 1.35))),
		scaleByMapSize(4, 20));

log("Creating reeds...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aReeds, 20, 40, 0, 4),
			new SimpleObject(aReeds2, 20, 40, 0, 4),
			new SimpleObject(aLillies, 10, 30, 0, 4)
		],
		true),
	0,
	stayClasses(clWater, 1),
	scaleByMapSize(400, 1000),
	100);
RMS.SetProgress(40);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 1, 2),
	stayClasses(clWater, 2),
	scaleByMapSize(50, 100));

log("Creating mud patches...");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(2, Math.floor(scaleByMapSize(3, 6)), size, 1),
		[
			new LayeredPainter([tGrassA, tGrassB, tMud], [1, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 1, clHill, 0, clDirt, 5, clPlayer, 8),
		scaleByMapSize(15, 45));
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
	[avoidClasses(clWater, 0, clPlayer, 20, clRock, 10, clHill, 1)],
	scaleByMapSize(4, 16),
	100);

createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
	0,
	[avoidClasses(clWater, 0, clPlayer, 20, clRock, 10, clHill, 1)],
	scaleByMapSize(4, 16),
	100);

log("Creating metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[avoidClasses(clWater, 0, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1)],
	scaleByMapSize(4, 16),
	100);
RMS.SetProgress(60);

log("Creating small decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aRockMedium, 1, 3, 0, 1)], true),
	0,
	avoidClasses(clPlayer, 1),
	scaleByMapSize(16, 262),
	50);
RMS.SetProgress(65);

log("Creating large decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aRockLarge, 1, 2, 0, 1),
			new SimpleObject(aRockMedium, 1, 3, 0, 2)
		],
		true),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131),
	50);
RMS.SetProgress(70);

log("Creating lions...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oLion, 0, 1, 0, 4),
			new SimpleObject(oLioness, 2, 3, 0, 4)
		],
		true,
		clFood),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11, clHill, 1),
	scaleByMapSize(4, 12),
	50);

log("Creating zebras...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oZebra, 4, 6, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

log("Creating wildebeest...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oWildebeest, 2, 4, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

log("Creating crocodiles...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oCrocodile, 2, 3, 0, 4)],
		true,
		clFood),
	0,
	[avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 13), stayClasses(clWater, 3)],
	5 * numPlayers,
	200);

log("Creating gazelles...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oGazelle, 4, 6, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);
RMS.SetProgress(75);

log("Creating rabbits...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oRabbit, 6, 8, 0, 2)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	6 * numPlayers,
	50);

log("Creating rhinos...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oRhino, 1, 1, 0, 2)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

log("Creating elephants...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oElephant, 2, 3, 0, 4), new SimpleObject(oElephantInfant, 1, 1, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 13),
	3 * numPlayers,
	50);

log("Creating berry bushes...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)],
		true,
		clFood),
	0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2,
	50);
RMS.SetProgress(80);

log("Creating straggler trees...");
let treeTypes = [oToona, oBaobab, oBush, oBush];
for (let treeType of treeTypes)
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[new SimpleObject(treeType, 1, 3, 0, 3)],
			true,
			clForest),
		0,
		avoidClasses(clForest, 1, clWater, 1, clHill, 1, clPlayer, 13, clMetal, 1, clRock, 1),
		Math.floor(scaleByMapSize(60, 500) / treeTypes.length));
RMS.SetProgress(85);

log("Creating small grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aGrassShort, 1, 2, 0, 1, -PI/8, PI/8)]),
	0,
	avoidClasses(clWater, 2, clPlayer, 13, clDirt, 0),
	scaleByMapSize(13, 200));
RMS.SetProgress(90);

log("Creating large grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup([
		new SimpleObject(aGrass, 2, 4, 0, 1.8, -PI/8, PI/8),
		new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5, -PI/8, PI/8)
	]),
	0,
	avoidClasses(clWater, 3, clPlayer, 13, clDirt, 1, clForest, 0),
	scaleByMapSize(13, 200));
RMS.SetProgress(95);

log("Creating bushes...");
createObjectGroupsDeprecated(
	new SimpleGroup(
	[
		new SimpleObject(aBushMedium, 1, 2, 0, 2),
		new SimpleObject(aBushSmall, 2, 4, 0, 2)
	]),
	0,
	avoidClasses(clWater, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

log("Creating flowering shrubs...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aShrub, 1, 1, 0, 2)]),
	0,
	avoidClasses(clWater, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

log("Creating decorative palms...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aPalm, 1, 3, 0, 2)]),
	0,
	avoidClasses(clWater, 2, clPlayer, 12, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

log("Creating shrubs,flowers and other decorations...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(aFlower, 0, 6, 0, 2),
			new SimpleObject(aGrass2, 2, 5, 0, 2)
		]),
	0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 13, clDirt, 1),
	scaleByMapSize(13, 200),
	50);

setSkySet("cirrus");
setWaterColor(0.553, 0.635, 0.345);
setWaterTint(0.161, 0.514, 0.635);
setWaterMurkiness(0.8);
setWaterWaviness(1.0);
setWaterType("clap");

setFogThickness(0.25);
setFogFactor(0.6);

setPPEffect("hdr");
setPPSaturation(0.44);
setPPBloom(0.3);

ExportMap();
