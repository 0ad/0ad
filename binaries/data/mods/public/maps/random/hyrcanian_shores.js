Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

setSelectedBiome();

const tPrimary = g_Terrains.mainTerrain;
const tGrass = g_Terrains.tier1Terrain;
const tGrassPForest = g_Terrains.forestFloor1;
const tGrassDForest = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tGrassA = g_Terrains.tier2Terrain;
const tGrassB = g_Terrains.tier3Terrain;
const tGrassC = g_Terrains.tier4Terrain;
const tHill = g_Terrains.hill;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tGrassPatch = g_Terrains.dirt;
const tShore = g_Terrains.shoreBlend;
const tWater = g_Terrains.shore;

const oPoplar = g_Gaia.tree1;
const oPalm = g_Gaia.tree2;
const oApple = g_Gaia.tree3;
const oOak = g_Gaia.tree4;
const oBerryBush = g_Gaia.fruitBush;
const oDeer = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oGoat = "gaia/fauna_goat";
const oBoar = "gaia/fauna_boar";
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oPoplar, tGrassDForest];
const pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oOak, tGrassPForest];

const heightSeaGround1 = -3;
const heightShore1 = -1.5;
const heightShore2 = 0;
const heightLand = 1;
const heightOffsetBump = 4;
const heightHill = 15;

var g_Map = new RandomMap(heightLand, tPrimary);

const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();
const numPlayers = getNumPlayers();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clHighlands = g_Map.createTileClass();

var waterPosition = fractionToTiles(0.25);
var highlandsPosition = fractionToTiles(0.75);

var startAngle = randomAngle();

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), playerPlacementLine(startAngle, mapCenter, fractionToTiles(0.2))],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"StartingAnimal": {
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
		"template": oOak,
		"count": 2
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(10);

paintRiver({
	"parallel": true,
	"start": new Vector2D(mapBounds.left, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapBounds.right, mapBounds.top).rotateAround(startAngle, mapCenter),
	"width": 2 * waterPosition,
	"fadeDist": scaleByMapSize(6, 25),
	"deviation": 0,
	"heightRiverbed": heightSeaGround1,
	"heightLand": heightLand,
	"meanderShort": 20,
	"meanderLong": 0,
	"waterFunc": (position, height, riverFraction) => {

		if (height < heightShore2)
			clWater.add(position);

		createTerrain(height < heightShore1 ? tWater : tShore).place(position);
	}
});
Engine.SetProgress(20);

g_Map.log("Marking highlands area");
createArea(
	new ConvexPolygonPlacer(
		[
			new Vector2D(mapBounds.left, mapBounds.top - highlandsPosition),
			new Vector2D(mapBounds.right, mapBounds.top - highlandsPosition),
			new Vector2D(mapBounds.left, mapBounds.bottom),
			new Vector2D(mapBounds.right, mapBounds.bottom)
		].map(pos => pos.rotateAround(startAngle, mapCenter)),
		Infinity),
	new TileClassPainter(clHighlands));

g_Map.log("Creating fish");
for (let i = 0; i < scaleByMapSize(10, 20); ++i)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oFish, 2, 3, 0, 2)], true, clFood),
		0,
		[stayClasses(clWater, 2), avoidClasses(clFood, 3)],
		numPlayers,
		50);
Engine.SetProgress(25);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(10, 60), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 3),
	stayClasses(clHighlands, 1),
	scaleByMapSize(300, 600));

Engine.SetProgress(30);

g_Map.log("Creating hills");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, Infinity),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 20, clWater, 5, clHill, 15, clHighlands, 5),
	scaleByMapSize(1, 4) * numPlayers);

Engine.SetProgress(35);

g_Map.log("Creating mainland forests");
const [forestTrees, stragglerTrees] = getTreeCounts(1000, 3500, 0.85);
const highlandShare = 0.4;
{
	var types = [
		[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]]
	];
	const numberOfForests = scaleByMapSize(20, 100) / types[0].length;
	for (const type of types)
		createAreas(
			new ClumpPlacer(forestTrees * (1.0 - highlandShare) / numberOfForests, 0.1, 0.1, Infinity),
			[
				new LayeredPainter(type, [2]),
				new TileClassPainter(clForest)
			],
			avoidClasses(clPlayer, 20, clWater, 3, clForest, 10, clHill, 0, clBaseResource, 3, clHighlands, 2),
			numberOfForests);
	Engine.SetProgress(45);
}

g_Map.log("Creating highland forests");
{
	var types = [
		[[tGrassDForest, tGrass, pForestP], [tGrassDForest, pForestP]]
	];
	const numberOfForests = scaleByMapSize(8, 50) / types[0].length;
	for (const type of types)
		createAreas(
			new ClumpPlacer(forestTrees * (highlandShare) / numberOfForests, 0.1, 0.1, Infinity),
			[
				new LayeredPainter(type, [2]),
				new TileClassPainter(clForest)
			],
			[
				avoidClasses(clPlayer, 20, clWater, 3, clForest, 10, clHill, 0, clBaseResource, 3),
				stayClasses(clHighlands, 2)
			],
			numberOfForests,
			30);
}

Engine.SetProgress(70);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([[tGrass, tGrassA], [tGrassA, tGrassB], [tGrassB, tGrassC]], [1, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4),
		scaleByMapSize(15, 45));
Engine.SetProgress(75);

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new LayeredPainter([tGrassC, tGrassPatch], [2]),
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 6, clBaseResource, 6),
		scaleByMapSize(15, 45));

Engine.SetProgress(80);

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10, clHill, 2)],
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone quarries");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10, clHill, 2)],
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 2)],
	scaleByMapSize(5, 20), 100
);

Engine.SetProgress(85);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);
Engine.SetProgress(90);

g_Map.log("Creating large decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

g_Map.log("Creating deer");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 0, clFood, 5),
	6 * numPlayers, 50
);

g_Map.log("Creating sheep");
group = new SimpleGroup(
	[new SimpleObject(oGoat, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 0, clFood, 20),
	3 * numPlayers, 50
);

g_Map.log("Creating berry bush");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 6, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

g_Map.log("Creating boar");
group = new SimpleGroup(
	[new SimpleObject(oBoar, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 0, clFood, 20),
	2 * numPlayers, 50
);

createStragglerTrees(
	[oPoplar, oPalm, oApple],
	avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 10, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

g_Map.log("Creating small grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	scaleByMapSize(13, 200)
);

g_Map.log("Creating large grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	scaleByMapSize(13, 200)
);
Engine.SetProgress(95);

g_Map.log("Creating bushes");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	scaleByMapSize(13, 200), 50
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setWaterWaviness(2.0);
setWaterType("ocean");

g_Map.ExportMap();
