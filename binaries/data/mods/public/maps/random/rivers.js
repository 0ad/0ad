Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.hill;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tTier4Terrain = g_Terrains.tier4Terrain;
var tShore = g_Terrains.shore;
var tWater = g_Terrains.water;
if (currentBiome() == "tropic")
{
	tShore = "tropic_dirt_b_plants";
	tWater = "tropic_dirt_b";
}
const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aReeds = g_Decoratives.reeds;
const aLillies = g_Decoratives.lillies;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = getMapArea();
const mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clShallow = createTileClass();

var waterHeight = -3;
var shallowHeight = -1;

initTerrain(tMainTerrain);

var [playerIDs, playerX, playerZ, playerAngle, startAngle] = playerPlacementCircle(0.35);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerX, playerZ],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
	},
	"Berries": {
		"template": oFruitBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTree1,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});

log("Creating central lake...");
createArea(
	new ClumpPlacer(
		mapArea / 100 * Math.pow(scaleByMapSize(1, 6), 1/8),
		0.7,
		0.1,
		10,
		mapCenter.x,
		mapCenter.y),
	[
		new LayeredPainter([tShore, tWater, tWater, tWater], [1, 4, 2]),
		new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
		paintClass(clWater)
	],
	null);

log("Creating rivers between opponents...");
let rivers = distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.5), mapCenter)[0];
for (let i = 0; i < numPlayers; ++i)
{
	if (areAllies(playerIDs[i], playerIDs[(i + 1) % numPlayers]))
		continue;

	let shallowLocation = randFloat(0.2, 0.7);
	let shallowWidth = randFloat(0.12, 0.21);

	paintRiver({
		"parallel": true,
		"startX": tilesToFraction(rivers[i].x),
		"startZ": tilesToFraction(rivers[i].y),
		"endX": tilesToFraction(mapCenter.x),
		"endZ": tilesToFraction(mapCenter.y),
		"width": tilesToFraction(scaleByMapSize(10, 30)),
		"fadeDist": tilesToFraction(5),
		"deviation": 0,
		"landHeight": getMapBaseHeight(),
		"waterHeight": waterHeight,
		"minHeight": waterHeight,
		"meanderShort": tilesToFraction(scaleByMapSize(20, 60) * scaleByMapSize(35, 160)),
		"meanderLong": 0,
		"waterFunc": (ix, iz, height, riverFraction) => {

			addToClass(ix, iz, clWater);

			let isShallow = height < shallowHeight &&
				riverFraction > shallowLocation &&
				riverFraction < shallowLocation + shallowWidth;

			let newHeight = isShallow ? shallowHeight : Math.max(height, waterHeight);

			if (getHeight(ix, iz) < newHeight)
				return;

			setHeight(ix, iz, newHeight);
			placeTerrain(ix, iz, height >= 0 ? tShore : tWater);

			if (isShallow)
				addToClass(ix, iz, clShallow);
		}
	});
}
Engine.SetProgress(40);

createBumps(avoidClasses(clWater, 2, clPlayer, 20));

if (randBool())
	createHills([tMainTerrain, tCliff, tHill], avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(3, 15));
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(3, 15));

var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 2),
 clForest,
 forestTrees);

Engine.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
 scaleByMapSize(15, 45),
 clDirt);
Engine.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
 clRock);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
);

Engine.SetProgress(65);

var planetm = 1;

if (currentBiome() == "tropic")
	planetm = 8;

createDecoration(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
		[new SimpleObject(aGrass, 2, 4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
		[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200)
	],
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0));

createDecoration(
	[
		[new SimpleObject(aReeds, 1, 3, 0, 1)],
		[new SimpleObject(aLillies, 1, 2, 0, 1)]
	],
	[
		scaleByMapSize(800, 12800),
		scaleByMapSize(800, 12800)
	],
	stayClasses(clShallow, 0));

Engine.SetProgress(70);

createFood(
	[
		[new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)],
		[new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)]
	],
	[
		3 * numPlayers,
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	clFood);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[
		3 * numPlayers
	],
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	clFood);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		25 * numPlayers
	],
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	clFood);

Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	avoidClasses(clWater, 5, clForest, 7, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

setWaterWaviness(3.0);
setWaterType("lake");

ExportMap();
