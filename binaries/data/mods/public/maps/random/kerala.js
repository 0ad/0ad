Engine.LoadLibrary("rmgen");

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
const oPalm = "gaia/flora_tree_palm_tropic";
const oStoneLarge = "gaia/geology_stonemine_tropic_quarry";
const oStoneSmall = "gaia/geology_stone_tropic_a";
const oMetalLarge = "gaia/geology_metal_tropic_slabs";
const oFish = "gaia/fauna_fish";
const oDeer = "gaia/fauna_deer";
const oSheep = "gaia/fauna_tiger";
const oBush = "gaia/flora_bush_berry";

const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBush1 = "actor|props/flora/plant_tropic_a.xml";
const aBush2 = "actor|props/flora/plant_lg.xml";
const aBush3 = "actor|props/flora/plant_tropic_large.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oTree, tForestFloor];
const pForestP = [tForestFloor + TERRAIN_SEPARATOR + oPalm, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clMountains = createTileClass();

const waterPos = 0.31;
const mountainPos = 0.69;

placePlayerBases({
	"PlayerPlacement": playerPlacementLine(false, 0.55, 0.2),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
	},
	"Berries": {
		"template": oBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTree,
		"count": scaleByMapSize(12, 30),
		"minDist": 12,
		"maxDist": 14,
		"minDistGroup": 1,
		"maxDistGroup": 3
	}
	// No decoratives
});
Engine.SetProgress(15);

paintRiver({
	"parallel": true,
	"startX": 0,
	"startZ": 0,
	"endX": 0,
	"endZ": 1,
	"width": 2 * waterPos,
	"fadeDist": 0.025,
	"deviation": 0,
	"waterHeight": -5,
	"landHeight": 3,
	"meanderShort": 20,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height, riverFraction) => {
		addToClass(ix, iz, clWater);
	},
	"landFunc": (ix, iz, shoreDist1, shoreDist2) => {
		if (ix > fractionToTiles(mountainPos))
			addToClass(ix, iz, clMountains)
	}
});

log("Creating shores...");
for (let i = 0; i < scaleByMapSize(20, 120); ++i)
	createArea(
		new ChainPlacer(
			1,
			Math.floor(scaleByMapSize(4, 6)),
			Math.floor(scaleByMapSize(16, 30)),
			1,
			randIntExclusive(0.28 * mapSize, 0.34 * mapSize),
			randIntExclusive(0.1 * mapSize, 0.9 * mapSize)),
		[
			new LayeredPainter([tGrass, tGrass], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 3, 3),
			unPaintClass(clWater)
		],
		null);

paintTerrainBasedOnHeight(-6, 1, 1, tWater);
paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTerrainBasedOnHeight(0, 1, 1, tShore);
paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);

Engine.SetProgress(45);

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new LayeredPainter([tCliff, tGrass], [3]),
		new SmoothElevationPainter(ELEVATION_SET, 25, 3),
		paintClass(clHill)
	],
	[avoidClasses(clPlayer, 20, clHill, 5, clWater, 2, clBaseResource, 2), stayClasses(clMountains, 0)],
	scaleByMapSize(5, 40) * numPlayers);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(1000, 6000, 0.7);
var types = [
	[[tGrass, tGrass, tGrass, tGrass, pForestD], [tGrass, tGrass, tGrass, pForestD]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP], [tGrass, tGrass, tGrass, pForestP]]
];
var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(
			1,
			Math.floor(scaleByMapSize(3, 5)),
			forestTrees / (num * Math.floor(scaleByMapSize(2, 4))),
			0.5),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 20, clForest, 10, clHill, 0, clWater, 8),
		num);

Engine.SetProgress(70);

log("Creating grass patches...");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tGrassC, tGrassA, tGrassB], [2, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 8, clForest, 0, clHill, 0, clPlayer, 12, clDirt, 16),
		scaleByMapSize(20, 80));

for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tPlants, tPlants], [1]),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 8, clForest, 0, clHill, 0, clPlayer, 12, clDirt, 16),
		scaleByMapSize(20, 80));

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	3*scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	3*scaleByMapSize(8, 131), 50
);

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aBush1, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	8 * scaleByMapSize(13, 200)
);

Engine.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aBush1, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	8 * scaleByMapSize(13, 200)
);

Engine.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBush3, 1,2, 0,2), new SimpleObject(aBush2, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	8 * scaleByMapSize(13, 200), 50
);

Engine.SetProgress(95);

createStragglerTrees(
	[oTree, oPalm],
	avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

Engine.SetProgress(75);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 6, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 22, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

setSunColor(0.6, 0.6, 0.6);
setSunElevation(PI/ 3);

setWaterColor(0.524,0.734,0.839);
setWaterTint(0.369,0.765,0.745);
setWaterWaviness(1.0);
setWaterType("ocean");
setWaterMurkiness(0.35);

setFogFactor(0.4);
setFogThickness(0.2);

setPPEffect("hdr");
setPPContrast(0.7);
setPPSaturation(0.65);
setPPBloom(0.6);

setSkySet("cirrus");
ExportMap();
