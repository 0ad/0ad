RMS.LoadLibrary("rmgen");
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
const tPass = ["alpine_cliff_b", "alpine_cliff_c", "alpine_grass_rocky", "alpine_grass_rocky", "alpine_grass_rocky"];

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

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clDirt = createTileClass();
var clLush = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clPass = createTileClass();
var clPyrenneans = createTileClass();
var clPass = createTileClass();
var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();

// Initial Terrain Creation
// I'll use very basic noised sinusoidal functions to give the terrain a way aspect
// It looks like we can't go higher than ≈ 75. Given this I'll lower the ground

const baseHeight = -6;
setWaterHeight(8);

// let's choose the angle of the pyreneans
var MoutainAngle = randFloat(0,TWO_PI);
var lololo = randFloat(-PI/12,-PI/12);	// used by oceans

var baseHeights = [];
for (var ix = 0; ix < mapSize; ix++)
{
	baseHeights.push([]);
	for (var iz = 0; iz < mapSize; iz++)
	{
		if (g_Map.inMapBounds(ix,iz))
		{
			placeTerrain(ix, iz, tGrass);
			setHeight(ix,iz,baseHeight +randFloat(-1,1) + scaleByMapSize(1,3)*(cos(ix/scaleByMapSize(5,30))+sin(iz/scaleByMapSize(5,30))));
			baseHeights[ix].push( baseHeight +randFloat(-1,1) + scaleByMapSize(1,3)*(cos(ix/scaleByMapSize(5,30))+sin(iz/scaleByMapSize(5,30)))  );
		}
		else
			baseHeights[ix].push(-100);
	}
}

var playerIDs = primeSortAllPlayers();

// place players
var playerX = [];
var playerZ = [];
var playerAngle = [];

for (var i = 0; i < numPlayers; i++)
{
	if ( i%2 == 1)
		playerAngle[i] = MoutainAngle+lololo + PI/2 + i/numPlayers*(PI/3) + (1-i/numPlayers)*(-PI/3);
	else
		playerAngle[i] = MoutainAngle + lololo - PI/2 + (i+1)/numPlayers*(PI/3) + (1-(i+1)/numPlayers)*(-PI/3);
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oPine, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}
RMS.SetProgress(30);

log ("Creating the pyreneans...");

// This is the basic orientation of the pyreneans
var MountainStartX = fractionToTiles(0.5) + cos(MoutainAngle)*fractionToTiles(0.34);
var MountainStartZ = fractionToTiles(0.5) + sin(MoutainAngle)*fractionToTiles(0.34);
var MountainEndX = fractionToTiles(0.5) - cos(MoutainAngle)*fractionToTiles(0.34);
var MountainEndZ = fractionToTiles(0.5) - sin(MoutainAngle)*fractionToTiles(0.34);
var MountainHeight = scaleByMapSize(50,65);
// Number of peaks
var NumOfIterations = scaleByMapSize(100,1000);

var randomNess = randFloat(-scaleByMapSize(1,12),scaleByMapSize(1,12));

for (var i = 0; i < NumOfIterations; i++)
{
	RMS.SetProgress(45 * i/NumOfIterations + 30 * (1-i/NumOfIterations));

	var position = i/NumOfIterations;
	var width = scaleByMapSize(15,55);

	var randHeight2 = randFloat(0,10) + MountainHeight;

	for (var dist = 0; dist < width*3; dist++)
	{
		var okDist = dist/3;
		var S1x = round((MountainStartX * (1-position) + MountainEndX*position) + randomNess*cos(position*3.14*4) + cos(MoutainAngle+PI/2)*okDist);
		var S1z = round((MountainStartZ * (1-position) + MountainEndZ*position) + randomNess*sin(position*3.14*4) + sin(MoutainAngle+PI/2)*okDist);
		var S2x = round((MountainStartX * (1-position) + MountainEndX*position) + randomNess*cos(position*3.14*4) + cos(MoutainAngle-PI/2)*okDist);
		var S2z = round((MountainStartZ * (1-position) + MountainEndZ*position) + randomNess*sin(position*3.14*4) + sin(MoutainAngle-PI/2)*okDist);

		// complicated sigmoid
		// Ranges is 0-1, FormX is 0-1 too.
		var FormX = (-2*(1-okDist/width)+1.9) - 4*(2*(1-okDist/width)-randFloat(0.9,1.1))*(2*(1-okDist/width)-randFloat(0.9,1.1))*(2*(1-okDist/width)-randFloat(0.9,1.1));
		var Formula = (1/(1 + Math.exp(FormX)));

		// If we're too far from the border, we flatten
		Formula *= (0.2 - Math.max(0,abs(0.5 - position) - 0.3)) * 5;

		var randHeight = randFloat(-9,9) * Formula;

		var height = baseHeights[S1x][S1z];
		setHeight(S1x,S1z, height + randHeight2 * Formula + randHeight );
		var height = baseHeights[S2x][S2z];
		setHeight(S2x,S2z, height + randHeight2 * Formula + randHeight );
		if (getHeight(S1x,S1z) > 15)
			addToClass(S1x,S1z, clPyrenneans);
		if (getHeight(S2x,S2z) > 15)
			addToClass(S2x,S2z, clPyrenneans);
	}
}
// Allright now slight smoothing (decreasing with height)
for (var ix = 1; ix < mapSize-1; ix++)
{
	for (var iz = 1; iz < mapSize-1; iz++)
	{
		if (g_Map.inMapBounds(ix,iz) && checkIfInClass(ix,iz,clPyrenneans) ) {
			var NB = getNeighborsHeight(ix,iz);
			var index = 9/(1 + Math.max(0,getHeight(ix,iz)/7));
			setHeight(ix,iz, (getHeight(ix,iz)*(9-index) + NB*index)/9 );
		}
	}
}
RMS.SetProgress(48);

// Okay so the mountains are pretty much here.
// Making the passes

var passWidth = scaleByMapSize(15,100) /1.8;
var S1x = round((MountainStartX * (0.35) + MountainEndX*0.65) + cos(MoutainAngle+PI/2)*passWidth);
var S1z = round((MountainStartZ * (0.35) + MountainEndZ*0.65) + sin(MoutainAngle+PI/2)*passWidth);
var S2x = round((MountainStartX * (0.35) + MountainEndX*0.65) + cos(MoutainAngle-PI/2)*passWidth);
var S2z = round((MountainStartZ * (0.35) + MountainEndZ*0.65) + sin(MoutainAngle-PI/2)*passWidth);
PassMaker(S1x, S1z, S2x, S2z, 4, 7, (getHeight(S1x,S1z) + getHeight(S2x,S2z))/2.0, MountainHeight-25, 2, clPass);

S1x = round((MountainStartX * (0.65) + MountainEndX*0.35) + cos(MoutainAngle+PI/2)*passWidth);
S1z = round((MountainStartZ * (0.65) + MountainEndZ*0.35) + sin(MoutainAngle+PI/2)*passWidth);
S2x = round((MountainStartX * (0.65) + MountainEndX*0.35) + cos(MoutainAngle-PI/2)*passWidth);
S2z = round((MountainStartZ * (0.65) + MountainEndZ*0.35) + sin(MoutainAngle-PI/2)*passWidth);
PassMaker(S1x, S1z, S2x, S2z, 4, 7, (getHeight(S1x,S1z) + getHeight(S2x,S2z))/2.0, MountainHeight-25, 2, clPass);

RMS.SetProgress(50);

// Smoothing the mountains
for (var ix = 1; ix < mapSize-1; ix++)
{
	for (var iz = 1; iz < mapSize-1; iz++)
	{
		if ( g_Map.inMapBounds(ix,iz) && checkIfInClass(ix,iz,clPyrenneans) ) {
			var NB = getNeighborsHeight(ix,iz);
			var index = 9/(1 + Math.max(0,(getHeight(ix,iz)-10)/7));
			setHeight(ix,iz, (getHeight(ix,iz)*(9-index) + NB*index)/9 );
			baseHeights[ix][iz] = (getHeight(ix,iz)*(9-index) + NB*index)/9;
		}
	}
}

log ("creating Oceans");
// ALlright for hacky reasons I can't use a smooth Elevation Painter, that wouldn't work.
// I'll use a harsh one, and then smooth it out
var OceanX = fractionToTiles(0.5) + cos(MoutainAngle + lololo)*fractionToTiles(0.48);
var OceanZ = fractionToTiles(0.5) + sin(MoutainAngle + lololo)*fractionToTiles(0.48);

createArea(
	new ClumpPlacer(diskArea(fractionToTiles(0.18)), 0.9, 0.05, 10, OceanX, OceanZ),
	[
		paintClass(clWater),
		new ElevationPainter(-22)
	],
	null);

OceanX = fractionToTiles(0.5) + cos(PI + MoutainAngle + lololo)*fractionToTiles(0.48);
OceanZ = fractionToTiles(0.5) + sin(PI + MoutainAngle + lololo)*fractionToTiles(0.48);

createArea(
	new ClumpPlacer(diskArea(fractionToTiles(0.18)), 0.9, 0.05, 10, OceanX, OceanZ),
	[
		new ElevationPainter(-22),
		paintClass(clWater)
	],
	null);

// Smoothing around the water, then going a bit random
for (var ix = 1; ix < mapSize-1; ix++)
{
	for (var iz = 1; iz < mapSize-1; iz++)
	{
		if ( g_Map.inMapBounds(ix,iz) && getTileClass(clWater).countInRadius(ix,iz,5,true) > 0 ) {
			// Allright smoothing
			// I'll have to hack again.

			var averageHeight = 0;
			var size = 5;
			if (getTileClass(clPyrenneans).countInRadius(ix,iz,1,true) > 0)
				size = 1;
			else if (getTileClass(clPyrenneans).countInRadius(ix,iz,2,true) > 0)
				size = 2;
			else if (getTileClass(clPyrenneans).countInRadius(ix,iz,3,true) > 0)
				size = 3;
			else if (getTileClass(clPyrenneans).countInRadius(ix,iz,4,true) > 0)
				size = 4;
			var todivide = 0;
			for (var xx = -size; xx <= size;xx++)
				for (var yy = -size; yy <= size;yy++) {
					if (g_Map.inMapBounds(ix + xx,iz + yy) && (xx != 0 || yy != 0)){
						averageHeight += getHeight(ix + xx,iz + yy) / (abs(xx)+abs(yy));
						todivide += 1/(abs(xx)+abs(yy));
					}
				}
			averageHeight += getHeight(ix,iz)*2;
			averageHeight /= (todivide+2);

			setHeight(ix,iz, averageHeight );
			//baseHeights[ix][iz] = averageHeight;
		}
		if ( g_Map.inMapBounds(ix,iz) && getTileClass(clWater).countInRadius(ix,iz,4,true) > 0 && getTileClass(clWater).countInRadius(ix,iz,4) > 0 )
			setHeight(ix,iz, getHeight(ix,iz) + randFloat(-1,1));
	}
}
RMS.SetProgress(55);

log ("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(60, 120), 0.3, 0.06, 5),
	[
		new SmoothElevationPainter(ELEVATION_MODIFY, 7, 4, 2),
		new TerrainPainter(tGrassSpecific),
		paintClass(clHill)
	],
	avoidClasses(clWater, 5, clPlayer, 20, clBaseResource, 6, clPyrenneans, 2), scaleByMapSize(5, 35));

log("Creating forests...");
var types = [[tForestTransition, pForestLandVeryLight, pForestLandLight, pForestLand]];
var size = scaleByMapSize(40,115)*PI;
var num = floor(scaleByMapSize(8,40) / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(size, 0.2, 0.1, 1),
		[
			new LayeredPainter(type, [scaleByMapSize(1, 2), scaleByMapSize(3, 6), scaleByMapSize(3, 6)]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 20, clPyrenneans,0, clForest, 7, clWater, 2),
		num);
RMS.SetProgress(60);

log("Creating lone trees...");
var num = scaleByMapSize(80,400);

var group = new SimpleGroup([new SimpleObject(oPine, 1,2, 1,3),new SimpleObject(oBeech, 1,2, 1,3)], true, clForest);
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 1, clPlayer, 8,clPyrenneans, 1), num, 20 );

log("Painting the map");

var terrainGrass = createTerrain(tGrass);
var terrainGrassMidRange = createTerrain(tGrassMidRange);
var terrainGrassHighRange = createTerrain(tGrassHighRange);
var terrainRocks = createTerrain(tHighRocks);
var terrainRocksSnow = createTerrain(tSnowedRocks);
var terrainTopSnow = createTerrain(tTopSnow);
var terrainTopSnowOnly = createTerrain(tTopSnowOnly);
var terrainMidRangeCliff = createTerrain(tMidRangeCliffs);
var terrainHighRangeCliff = createTerrain(tHighRangeCliffs);
var terrainPass = createTerrain(tPass);
var terrainSand = createTerrain(tSand);
var terrainSandTransition = createTerrain(tSandTransition);
var terrainWater = createTerrain(tWater);

for (var x = 0; x < mapSize; x++) {
	for (var z = 0; z < mapSize; z++) {
		var height = getHeight(x,z);
		var heightDiff = getHeightDifference(x,z);
		if (getTileClass(clPyrenneans).countInRadius(x,z,2,true) > 0) {
			if (height < 6) {
				if (heightDiff < 5)
					terrainGrass.place(x,z);
				else
					terrainMidRangeCliff.place(x,z);
			} else if (height >= 6 && height < 18) {
				if (heightDiff < 8)
					terrainGrassMidRange.place(x,z);
				else
					terrainMidRangeCliff.place(x,z);
			} else if (height >= 18 && height < 30) {
				if (heightDiff < 8)
					terrainGrassHighRange.place(x,z);
				else
					terrainMidRangeCliff.place(x,z);
			} else if (height >= 30 && height < MountainHeight-20) {
				if (heightDiff < 8)
					terrainRocks.place(x,z);
				else
					terrainHighRangeCliff.place(x,z);
			} else if (height >= MountainHeight-20 && height < MountainHeight-10) {
				if (heightDiff < 7)
					terrainRocksSnow.place(x,z);
				else
					terrainHighRangeCliff.place(x,z);
			} else if (height >= MountainHeight-10) {
				if (heightDiff < 6)
					terrainTopSnowOnly.place(x,z);
				else
					terrainTopSnow.place(x,z);
			}
			if (height >= 30 && getTileClass(clPass).countInRadius(x,z,2,true) > 0)
				if (heightDiff < 5)
					terrainPass.place(x,z);
		}
		if (height > -14 &&  height <= -2 && getTileClass(clWater).countInRadius(x,z,2,true) > 0) {
			if (heightDiff < 2.5)
				terrainSand.place(x,z);
			else
				terrainMidRangeCliff.place(x,z);
		} else if (height > -14 && height <= 0 && getTileClass(clWater).countInRadius(x,z,3,true) > 0) {
			if (heightDiff < 2.5)
				terrainSandTransition.place(x,z);
			else
				terrainMidRangeCliff.place(x,z);
		} else if (height <= -14) {
			terrainWater.place(x,z);
		}
	}
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
		[
			new TerrainPainter(tLushGrass),
			paintClass(clLush)
		],
		avoidClasses(clWater, 3, clForest, 0, clPyrenneans,5, clHill, 0, clDirt, 5, clPlayer, 6),
		scaleByMapSize(15, 45));

RMS.SetProgress(70);

// making more in dirt areas so as to appear different
log("Creating small grass tufts...");
var group = new SimpleGroup( [new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)] );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 2, clHill, 2, clPlayer, 5, clDirt, 0, clPyrenneans,2), scaleByMapSize(13, 200) );
createObjectGroupsDeprecated(group, 0, stayClasses(clDirt,1), scaleByMapSize(13, 200),10);

log("Creating large grass tufts...");
group = new SimpleGroup( [new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)] );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clHill, 2, clPlayer, 5, clDirt, 1, clForest, 0, clPyrenneans,2), scaleByMapSize(13, 200) );
createObjectGroupsDeprecated(group, 0, stayClasses(clDirt,1), scaleByMapSize(13, 200),10);
RMS.SetProgress(75);

log("Creating bushes...");
group = new SimpleGroup( [new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)] );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 2, clPlayer, 1, clPyrenneans, 1), scaleByMapSize(13, 200), 50 );

RMS.SetProgress(80);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 8, clPyrenneans, 1),  scaleByMapSize(4,16), 100 );

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,  avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 8, clPyrenneans, 1),  scaleByMapSize(4,16), 100 );

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 8, clRock, 5, clPyrenneans, 1), scaleByMapSize(4,16), 100  );

RMS.SetProgress(85);

log("Creating small decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRockMedium, 1,3, 0,1)], true );
createObjectGroupsDeprecated( group, 0, avoidClasses(clWater, 0, clForest, 0, clPlayer, 0), scaleByMapSize(16, 262), 50 );

log("Creating large decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)], true );
createObjectGroupsDeprecated( group, 0,  avoidClasses(clWater, 0, clForest, 0, clPlayer, 0), scaleByMapSize(8, 131), 50 );

RMS.SetProgress(90);

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

setSunElevation(randFloat(PI/5, PI / 3));
setSunRotation(randFloat(0, TWO_PI));

setSkySet("cumulus");
setSunColor(0.73,0.73,0.65);
setTerrainAmbientColor(0.45,0.45,0.50);
setUnitsAmbientColor(0.4,0.4,0.4);
setWaterColor(0.263, 0.353, 0.616);
setWaterTint(0.104, 0.172, 0.563);
setWaterWaviness(5.0);
setWaterType("ocean");
setWaterMurkiness(0.83);

ExportMap();

function getNeighborsHeight(x1, z1)
{
	var toCheck = [ [-1,-1], [-1,0], [-1,1], [0,1], [1,1], [1,0], [1,-1], [0,-1] ];
	var height = 0;
	for (var i in toCheck) {
		var xx = x1 + toCheck[i][0];
		var zz = z1 + toCheck[i][1];
		height += getHeight(round(xx),round(zz));
	}
	height /= 8;
	return height;
}
// Taken from Corsica vs Sardinia with tweaks
function PassMaker(x1, z1, x2, z2, startWidth, centerWidth, startElevation, centerElevation, smooth, tileclass, terrain)
{
	var mapSize = g_Map.size;
	var stepNB = sqrt((x2-x1)*(x2-x1) + (z2-z1)*(z2-z1)) + 2;

	var startHeight = startElevation;
	var finishHeight = centerElevation;

	for (var step = 0; step <= stepNB; step+=0.5)
	{
		var ix = ((stepNB-step)*x1 + x2*step) / stepNB;
		var iz = ((stepNB-step)*z1 + z2*step) / stepNB;

		var width = (abs(step - stepNB/2.0) *startWidth + (stepNB/2 - abs(step - stepNB/2.0)) * centerWidth ) / (stepNB/2);

		var oldDirection = [x2-x1, z2-z1];
		// let's get the perpendicular direction
		var direction = [ -oldDirection[1],oldDirection[0] ];
		if (abs(direction[0]) > abs(direction[1]))
		{
			direction[1] = direction[1] / abs(direction[0]);
			if (direction[0] > 0)
				direction[0] = 1;
			else
				direction[0] = -1;
		} else {
			direction[0] = direction[0] / abs(direction[1]);
			if (direction[1] > 0)
				direction[1] = 1;
			else
				direction[1] = -1;
		}
		for (var po = -Math.floor(width/2.0); po <= Math.floor(width/2.0); po+=0.5)
		{
			var rx = po*direction[0];
			var rz = po*direction[1];

			var targetHeight = (abs(step - stepNB/2.0) *startHeight + (stepNB/2 - abs(step - stepNB/2.0)) * finishHeight ) / (stepNB/2);
			if (round(ix + rx) < mapSize && round(iz + rz) < mapSize && round(ix + rx) >= 0 && round(iz + rz) >= 0)
			{
				// smoothing the sides
				if ( abs(abs(po) - abs(Math.floor(width/2.0))) < smooth)
				{
					var localHeight = getHeight(round(ix + rx), round(iz + rz));
					var localPart = smooth - abs(abs(po) - abs(Math.floor(width/2.0)));
					var targetHeight = (localHeight * localPart + targetHeight * (1/localPart) )/ (localPart + 1/localPart);
				}

				g_Map.setHeight(round(ix + rx), round(iz + rz), targetHeight);
				if (tileclass != null)
					addToClass(round(ix + rx), round(iz + rz), tileclass);
				if (terrain != null)
					placeTerrain(round(ix + rx), round(iz + rz), terrain);
			}
		}
	}
}
// no need for preliminary rounding
function getHeightDifference(x1, z1)
{
	x1 = round(x1);
	z1 = round(z1);
	var height = getHeight(x1,z1);

	if (!g_Map.inMapBounds(x1,z1))
		return 0;
	// I wanna store the height difference with any neighbor

	var toCheck = [ [-1,-1], [-1,0], [-1,1], [0,1], [1,1], [1,0], [1,-1], [0,-1] ];

	var diff = 0;
	var todiv = 0;
	for (var i in toCheck) {
		var xx = round(x1 + toCheck[i][0]);
		var zz = round(z1 + toCheck[i][1]);
		if (g_Map.inMapBounds(xx,zz)) {
			diff += abs(getHeight(xx,zz) - height);
			todiv++;
		}
	}
	if (todiv > 0)
		diff /= todiv;
	return diff;
}

